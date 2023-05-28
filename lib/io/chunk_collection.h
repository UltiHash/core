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

#define CHUNK_COLLECTION_BUFFER_SIZE 1 << 23


namespace uh::io {

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
        explicit chunk_collection(std::filesystem::path collection_location,bool create_tempfile = false):
                path(std::move(collection_location)),
                to_be_deleted(create_tempfile)
        {
            if (create_tempfile){
                auto file = io::temp_file(path, std::ios_base::out);
                file.release_to(file.path());
                path = file.path();
            }

            auto temporarily_open_file = io::file(path, std::ios_base::in);

            auto temporarily_cached_fragment_on_seekable_device =
                    io::fragment_on_seekable_device(temporarily_open_file);

            if(std::filesystem::exists(path))
            {
                std::streamoff collection_offset{};
                uint16_t index_entry_count{};
                serialization::fragment_serialize_size_format skip_format;

                do{
                    skip_format = temporarily_cached_fragment_on_seekable_device.skip();

                    if(skip_format.content_size == 0)
                        break;

                    if(index_entry_count > std::numeric_limits<unsigned char>::max()+1)
                        THROW(util::exception,"Indexing of chunk collection "+path.string()+" exceeded limits");

                    index.emplace_back(skip_format,collection_offset);
                    collection_offset += skip_format.header_size + skip_format.content_size;
                    index_entry_count++;

                } while (skip_format.content_size > 0);
            }
        }

        /**
         * read a certain index pointer and return a vector buffer of the content
         *
         * @param at index
         * @return buffer
         */
        std::pair<std::vector<char>,serialization::fragment_serialize_size_format>
        read_indexed(uint8_t at)
        {
            auto temporarily_open_file = io::file(path, std::ios_base::in);

            auto fragment_pos_element = find_address(at);

            auto temporarily_cached_fragment_on_seekable_device =
                    io::fragment_on_seekable_device(temporarily_open_file);

            temporarily_open_file.seek((*fragment_pos_element).second,std::ios_base::beg);

            serialization::fragment_serialize_size_format read;
            std::vector<char> buffer(CHUNK_COLLECTION_BUFFER_SIZE);
            std::vector<char> output{};

            do{
                serialization::fragment_serialize_size_format temp_read =
                        temporarily_cached_fragment_on_seekable_device.read(buffer);

                std::size_t old_size = output.size();

                output.resize(output.size()+temp_read.content_size);
                std::memcpy(output.data()+old_size,buffer.data(),temp_read.content_size);

                read.header_size = std::max(read.header_size,temp_read.header_size);
                read.content_size += temp_read.content_size;
                read.index_num = read.index_num;

            } while (temporarily_cached_fragment_on_seekable_device.valid());

            return {output,read};
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

            auto temporarily_open_file = io::file(path, std::ios_base::app);
            auto temporarily_cached_fragment_on_seekable_device =
                    io::fragment_on_seekable_device(temporarily_open_file,
                                                                      next_free_address());

            uint32_t allocate_space = std::max(static_cast<uint32_t>(buffer.size()),alloc);
            serialization::fragment_serialize_size_format written =
                    temporarily_cached_fragment_on_seekable_device.write(buffer,allocate_space);

            if(index.empty())
                index.emplace_back(written,0);
            else
                index.emplace_back(written,
                                   index.back().second+
                                      index.back().first.header_size+
                                      index.back().first.content_size);

            return written;
        }

        /**
         * read indexed multiple positions with smart seeking
         */
        std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format>>
        read_indexed_multi(const std::vector<uint8_t>& at)
        {
            for(const auto item:at){
                if(std::count(at.cbegin(),at.cend(),item) > 1){
                    THROW(util::exception,"Read indexed multi received dupliate read indexes. This is not allowed!");
                }
            }

            auto temporarily_open_file = io::file(path, std::ios_base::in);

            std::vector<uint8_t> index_num_list = get_index_num_content_list();
            std::vector<uint8_t> filtered_at_list_in_seek_order;

            for(const auto item: index_num_list){
                if(std::any_of(at.cbegin(),at.cend(),[item](const auto item2){
                    return item == item2;
                }))
                    filtered_at_list_in_seek_order.push_back(item);
            }

            std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format>>
                    out_list(filtered_at_list_in_seek_order.size());

            std::vector<char> buffer(CHUNK_COLLECTION_BUFFER_SIZE);

            for(const auto at_item:filtered_at_list_in_seek_order){
                auto fragment_pos_element = find_address(at_item);

                auto temporarily_cached_fragment_on_seekable_device =
                        io::fragment_on_seekable_device(temporarily_open_file);

                temporarily_open_file.seek(fragment_pos_element->second,std::ios_base::beg);

                serialization::fragment_serialize_size_format read;
                std::vector<char> output{};

                do{
                    serialization::fragment_serialize_size_format temp_read =
                            temporarily_cached_fragment_on_seekable_device.read(buffer);

                    std::size_t old_size = output.size();

                    output.resize(output.size()+temp_read.content_size);
                    std::memcpy(output.data()+old_size,buffer.data(),temp_read.content_size);

                    read.header_size = std::max(read.header_size,temp_read.header_size);
                    read.content_size += temp_read.content_size;
                    read.index_num = temp_read.index_num;

                } while (temporarily_cached_fragment_on_seekable_device.valid());

                std::streamoff distance_filtered_projected_to_at =
                        std::distance(at.begin(),std::find(at.begin(),at.end(),at_item));

                out_list[distance_filtered_projected_to_at] = std::make_pair(output,read);
            }

            return out_list;
        }

        /**
         * Write indexed multiple buffers and return a list of fragment size structs that contain written fragment
         * information.
         * Does not close filestream.
         */
        std::vector<serialization::fragment_serialize_size_format>
        write_indexed_multi(const std::vector<std::span<const char>> &buffer)
        {

            if(static_cast<long>(free()) - buffer.size() < 0)
                THROW(util::exception,"On chunk collection " + path.string() +
                                      "was no space left to multi write indexed!");

            std::for_each(buffer.cbegin(),buffer.cend(),[](const auto& item)
            {
                if(item.size() > std::numeric_limits<uint32_t>::max())
                    THROW(util::exception,"Incoming writing buffer was too large!");
            });

            auto temporarily_open_file = io::file(path, std::ios_base::app);

            std::vector<serialization::fragment_serialize_size_format> out_list{};

            for(const auto& item:buffer)
            {
                auto temporarily_cached_fragment_on_seekable_device =
                        io::fragment_on_seekable_device(temporarily_open_file,next_free_address());

                out_list.push_back(temporarily_cached_fragment_on_seekable_device.write(item));

                if(index.empty())
                    index.emplace_back(out_list.back(),0);
                else
                    index.emplace_back(out_list.back(),
                                       index.back().second+
                                          index.back().first.header_size+
                                          index.back().first.content_size);
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
            return index.size() == std::numeric_limits<unsigned char>::max()+1;
        }

        /**
         *
         * @return tell how many addresses are still free
         */
        uint8_t free() {
            return std::numeric_limits<uint8_t>::max()+1 - count();
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
            for(unsigned short i = 0; i < std::numeric_limits<unsigned char>::max()+1;i++)
            {
                if(index_beg == std::end(copy_index) || index_beg->first.index_num < i)
                {
                    return i;
                }
                else
                    index_beg++;
            }

            return 0;
        }

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

        bool to_be_deleted;
        std::filesystem::path path;
        std::vector<std::pair<serialization::fragment_serialize_size_format,std::streamoff>> index;
    };

} // namespace uh::io

#endif //CORE_CHUNK_COLLECTION_H
