//
// Created by benjamin-elias on 18.05.23.
//
#include "fragment_on_device.h"

#include <util/exception.h>
#include <serialization/fragment_serialization.h>

namespace uh::io{

    // ---------------------------------------------------------------------

    fragment_on_device::fragment_on_device(io::device &dev) : dev_(dev){}

    // ---------------------------------------------------------------------

    std::streamsize fragment_on_device::write(std::span<const char> buffer)
    {
        if(state_machine == READING_BEGIN)
            THROW(util::exception,"Writing on fragment_on_device corrupted the fragments incomplete reading state!");

        auto ser = serialization::fragment_serialization(dev_);


        return ser.write(buffer);
    }

    // ---------------------------------------------------------------------

    std::streamsize fragment_on_device::read(std::span<char> buffer)
    {
        std::streamsize accumulate_read{};
        std::streamoff buffer_size{};

        try{
            if(state_machine == UNDEFINED_STATE || state_machine == READING_COMPLETE){
                auto ser = serialization::serialization(dev_);
                auto data_size = ser.get_data_size();
                state_machine = READING_BEGIN;

                elements_left_to_read = static_cast<std::streamoff>(std::get<1>(data_size));
            }

            buffer_size = static_cast<std::streamoff>(buffer.size());
            std::size_t stream_advance = std::min(static_cast<std::size_t>(buffer_size),
                                                  static_cast<std::size_t>(elements_left_to_read));

            accumulate_read += io::read(dev_, {buffer.data(),stream_advance});

            elements_left_to_read -= static_cast<std::streamoff>(stream_advance);
            if(!elements_left_to_read)state_machine = READING_COMPLETE;
        }
        catch(std::exception &e){
            THROW(util::exception,"fragment_on_device serialization failed on skip! "
                                  "Buffer size was "+std::to_string(buffer_size)+
                                  " The fragment_on_device state was "+std::to_string(state_machine)+
                                  " and the error code was: "+e.what());
        }
        return accumulate_read;
    }

    // ---------------------------------------------------------------------

    bool fragment_on_device::valid() const
    {
        return dev_.valid();
    }

    // ---------------------------------------------------------------------

    std::streamsize fragment_on_device::skip()
    {
        std::streamsize accumulate_read{};
        std::streamoff buffer_size{};

        try{
            std::streamoff header_elements{};
            std::vector<char>tmp{};

            if(state_machine == UNDEFINED_STATE || state_machine == READING_COMPLETE){
                auto ser = serialization::serialization(dev_);
                auto data_size = ser.get_data_size();
                state_machine = READING_BEGIN;

                header_elements = static_cast<std::streamoff>(std::get<0>(data_size));
                elements_left_to_read = static_cast<std::streamoff>(std::get<1>(data_size));
            }

            tmp.resize(elements_left_to_read,0);
            buffer_size = static_cast<std::streamoff>(tmp.size());

            accumulate_read += header_elements;
            accumulate_read += io::read(dev_, {tmp.data(), static_cast<std::size_t>(elements_left_to_read)});

            state_machine = READING_COMPLETE;
            elements_left_to_read = 0;
        }
        catch(std::exception &e){
            THROW(util::exception,"fragment_on_device serialization failed on skip! "
                                  "Buffer size was "+std::to_string(buffer_size)+
                                  " The fragment_on_device state was "+std::to_string(state_machine)+
                                  " and the error code was: "+e.what());
        }
        return accumulate_read;
    }

    // ---------------------------------------------------------------------

} // namespace uh::io