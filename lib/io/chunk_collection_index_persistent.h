#ifndef CHUNK_COLLECTION_INDEX_PERSISTENT_H
#define CHUNK_COLLECTION_INDEX_PERSISTENT_H

#include <serialization/fragment_size_struct.h>
#include <io/fragment_on_seekable_device.h>
#include <io/file.h>
#include <io/temp_file.h>
#include <util/exception.h>

#include <vector>
#include <ios>
#include <memory>
#include <algorithm>
#include <mutex>

namespace uh::io
{

// ---------------------------------------------------------------------

class chunk_collection_index_persistent:
    public std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>>
{
public:
    ~chunk_collection_index_persistent();

    /**
     * Guarantee that the index from the chunk collection is persisted and copied to memory
     *
     * @param chunk_collection_file is the incoming open chunk collection file
     */
    explicit chunk_collection_index_persistent(io::file& chunk_collection_file);

    /**
     * Emplace object to memory and also append it to index
     */
    std::pair<serialization::fragment_serialize_size_format,
              std::streamoff> emplace_back_index(serialization::fragment_serialize_size_format write_format,
                                                 std::size_t emplace_size,
                                                 bool flush_after_operation = true);

    /**
     *
     * @return a list of used index sort orders
     */
    std::vector<uint8_t> get_index_num_content_list(const std::vector<uint8_t>& without = std::vector<uint8_t>{});

    /**
     *
     * @return the count of addresses used
     */
    uint16_t count();

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
     * @return index file size
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
    [[nodiscard]] bool full();

    /**
     *
     * @return tell how many addresses are still free
     */
    uint16_t free();

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

    /**
     * Rename the temp_file to `path` and make it a permanent file, in case we are based on temp_file,
     * reset index file to new path and close the stream afterwards
     *
     * @throw a file with the given name already exists
     */
    void release_to(const std::filesystem::path& release_path);

    /**
     * Delete index file that was created from chunk collection
     */
    void maybe_forget_index_file();

    /**
     * Recreate index file from chunk collection file
     */
    void maybe_recreate_index_file();

    /*
     * getters and setters
     */
    [[nodiscard]] size_t getM_index_file_size() const;
    void setM_index_file_size(size_t mIndexFileSize);
    [[nodiscard]] bool isM_index_file_forgotten() const;
    void setM_index_file_forgotten(bool mIndexFileForgotten);

    void copy(const std::unique_ptr<chunk_collection_index_persistent>& input_collection);

private:
    std::unique_ptr<io::file> m_index_file;
    io::file& m_workfile;
    std::size_t m_index_file_size;
    bool m_index_file_forgotten = false;

    std::recursive_mutex m_index_work_mux{};

};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif //CHUNK_COLLECTION_INDEX_PERSISTENT_H
