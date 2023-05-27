//
// Created by benjamin-elias on 27.05.23.
//

#ifndef CORE_CHUNK_COLLECTION_H
#define CORE_CHUNK_COLLECTION_H

#include "serialization/fragment_size_struct.h"
#include "io/file.h"
#include "io/temp_file.h"
#include "io/fragment_on_seekable_device.h"
#include "io/fragment_on_seekable_reset_device.h"
#include "serialization/fragment_size_struct.h"

#include <utility>
#include <filesystem>
#include <algorithm>
#include <array>
#include <numeric>
#include <vector>
#include <list>
#include <span>


namespace uh::io {

    template<class SEEKABLE_TYPE = io::file>
    class chunk_collection {

    public:

        ~chunk_collection() {
            if(to_be_deleted){
                std::filesystem::remove(getPath());
            }
        }

        /**
         * a chunk collection keeps track of the position and the movement
         * of incoming and outgoing chunks/fragments.
         * It only builds up a temporary connection to an underlying seekable_device.
         * On constructor the fragmented seekable device is scanned for the
         * position of its contents.
         *
         * @param collection_location where the file containing the chunk collection is located
         */
        explicit chunk_collection(std::filesystem::path collection_location):
                path(std::move(collection_location)),
                temporarily_open_file(std::make_unique<SEEKABLE_TYPE>(path, std::ios_base::in)),
                temporarily_cached_fragment_on_seekable_device(
                        std::make_unique<io::fragment_on_seekable_device>(*temporarily_open_file)),
                to_be_deleted(std::is_same_v<SEEKABLE_TYPE,io::temp_file>)
        {
            if(std::filesystem::exists(path))
            {
                std::streamoff collection_offset{};
                uint16_t index_entry_count{};
                serialization::fragment_serialize_size_format skip_format;

                do{
                    skip_format = temporarily_cached_fragment_on_seekable_device->skip();

                    if(skip_format.content_size == 0)
                        break;

                    if(index_entry_count >= std::numeric_limits<unsigned char>::max())
                        THROW(util::exception,"Indexing of chunk collection "+path.string()+" exceeded limits");

                    index.emplace_back(skip_format,collection_offset);
                    index_entry_count++;

                } while (skip_format.content_size > 0);
            }

            if constexpr (std::is_same_v<SEEKABLE_TYPE,io::temp_file>){
                temporarily_open_file->release_to(temporarily_open_file->path());
                path = temporarily_open_file->path();
            }

        }

        /**
         * write with returning the index that was assigned to the written buffer
         * or instead give an index
         *
         * @param buffer to be written
         * @param alloc allocate space and write fragment to chunk collection with the help of multiple buffers
         * WARNING: allocated space must be written completely (accumulated buffer size is longer or equal alloc)
         * @return tuple<stream size, assigned index position>
         */
        serialization::fragment_serialize_size_format write_indexed
                (std::span<const char> buffer,uint32_t alloc = 0)
        {
            if(!free())
                THROW(util::exception,"On chunk collection " + path.string() +
                                      "was no space left to multi write indexed!");

            if(buffer.size() > std::numeric_limits<uint32_t>::max())
                THROW(util::exception,"Incoming writing buffer was too large!");

            if(!temporarily_cached_fragment_on_seekable_device->valid()){
                temporarily_open_file = std::make_unique<io::file>(path, std::ios_base::app);
                temporarily_cached_fragment_on_seekable_device =
                        std::make_unique<io::fragment_on_seekable_device>(*temporarily_open_file,
                                                                          next_free_address());
            }

            uint32_t allocate_space = std::max(static_cast<uint32_t>(buffer.size()),alloc);
            serialization::fragment_serialize_size_format written =
                    temporarily_cached_fragment_on_seekable_device->write(buffer,allocate_space);

            return written;
        }

