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

        /**
         * a chunk collection keeps track of the position and the movement
         * of incoming and outgoing chunks/fragments.
         * It only builds up a temporary connection to an underlying seekable_device.
         * On constructor the fragmented seekable device is scanned for the
         * position of its contents.
         *
         * @param fragment_file is the path to the managed fragmented file
         */
        explicit chunk_collection(std::filesystem::path  collection_path);

        /**
         * @param buffer is the data to be written or appended to the
         * chunk collection
         * @return is the size truly written to the underlying device
         */
        std::streamsize write(std::span<const char> buffer);

        /**
         * @param buffer to be read from the current unspecified position by
         * using the fragment implementation
         * @return the size of the fragment that was read with the help
         * of fragment implementation
         */
        virtual std::streamsize read(std::span<char> buffer);

        /**
         *
         * @return if the current position has not reached the end of
         * the underlying seekable device
         */
        [[nodiscard]] virtual bool valid() const;

        /**
         *
         * @param buffer a list of buffers to be written to the chunk collection
         * @return stream sizes to determine if writing was completed (0 for not completed)
         */
        std::vector<std::streamsize> write(std::vector<std::span<const char>> buffer);

        /**
         *
         * @param buffer to read multiple contents at once from the current position
         * @return stream sizes to determine if reading was completed and correct
         */
        std::vector<std::streamsize> read(std::vector<std::span<char>> buffer);

        /**
         * write with returning the index that was assigned to the written buffer
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
        std::vector<std::tuple<std::streamsize,uint8_t>> write_indexed(std::vector<std::span<const char>> buffer);

        /**
         * read indexed multiple positions
         */
        std::vector<std::vector<char>> read_indexed(std::vector<uint8_t> at);

    private:
        std::filesystem::path path;
        std::vector<chunk_collection_entry> index;
        uint8_t at_collection_index_entry_count{};
        uint32_t at_collection_offset_count{};
    };

} // namespace uh::io

#endif //CORE_CHUNK_COLLECTION_H
