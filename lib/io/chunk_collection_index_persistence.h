#ifndef CHUNK_COLLECTION_INDEX_PERSISTENCE_H
#define CHUNK_COLLECTION_INDEX_PERSISTENCE_H

#include <serialization/fragment_size_struct.h>
#include <io/fragment_on_seekable_device.h>
#include <io/file.h>
#include <io/temp_file.h>
#include <util/exception.h>

#include <vector>
#include <ios>
#include <memory>
#include <algorithm>

namespace uh::io
{

// ---------------------------------------------------------------------

namespace
{

// ---------------------------------------------------------------------

std::filesystem::path index_path(std::unique_ptr<io::file>& collection_file)
{
    return collection_file->path().replace_extension(".index");
}

// ---------------------------------------------------------------------

std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>>
maybe_index_persist_chunk_collection(std::unique_ptr<io::file>& collection_file)
{
    auto filename_index = index_path(collection_file);
    bool is_index_persisted = std::filesystem::exists(filename_index);

    std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>> output_index;
    std::streamoff collection_offset{};

    if (is_index_persisted)
    {
        io::file read_index_file(filename_index, std::ios_base::binary | std::ios_base::in);

        std::string read_buffer;
        read_buffer.resize(sizeof(serialization::fragment_serialize_size_format));

        while (io::read(read_index_file, read_buffer))
        {
            serialization::fragment_serialize_size_format skip_format;
            std::istringstream ss(read_buffer);
            skip_format.deserialize(ss);

            output_index.emplace_back(skip_format, collection_offset);
            collection_offset += skip_format.header_size + skip_format.content_size;
        }

        if (collection_offset < collection_file->size())
        {
            read_index_file.close();
            std::filesystem::remove(read_index_file.path());
            output_index.clear();
        }
        else
            return output_index;
    }

    auto temporarily_cached_fragment_on_seekable_device =
        io::fragment_on_seekable_device(*collection_file);

    io::file write_index_file(filename_index, std::ios_base::binary | std::ios_base::out);

    if (collection_file->size())
    {
        uint16_t index_entry_count{};
        serialization::fragment_serialize_size_format skip_format;

        do
        {
            skip_format = temporarily_cached_fragment_on_seekable_device.skip();

            if (skip_format.content_size == 0)
                break;

            if (index_entry_count > std::numeric_limits<unsigned char>::max() + 1)
            THROW(util::exception,
                  "Indexing of chunk collection " + collection_file->path().string() + " exceeded limits");

            output_index.emplace_back(skip_format, collection_offset);

            io::write(write_index_file, skip_format.serialize().str());

            collection_offset += skip_format.header_size + skip_format.content_size;
            index_entry_count++;
        }
        while (skip_format.content_size > 0);
    }

    return output_index;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

class chunk_collection_index_persistence:
    public std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>>
{
public:

    /**
     * Guarantee that the index from the chunk collection is persisted and copied to memory
     *
     * @param chunk_collection_file is the incoming open chunk collection file
     */
    explicit chunk_collection_index_persistence(std::unique_ptr<io::file>& chunk_collection_file);

    /**
     * Emplace object to memory and also append it to index
     *
     * @tparam Args any incoming types that match base class type
     * @param args any number of matching arguments
     * @return iterator reference to emplace_back object
     */
    template<typename...Args>
    auto emplace_back_index(Args&& ... args)
    {
        auto tmp = emplace_back(args...);

        if(forgotten){
            maybe_index_persist_chunk_collection(m_workfile);
            m_index_file = io::file(index_path(m_workfile), std::ios_base::app);
            forgotten = false;
        }
        else
            io::write(m_index_file, back().first.serialize().str());

        return tmp;
    }

    /**
     * delete multiple index positions from index and index file
     *
     * @param at
     */
    void erase_index_items(const std::vector<uint8_t>& at);

    /**
     *
     * @return a list of used index sort orders
     */
    std::vector<uint8_t> get_index_num_content_list();

    /**
     *
     * @return the count of addresses used
     */
    uint16_t count() const;

    /**
     *
     * @return the accumulated size of the chunk collection together with number of indexes used and further
     * header- and content size bytes
     */
    std::size_t size();

    /**
     *
     * @param index_address is the address of a registered fragment/chunk
     * @return the size of the header and content payload of the fragment/chunk
     */
    std::size_t size(uint8_t index_address);

    /**
     *
     * @return size of index helper file on disk
     */
    std::size_t index_file_size();

    /**
     *
     * @param index_adress is the address of a registered fragment/chunk
     * @return the size of the content payload of the fragment/chunk
     */
    std::size_t content_size(uint8_t index_adress);

    /**
     *
     * @return if the chunk collection is full
     */
    [[nodiscard]] bool full() const;

    /**
     *
     * @return tell how many addresses are still free
     */
    uint16_t free() const;

    /**
     *
     * @return path of chunk collection
     */
    std::filesystem::path getPath();

    /**
     * @return next free address on chunk collection
     */
    uint8_t next_free_address();

    /**
     *
     * @param at is a list of chunk collection addresses to be removed
     * @return at in order of chunks existining on chunk collextion
     * @throw if an element in at is non existent on the chunk collection
     */
    std::vector<uint8_t> filtered_at_list_in_seek_order(const std::vector<uint8_t>& at);

    /**
     *
     * @param at is an element of a chunk collection address to be removed from index
     * @param start_pos start searching from index position
     * @return iterator of found position "at"
     */
    std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>>::iterator
    find_address(uint8_t at,
                 std::vector<std::pair<serialization::fragment_serialize_size_format,
                                       std::streamoff>>::iterator start_pos);

    void forget();

private:
    io::file m_index_file;
    std::unique_ptr<io::file>& m_workfile;
    bool forgotten = false;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif //CHUNK_COLLECTION_INDEX_PERSISTENCE_H
