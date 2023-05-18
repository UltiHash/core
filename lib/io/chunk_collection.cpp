//
// Created by benjamin-elias on 18.05.23.
//

#include "chunk_collection.h"

#include <io/fragment_seekable_device.h>
#include <util/exception.h>

#include <utility>
#include <limits>

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



} // io