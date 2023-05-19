//
// Created by benjamin-elias on 18.05.23.
//

#include "chunk_collection.h"

#include <io/fragment_seekable_device.h>
#include <io/buffer.h>
#include <util/exception.h>
#include <serialization/serialization.h>

#include <utility>
#include <limits>
#include <algorithm>
#include <numeric>

namespace uh::io {

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)
    chunk_collection<SeekableDevice>::chunk_collection(std::filesystem::path collection_path):
    path(std::move(collection_path))
    {
        if constexpr (std::is_same_v<SeekableDevice,file>){
            file f1(path,std::ios_base::in);
            fragment_seekable_device frag(f1);

            std::streamsize check_read{};
            std::vector<char> index_buf;
            index_buf.resize(1,0);

            do{
                check_read = io::read(f1,{index_buf.data(),index_buf.size()});
                if(check_read == 0)break;

                check_read += frag.skip();

                if(at_collection_index_entry_count != std::numeric_limits<unsigned char>::max())
                    at_collection_index_entry_count++;
                else
                    THROW(util::exception,"Indexing of chunk collection exceeded limits");

                auto start_pos = at_collection_offset_count;
                at_collection_offset_count += check_read;

                index.emplace_back(start_pos,
                                   at_collection_offset_count-start_pos,
                                   index_buf[0]);

            } while (check_read > 0);
        }
    }

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)std::streamsize
    chunk_collection<SeekableDevice>::write(std::span<const char> buffer)
    {
        return std::get<0>(write_indexed(buffer));
    }

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)std::streamsize
    chunk_collection<SeekableDevice>::read(std::span<char> buffer) {
        file f1(path,std::ios_base::in);
        char old_index_address[1];

        f1.read({old_index_address,1});

        fragment_seekable_device frag(f1);
        return frag.read(buffer);
    }

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)bool chunk_collection<SeekableDevice>::valid() const {
        return count() < std::numeric_limits<unsigned char>::max();
    }

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)std::vector<std::streamsize>
    chunk_collection<SeekableDevice>::write(std::vector<std::span<const char>> buffer) {
        std::vector<std::streamsize> sizes_out{};
        sizes_out.reserve(buffer.size());

        for(const auto&item:buffer){
            sizes_out.push_back(write(item));
        }

        return sizes_out;
    }

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)std::vector<std::streamsize>
    chunk_collection<SeekableDevice>::read(std::vector<std::span<char>> buffer) {
        std::vector<std::streamsize> sizes_out{};
        sizes_out.reserve(buffer.size());

        for(const auto&item:buffer){
            sizes_out.push_back(read(item));
        }

        return sizes_out;
    }

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)std::tuple<std::streamsize, uint8_t>
    chunk_collection<SeekableDevice>::write_indexed(std::span<const char> buffer) {
        std::streamsize write_out,write_total;

        char new_index_address[1];
        new_index_address[0] = next_free_address();

        file f1(path,std::ios_base::app);
        write_total = f1.write({new_index_address,1});

        fragment_seekable_device frag(f1);
        write_out = frag.write(buffer);
        write_total += write_out;

        io::buffer tmp_buf;
        write(tmp_buf,{buffer.data(),16});
        serialization::serialization ser(tmp_buf);
        write_total += std::get<0>(ser.get_data_size());

        auto start_pos = at_collection_offset_count;
        at_collection_offset_count += write_total;

        index.emplace_back(start_pos,
                           at_collection_offset_count-start_pos,
                           new_index_address[0]);

        return {write_out,new_index_address[0]};
    }

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)std::vector<char>
    chunk_collection<SeekableDevice>::read_indexed(uint8_t at)
    {
        file f1(path,std::ios_base::in);

        auto el = std::find(index.begin(),index.end(),[&at](const auto &item){
            return item.fragment_index_count_position == at;
        });

        if(el == index.end())
            THROW(util::exception,"Write indexed could not find address "+std::to_string(at)+
            " at path "+path.string());

        f1.seek(el->fragment_absolute_position,std::ios_base::beg);

        char old_index_address[1];
        std::vector<char> buffer{};
        buffer.resize(el->fragment_size);

        f1.read({old_index_address,1});

        fragment_seekable_device frag(f1);
        frag.read({buffer.data(),buffer.size()});

        return buffer;
    }

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)std::vector<std::tuple<std::streamsize, uint8_t>>
    chunk_collection<SeekableDevice>::write_indexed(const std::vector<std::span<const char>>& buffer) {
        std::vector<std::tuple<std::streamsize, uint8_t>> sizes_out{};
        sizes_out.reserve(buffer.size());

        for(const auto&item:buffer){
            sizes_out.push_back(write_indexed(item));
        }

        return sizes_out;
    }

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)std::vector<std::vector<char>>
    chunk_collection<SeekableDevice>::read_indexed(const std::vector<uint8_t>& at) {
        std::vector<std::vector<char>> results_out{};
        results_out.reserve(at.size());

        for(const auto&item:at){
            results_out.push_back(read_indexed(item));
        }

        return results_out;
    }

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)uint8_t chunk_collection<SeekableDevice>::count() {
        return index.size();
    }

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)std::size_t chunk_collection<SeekableDevice>::size() {
        return std::accumulate(index.begin(),index.end(),[](uint32_t acc,chunk_collection_entry &c){
            return acc + c.fragment_size;
        });
    }

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)std::size_t
    chunk_collection<SeekableDevice>::size(uint8_t index_adress) {

        auto el = std::find(index.begin(),index.end(),[&index_adress](const auto &item){
            return item.fragment_index_count_position == index_adress;
        });

        if(el == index.end())
            THROW(util::exception,"Index address was not found on size search!");

        return el->fragment_size;
    }

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)std::size_t
    chunk_collection<SeekableDevice>::size(std::size_t index_pos) {
        return index[index_pos].fragment_size;
    }

    template<class SeekableDevice>
    requires(std::is_base_of_v<seekable_device, SeekableDevice>)uint8_t
    chunk_collection<SeekableDevice>::next_free_address() {
        if(!valid())
            THROW(util::exception,"There are no more free addresses on chunk collection!");

        auto copy_index = index;

        std::sort(copy_index.begin(),copy_index.end(),[](const auto &a, const auto &b){
            return a.fragment_index_count_position < b.fragment_index_count_position;
        });

        auto index_beg = std::begin(copy_index);
        for(unsigned short i = 0; i < std::numeric_limits<unsigned char>::max();i++){
            if(index_beg == std::end(copy_index) || index_beg->fragment_index_count_position != i){
                return i;
            }
        }

        return 0;
    }

} // io