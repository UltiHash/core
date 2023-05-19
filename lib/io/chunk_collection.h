//
// Created by benjamin-elias on 18.05.23.
//

#include <io/multi_devices.h>
#include <io/file.h>

#include <filesystem>

#ifndef CORE_CHUNK_COLLECTION_H
#define CORE_CHUNK_COLLECTION_H

namespace uh::io {

    struct chunk_collection_entry{

    public:
        uint32_t fragment_absolute_position;
        uint32_t fragment_size;
        uint8_t fragment_index_count_position;

        chunk_collection_entry(uint32_t frag_abs,uint32_t frag_size,uint8_t frag_index):
        fragment_absolute_position(frag_abs),fragment_size(frag_size),fragment_index_count_position(frag_index){}

    };

    template<class SeekableDevice = file>
    requires(std::is_base_of_v<seekable_device,SeekableDevice>)
    class chunk_collection: public multi_devices{

    public:

        /**
         * a chunk collection keeps track of the position and the movement
         * of incoming and outgoing chunks/fragments.
         * It only builds up a temporary connection to an underlying seekable_device.
         * On constructor the fragmented seekable device is scanned for the
         * position of its contents.
         *
         * @param fragment_file is the path to the managed fragmented file
         */
        explicit chunk_collection(std::filesystem::path collection_path);

        /**
         * @param buffer is the data to be written or appended to the
         * chunk collection
         * @return is the size truly written to the underlying device
         */
        std::streamsize write(std::span<const char> buffer) override;

        /**
         * @param buffer to be read from the current unspecified position by
         * using the fragment implementation
         * @return the size of the fragment that was read with the help
         * of fragment implementation
         */
        std::streamsize read(std::span<char> buffer) override;

        /**
         *
         * @return if the current position has not reached the end of
         * the underlying seekable device
         */
        [[nodiscard]] bool valid() const override;

        /**
         *
         * @param buffer a list of buffers to be written to the chunk collection
         * @return stream sizes to determine if writing was completed (0 for not completed)
         */
        std::vector<std::streamsize> write(std::vector<std::span<const char>> buffer) override;

        /**
         *
         * @param buffer to read multiple contents at once from the current position
         * @return stream sizes to determine if reading was completed and correct
         */
        std::vector<std::streamsize> read(std::vector<std::span<char>> buffer) override;

        /**
         * write with returning the index that was assigned to the written buffer
         * or instead give an index
         *
         * @param buffer to be written
         * @return tuple<stream size, assigned index position>
         */
        std::tuple<std::streamsize,uint8_t> write_indexed(std::span<const char> buffer);

        /**
         * read a certain index pointer and return a vector buffer of the content
         *
         * @param at index
         * @return buffer
         */
        std::vector<char> read_indexed(uint8_t at);

        /**
         * write indexed multiple buffers
         */
        std::vector<std::tuple<std::streamsize,uint8_t>> write_indexed(const std::vector<std::span<const char>>& buffer);

        /**
         * read indexed multiple positions
         */
        std::vector<std::vector<char>> read_indexed(const std::vector<uint8_t>& at);

        /**
         *
         * @return the count of addresses used
         */
        uint8_t count();

        /**
         *
         * @return the accumulated size of the chunk collection together with index-, control- and size bytes
         */
        std::size_t size();

        /**
         *
         * @param index_adress is the address of a registered fragment/chunk
         * @return the size of the content payload of the fragment/chunk
         */
        std::size_t size(uint8_t index_adress);

        /**
         *
         * @param index_adress is the address of a registered fragment/chunk
         * @return the size of the content payload of the fragment/chunk
         */
        std::size_t size(std::size_t index_pos);

        [[nodiscard]] const std::filesystem::path &getPath() const;

        [[nodiscard]] uint8_t getAtCollectionIndexEntryCount() const;

        [[nodiscard]] uint32_t getAtCollectionOffsetCount() const;

    private:
        std::filesystem::path path;
        std::vector<chunk_collection_entry> index;
        uint8_t at_collection_index_entry_count{};
        uint32_t at_collection_offset_count{};

        uint8_t next_free_address();
    };

} // namespace uh::io

#endif //CORE_CHUNK_COLLECTION_H
