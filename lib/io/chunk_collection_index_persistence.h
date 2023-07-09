#ifndef CHUNK_COLLECTION_INDEX_PERSISTENCE_H
#define CHUNK_COLLECTION_INDEX_PERSISTENCE_H

#include <serialization/fragment_size_struct.h>
#include <io/file.h>
#include <io/temp_file.h>

#include <vector>
#include <ios>
#include <memory>
#include <algorithm>

namespace uh::io
{

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

private:
    io::file m_index_file;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif //CHUNK_COLLECTION_INDEX_PERSISTENCE_H