        /**
         * read a certain index pointer and return a vector buffer of the content
         *
         * @param at index
         * @return buffer
         */
        std::pair<std::vector<char>,serialization::fragment_serialize_size_format> read_indexed(uint8_t at)
        {
            if(!temporarily_cached_fragment_on_seekable_device->valid()){
                temporarily_open_file = std::make_unique<io::file>(path, std::ios_base::in);

                auto fragment_pos_element = find_address(at);

                temporarily_cached_fragment_on_seekable_device =
                        std::make_unique<io::fragment_on_seekable_reset_device>(*temporarily_open_file,
                                                                                0,
                                                                                fragment_pos_element->second);

                temporarily_cached_fragment_on_seekable_device.reset();
            }

            serialization::fragment_serialize_size_format read(0,0,0);
            std::array<char,buf_size> buffer{};
            std::vector<char> output{};

            do{
                serialization::fragment_serialize_size_format temp_read =
                        temporarily_cached_fragment_on_seekable_device->read(buffer);

                std::size_t old_size = output.size();

                output.resize(output.size()+temp_read.content_size);
                std::memcpy(output.data()+old_size,buffer.data(),temp_read.content_size);

                read.header_size = std::max(read.header_size,temp_read.header_size);
                read.content_size += temp_read.content_size;
                read.index_num = read.index_num;

            } while (temporarily_cached_fragment_on_seekable_device->valid());

            return {output,read};
        }

        /**
         * Write indexed multiple buffers and return a list of fragment size structs that contain written fragment
         * information.
         * Does not close filestream.
         */
        std::vector<serialization::fragment_serialize_size_format>
        write_indexed_multi(const std::vector<std::span<const char>> &buffer)
        {

            if(static_cast<long>(free()) - buffer.size() <= 0)
                THROW(util::exception,"On chunk collection " + path.string() +
                                      "was no space left to multi write indexed!");

            std::for_each(buffer.cbegin(),buffer.cend(),[](const auto& item)
            {
                if(item.size() > std::numeric_limits<uint32_t>::max())
                    THROW(util::exception,"Incoming writing buffer was too large!");
            });

            if(!temporarily_cached_fragment_on_seekable_device->valid()){
                temporarily_open_file = std::make_unique<io::file>(path, std::ios_base::app);
            }

            std::vector<serialization::fragment_serialize_size_format> out_list{};

            for(const auto& item:buffer)
            {
                temporarily_cached_fragment_on_seekable_device =
                        std::make_unique<io::fragment_on_seekable_device>(*temporarily_open_file,
                                                                          next_free_address());

                out_list.push_back(temporarily_cached_fragment_on_seekable_device->write(item));
            }

            return out_list;
        }

        /**
         * read indexed multiple positions with smart seeking
         */
        std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format>>
        read_indexed_multi(const std::vector<uint8_t>& at)
        {
            if(!temporarily_cached_fragment_on_seekable_device->valid()){
                temporarily_open_file = std::make_unique<io::file>(path, std::ios_base::in);
            }

            std::vector<uint8_t> index_num_list = get_index_num_content_list();
            std::vector<uint8_t> filtered_at_list_in_seek_order;

            std::copy_if(index_num_list.cbegin(),index_num_list.cend(),filtered_at_list_in_seek_order.begin(),
                         [&at](const auto item)
                         {
                             return std::any_of(at.cbegin(),at.cend(),[&item](const auto item2){
                                 return item == item2;
                             });
                         });

            std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format>>
                    out_list(filtered_at_list_in_seek_order.size());

            for(const auto at_item:filtered_at_list_in_seek_order){
                auto fragment_pos_element = find_address(at_item);

                temporarily_cached_fragment_on_seekable_device =
                        std::make_unique<io::fragment_on_seekable_reset_device>(*temporarily_open_file,
                                                                                0,
                                                                                fragment_pos_element->second);

                temporarily_cached_fragment_on_seekable_device.reset();

                serialization::fragment_serialize_size_format read;
                std::array<char,buf_size> buffer{};
                std::vector<char> output{};

                do{
                    serialization::fragment_serialize_size_format temp_read =
                            temporarily_cached_fragment_on_seekable_device->read(buffer);

                    std::size_t old_size = output.size();

                    output.resize(output.size()+temp_read.content_size);
                    std::memcpy(output.data()+old_size,buffer.data(),temp_read.content_size);

                    read.header_size = std::max(read.header_size,temp_read.header_size);
                    read.content_size += temp_read.content_size;
                    read.index_num = read.index_num;

                } while (temporarily_cached_fragment_on_seekable_device->valid());

                out_list[
                        std::distance(at.begin(),
                                      std::find(at.begin(),at.end(),at_item)
                        )
                ] = std::make_pair(output,read);
            }

            return out_list;
        }

