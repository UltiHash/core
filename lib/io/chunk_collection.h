#ifndef CORE_CHUNK_COLLECTION_H
#define CORE_CHUNK_COLLECTION_H

#include <io/file.h>
#include <io/fragment_on_seekable_device.h>
#include <io/chunk_collection_index_persistent.h>
#include <serialization/fragment_serialize_size_format.h>

#include <utility>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <span>
#include <mutex>

#define CHUNK_COLLECTION_BUFFER_SIZE 1 << 23

namespace uh::io
{

class chunk_collection
{

public:

    ~chunk_collection();

    /**
     * A chunk collection keeps track of the position and the movement
     * of incoming and outgoing chunks/fragments. It keeps filestreams open as long as possible, either until
     * the mode is switched from read to write or vice versa or on deconstruction of the chunk_collection
     * Scan existing chunk collection on construction.
     *
     * @param collection_temp_directory_else_file_path where the file containing the chunk collection is located
     * @param create_tempfile chunk collection behaves like a temp file; release to keep
     * @throw if chunk collection is corrupted after repair attempt
     */
    explicit chunk_collection(const std::filesystem::path& collection_temp_directory_else_file_path,
                              bool create_tempfile = false);

     /**
      *
      * @param buffer to be written
      * @param alloc allocate space and write fragment to chunk collection with the help of multiple buffers
      * WARNING: allocated space must be written completely (accumulated buffer size is longer or equal alloc)
      * @param flush_after_operation may keep filestream open to write multiple objects in the same spot
      * @param maybe_force_index force index position if it is unused
      * @return fragment_serialize_size_format for index, content_buf_size and content_size
      */
     serialization::fragment_serialize_size_format<> write_indexed
        (std::span<const char> buffer,
         uint32_t alloc = 0,
         bool flush_after_operation = true,
         int16_t maybe_force_index = -1);

     /**
      * read a certain index pointer and return a vector buffer of the content
      *
      * @param at index position
      * @param close_after_operation enables the user to keep the stream open to continuous read
      * @return a pair of a read back buffer and fragment_serialize_size_format for information about it
      */
     std::pair<std::vector<char>, serialization::fragment_serialize_size_format<>>
    read_indexed(uint8_t at, bool close_after_operation = false);

     /**
      * Write indexed multiple buffers and return a list of fragment size structs that contain written fragment
      * information.
      *
      * @param buffer a vector of buffer spans that should be written to chunk collection
      * @param flush_after_operation enables the user to keep the stream open to continuous write
      * @return a vector of fragment_serialize_size_format for information about written information
      */
     std::vector<serialization::fragment_serialize_size_format<>>
    write_indexed_multi(const std::vector<std::span<const char>>& buffer,
                        bool flush_after_operation = true);

     /**
      * read indexed multiple positions with smart seeking
      *
      * @param at read multiple buffers back to memory, indicated by the elements of at
      * @return a vector of pairs of a read back buffer and fragment_serialize_size_format for information about it
      */
     std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format<>>>
    read_indexed_multi(const std::vector<uint8_t>& at);

    /**
     *
     * @param at remove a vector of fragments by index
     */
    void remove(const std::vector<uint8_t>& at);

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
     * @param index_adress is the address of a registered fragment/chunk
     * @return the size of the header and content payload of the fragment/chunk
     */
    std::size_t size(uint8_t index_adress);

    /**
     * @return the size of the content payload of all fragments/chunks
     */
    std::size_t content_size();

    /**
     *
     * @param index_address is the address of a registered fragment/chunk
     * @return the size of the content payload of the fragment/chunk
     */
    std::size_t content_size(uint8_t index_address);

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
     *
     * @return path of chunk collection
     */
    std::filesystem::path getPath();

     /**
      * Overwrites the file of release path with this chunk collection; tries to also move and overwrite it's
      * index file
      *
      * @throw if kernel rename throws
      *
      * @param release_path path to overwrite to
      */
    void release_to(const std::filesystem::path& release_path);

    /**
     *
     * @return a list of used index numbers in the order they occur on the chunk collection
     */
    std::vector<uint8_t> get_index_num_content_list();

    /**
     * For space savings on disk the index file of chunk collection may be deleted
     */
    void maybe_forget_chunk_collection_index_file();

    /**
     *
     * @return average chunk size of chunk collection
     */
    long double average_chunk_size();

private:
    std::unique_ptr<chunk_collection_index_persistent> m_index;
    std::shared_ptr<io::file> m_workfile;

    std::recursive_mutex m_chunk_collection_workmux;

    bool m_behave_like_tempfile;

    void maybe_force_mode_flush_reopen(std::ios_base::openmode mode);
};

} // namespace uh::io

#endif //CORE_CHUNK_COLLECTION_H
