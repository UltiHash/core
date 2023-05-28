//
// Created by benjamin-elias on 28.05.23.
//

#include "chunk_collection.h"

namespace uh::io{

    chunk_collection::~chunk_collection() {
        if(to_be_deleted){
            std::filesystem::remove(getPath());
        }
    }

    chunk_collection::chunk_collection(std::filesystem::path collection_location, bool create_tempfile) :
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

    serialization::fragment_serialize_size_format
    chunk_collection::write_indexed(std::span<const char> buffer, uint32_t alloc) {
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

    std::pair<std::vector<char>, serialization::fragment_serialize_size_format>
    chunk_collection::read_indexed(uint8_t at) {
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

    std::vector<serialization::fragment_serialize_size_format>
    chunk_collection::write_indexed_multi(const std::vector<std::span<const char>> &buffer) {

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

    std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format>>
    chunk_collection::read_indexed_multi(const std::vector<uint8_t> &at) {
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

    uint16_t chunk_collection::count() {
        return static_cast<uint16_t>(index.size());
    }

    std::size_t chunk_collection::size() {
        std::size_t accumulated{};

        for(const auto& item:index)
        {
            accumulated += item.first.header_size + item.first.content_size;
        }

        return accumulated;
    }

    std::size_t chunk_collection::size(uint8_t index_adress) {
        auto found_address = find_address(index_adress);

        return found_address->first.header_size + found_address->first.content_size;
    }

    std::size_t chunk_collection::content_size(uint8_t index_adress) {
        auto found_address = find_address(index_adress);

        return found_address->first.content_size;
    }

    bool chunk_collection::full() const {
        return index.size() == std::numeric_limits<unsigned char>::max()+1;
    }

    uint8_t chunk_collection::free() {
        return std::numeric_limits<uint8_t>::max()+1 - count();
    }

    std::filesystem::path chunk_collection::getPath() {
        return path;
    }

    void chunk_collection::release_to(const std::filesystem::path &release_path) {
        to_be_deleted = false;

        if(release_path == getPath())
            return;

        if (::link(getPath().c_str(), release_path.c_str()) == -1)
        {
            THROW_FROM_ERRNO();
        }

        path = release_path;
    }

    uint8_t chunk_collection::next_free_address() {
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
}