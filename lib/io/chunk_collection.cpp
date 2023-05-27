//
// Created by benjamin-elias on 27.05.23.
//

#include "chunk_collection.h"

#include "io/file.h"
#include "io/fragment_on_seekable_device.h"
#include "io/fragment_on_seekable_reset_device.h"
#include "serialization/fragment_size_struct.h"

#include <utility>
#include <filesystem>
#include <algorithm>
#include <array>
#include <numeric>

namespace uh::io {

    // ---------------------------------------------------------------------

    chunk_collection::chunk_collection(std::filesystem::path collection_location):
    path(std::move(collection_location)),
    temporarily_open_file(std::make_unique<io::file>(path, std::ios_base::in)),
    temporarily_cached_fragment_on_seekable_device(
            std::make_unique<io::fragment_on_seekable_device>(*temporarily_open_file))
    {
        if(std::filesystem::exists(path))
        {
            std::streamoff collection_offset{};
            uint16_t index_entry_count{};
            serialization::fragment_serialize_size_format skip_format(0,0,0);

            do{
                skip_format = temporarily_cached_fragment_on_seekable_device->skip();

                if(skip_format.content_size == 0)
                    break;

                if(index_entry_count >= std::numeric_limits<unsigned char>::max())
                    THROW(util::exception,"Indexing of chunk collection "+path.string()+" exceeded limits");

                index.emplace_back(skip_format,collection_offset);
                index_entry_count++;

            } while (skip_format.content_size > 0);

            temporarily_open_file->close();
        }
    }

    // ---------------------------------------------------------------------

    serialization::fragment_serialize_size_format chunk_collection::write_indexed
    (std::span<const char> buffer,uint32_t alloc)
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

        temporarily_open_file->close();

        return written;
    }

    // ---------------------------------------------------------------------

    std::pair<std::vector<char>, serialization::fragment_serialize_size_format>
    chunk_collection::read_indexed(uint8_t at)
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

        temporarily_open_file->close();

        return {output,read};
    }

    // ---------------------------------------------------------------------

    std::vector<serialization::fragment_serialize_size_format>
    chunk_collection::write_indexed(const std::vector<std::span<const char>> &buffer)
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

        temporarily_open_file->close();

        return out_list;
    }

    // ---------------------------------------------------------------------

    std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format>>
    chunk_collection::read_indexed(const std::vector<uint8_t>& at)
    {
        //TODO: order read with the help of fragment position and seek into one direction
    }

    // ---------------------------------------------------------------------

    uint16_t chunk_collection::count()
    {
        return static_cast<uint16_t>(index.size());
    }

    // ---------------------------------------------------------------------

    std::size_t chunk_collection::size()
    {
        std::size_t accumulated{};

        for(const auto& item:index)
        {
            accumulated += item.first.header_size + item.first.content_size;
        }

        return accumulated;
    }

    // ---------------------------------------------------------------------

    std::size_t chunk_collection::size(uint8_t index_adress)
    {
        auto found_address = find_address(index_adress);

        return found_address->first.header_size + found_address->first.content_size;
    }

    // ---------------------------------------------------------------------

    std::size_t chunk_collection::content_size(uint8_t index_adress) {
        auto found_address = find_address(index_adress);

        return found_address->first.content_size;
    }

    // ---------------------------------------------------------------------

    bool chunk_collection::full() const
    {
        return index.size() == std::numeric_limits<unsigned char>::max();
    }

    // ---------------------------------------------------------------------

    uint8_t chunk_collection::free() {
        return std::numeric_limits<uint8_t>::max() - count();
    }

    // ---------------------------------------------------------------------

    std::filesystem::path chunk_collection::getPath()
    {
        return path;
    }

    // ---------------------------------------------------------------------

    uint8_t chunk_collection::next_free_address()
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

    // ---------------------------------------------------------------------

    std::vector<std::pair<serialization::fragment_serialize_size_format, std::streamoff>>::const_iterator
    chunk_collection::find_address(uint8_t at) {
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

    // ---------------------------------------------------------------------

} // namespace uh::io