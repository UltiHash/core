//
// Created by benjamin-elias on 18.05.23.
//
#include "fragment.h"

#include <util/exception.h>

namespace uh::io{

    // ---------------------------------------------------------------------

    fragment::fragment(io::device &dev) : dev_(dev){}

    // ---------------------------------------------------------------------

    std::streamsize fragment::write(std::span<const char> buffer)
    {
        if(state_machine == READING_BEGIN)
            THROW(util::exception,"Writing on fragment corrupted its incomplete reading state!");
        state_machine = WRITING_MODE;
        auto ser = serialization::serialization(dev_);
        return ser.write(buffer);
    }

    // ---------------------------------------------------------------------

    std::streamsize fragment::read(std::span<char> buffer)
    {
        if(state_machine == WRITING_MODE)
            THROW(util::exception,"The fragment had already written contents to device, please use a new fragment!");

        auto ser = serialization::serialization(dev_);
        std::streamsize accumulate_read{};

        try{
            auto data_size = ser.get_data_size();
            accumulate_read += io::read(dev_, {buffer.data (), std::get<1>(data_size)});
        }
        catch(std::exception &e){
            THROW(util::exception,"fragment serialization failed on read! "
                                  "Buffer size was "+std::to_string(buffer.size())+
                                  " The fragment state was "+std::to_string(state_machine)+
                                  " and the error code was: "+e.what());
        }
        return accumulate_read;
    }

    // ---------------------------------------------------------------------

    bool fragment::valid() const
    {
        return dev_.valid();
    }

    // ---------------------------------------------------------------------

    std::streamsize fragment::skip()
    {
        if(state_machine == WRITING_MODE)
            THROW(util::exception,"The fragment had already written contents to device, please use a new fragment!");

        std::streamsize accumulate_read{};
        std::streamoff buffer_size{};

        try{
            std::streamoff header_elements{};
            std::vector<char>tmp{};

            if(state_machine == UNDEFINED_STATE || state_machine == READING_COMPLETE){
                auto ser = serialization::serialization(dev_);
                auto data_size = ser.get_data_size();
                state_machine = READING_BEGIN;

                header_elements = std::get<0>(data_size);
                elements_left_to_read = std::get<1>(data_size);
            }

            tmp.resize(elements_left_to_read,0);
            buffer_size = tmp.size();

            accumulate_read += header_elements;
            accumulate_read += io::read(dev_, {tmp.data (), static_cast<std::size_t>(elements_left_to_read)});

            state_machine = READING_COMPLETE;
            elements_left_to_read = 0;
        }
        catch(std::exception &e){
            THROW(util::exception,"fragment serialization failed on skip! "
                                  "Buffer size was "+std::to_string(buffer_size)+
                                  " The fragment state was "+std::to_string(state_machine)+
                                  " and the error code was: "+e.what());
        }
        return accumulate_read;
    }

    // ---------------------------------------------------------------------

} // namespace uh::io