        /**
         *
         * @return the count of addresses used
         */
        uint16_t count()
        {
            return static_cast<uint16_t>(index.size());
        }

        /**
         *
         * @return the accumulated size of the chunk collection together with number of indexes used and further
         * header- and content size bytes
         */
        std::size_t size()
        {
            std::size_t accumulated{};

            for(const auto& item:index)
            {
                accumulated += item.first.header_size + item.first.content_size;
            }

            return accumulated;
        }

        /**
         *
         * @param index_adress is the address of a registered fragment/chunk
         * @return the size of the header and content payload of the fragment/chunk
         */
        std::size_t size(uint8_t index_adress)
        {
            auto found_address = find_address(index_adress);

            return found_address->first.header_size + found_address->first.content_size;
        }

        /**
         *
         * @param index_adress is the address of a registered fragment/chunk
         * @return the size of the content payload of the fragment/chunk
         */
        std::size_t content_size(uint8_t index_adress) {
            auto found_address = find_address(index_adress);

            return found_address->first.content_size;
        }

        /**
         *
         * @return if the chunk collection is full
         */
        [[nodiscard]] bool full() const
        {
            return index.size() == std::numeric_limits<unsigned char>::max();
        }

        uint8_t free() {
            return std::numeric_limits<uint8_t>::max() - count();
        }

        /**
         *
         * @return path of chunk collection
         */
        std::filesystem::path getPath()
        {
            return path;
        }

        /**
         * Rename the temp_file to `path` and make it a permanent file, in case we are based on temp_file
         *
         * @throw a file with the given name already exists
         */
        void release_to(const std::filesystem::path& release_path){
            to_be_deleted = false;

            if(release_path == getPath())
                return;

            if (::link(getPath().c_str(), release_path.c_str()) == -1)
            {
                THROW_FROM_ERRNO();
            }

            path = release_path;
        }

        std::vector<uint8_t> get_index_num_content_list() {
            std::vector<uint8_t> out_list(index.size());

            std::size_t counter{};

            for(const auto& item:index){
                out_list[counter] = item.first.index_num;
                counter++;
            }

            return out_list;
        }

    private:
        std::filesystem::path path;
        std::vector<std::pair<serialization::fragment_serialize_size_format,std::streamoff>> index;

        std::unique_ptr<io::seekable_device> temporarily_open_file;
        std::unique_ptr<io::fragmented_device> temporarily_cached_fragment_on_seekable_device;

        uint8_t next_free_address()
        {
            if(full())
                THROW(util::exception,"There are no more free addresses on chunk collection "+path.string()+" !");

            auto copy_index = index;

            std::sort(copy_index.begin(),copy_index.end(),[](const auto &a, const auto &b)
            {
                return a.first.index_num < b.first.index_num;
            });

            auto index_beg = std::begin(copy_index);
            for(unsigned short i = 0; i < std::numeric_limits<unsigned char>::max();i++)
            {
                if(index_beg == std::end(copy_index) || index_beg->first.index_num != i)
                {
                    return i;
                }
            }

            return 0;
        }

        bool to_be_deleted;

        std::vector<std::pair<serialization::fragment_serialize_size_format,std::streamoff>>::const_iterator
        find_address(uint8_t at) {
            auto fragment_pos_element = std::find_if(index.cbegin(),index.cend(),
                                                     [&at](const std::pair<
                                                             serialization::fragment_serialize_size_format,
                                                             std::streamoff
                                                     >& elem)
                                                     {
                                                         return elem.first.index_num == at;
                                                     });

            if(fragment_pos_element == index.cend())
                THROW(util::exception,"Fragment " + std::to_string(at)+
                                      " was not found on chunk collection " + path.string());

            return fragment_pos_element;
        }

        constexpr static std::size_t buf_size = 1 << 26;
    };

} // namespace uh::io

#endif //CORE_CHUNK_COLLECTION_H
