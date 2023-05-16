//
// Created by benjamin-elias on 15.05.23.
//

#include "io/device.h"
#include "io/buffer.h"

#include "serialization/serialization.h"

#include <openssl/evp.h>

#ifndef CORE_FRAGMENT_H
#define CORE_FRAGMENT_H

namespace uh::io{

    template<typename DeviceType> requires std::is_base_of_v<io::device, DeviceType>
    class fragment : public io::device{

    public:
        /**
         * a fragment uses a device to either only read or only write in its lifetime
         * after reading or writing the fragment can tell where to find its content
         * relatively to the device stream.
         * A fragment is a serialized device.
         *
         * @param impl input device
         * @param start_pos relative start position on device to distinguish fragments
         */
        explicit fragment(DeviceType &dev,std::streamoff start_pos = 0):
        dev_(&dev), frag_beg_pos(start_pos), frag_read_pos(start_pos){}

        /**
         * read un-serialized input and write serialized to device
         *
         * @param buffer input
         * @return number of bytes that were persisted to device
         */

        std::streamsize write(std::span<const char> buffer) override{
            auto ser = serialization::serialization(dev_);
            auto header_of_buffer = ser.get_header(buffer.size());
            header_size = header_of_buffer.size();
            data_size = buffer.size();
            is_analyzed = true;

            return ser.write(buffer);
        }

        /**
         * read serialized device to unserialized buffer
         *
         * @param buffer to be read to from device
         * @return number of bytes totally read from device
         */
        std::streamsize read(std::span<char> buffer) override{
            //smaller steps on reading deduplication, make it possible to start with 1 byte buffer and update
            // the chunks up to be read on this behaviour
            if(!is_analyzed){
                auto ser = serialization::serialization(dev_);
                auto data_size_tup = ser.get_data_size();
                header_size = std::get<0>(data_size_tup);
                data_size = std::get<1>(data_size_tup);
                is_analyzed = true;
                frag_read_pos += header_size;
            }

            auto read_advance = io::read(dev_,{buffer.data(), buffer.size()});
            frag_read_pos += read_advance;

            return read_advance;
        }

        [[nodiscard]] bool valid() const override{
            return frag_read_pos < frag_beg_pos+header_size+data_size;
        }

        /**
         *
         * @return size of content only
         */
        std::size_t content_size(){
            return data_size;
        }

        /**
         *
         * @return size of entire fragment with control and size bytes
         */
        std::size_t total_size(){
            return data_size+header_size;
        }

        /**
         *
         * @return stored relative starting position on device
         */
        std::streamoff begin_pos(){
            return frag_beg_pos;
        }

        /**
         * this function tries to optimize speed by enabling the developer to allocate a large buffer
         * by knowing how big a buffer needs to be to read the rest of the fragment in a single attempt
         *
         * @return size of leftover fragment size
         */
        std::streamoff ready_to_read(){
            return std::max(0,total_size()-frag_read_pos);
        }

        /**
         * with this function all the content is read from the device blind to increase its stream
         * pointer to one byte after the fragment
         */
        void skip(){

        }

    private:
        DeviceType dev_;
        bool is_analyzed{};
        std::size_t data_size{};
        std::size_t header_size{};
        std::streamoff frag_beg_pos{};
        std::streamoff frag_read_pos{};
    };
} // namespace uh::io

#endif //CORE_FRAGMENT_H
