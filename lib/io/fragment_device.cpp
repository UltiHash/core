//
// Created by benjamin-elias on 18.05.23.
//
#include "fragment_device.h"

#include <serialization/serialization.h>
#include <util/exception.h>

namespace uh::io{

    // ---------------------------------------------------------------------

    fragment_device::fragment_device(io::device &dev) : dev_(dev){}

    // ---------------------------------------------------------------------

    std::streamsize fragment_device::write(std::span<const char> buffer)
    {
        if(state_machine == READING_BEGIN)
            THROW(util::exception,"Writing on fragment_device corrupted the fragments incomplete reading state!");

        auto ser = serialization::serialization(dev_);
        return ser.write(buffer);
    }

    // ---------------------------------------------------------------------

    std::streamsize fragment_device::read(std::span<char> buffer)
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
            THROW(util::exception,"fragment_device serialization failed on skip! "
                                  "Buffer size was "+std::to_string(buffer_size)+
                                  " The fragment_device state was "+std::to_string(state_machine)+
                                  " and the error code was: "+e.what());
        }
        return accumulate_read;
    }

    // ---------------------------------------------------------------------

    bool fragment_device::valid() const
    {
        return dev_.valid();
    }

    // ---------------------------------------------------------------------

    std::streamsize fragment_device::skip()
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
            THROW(util::exception,"fragment_device serialization failed on skip! "
                                  "Buffer size was "+std::to_string(buffer_size)+
                                  " The fragment_device state was "+std::to_string(state_machine)+
                                  " and the error code was: "+e.what());
        }
        return accumulate_read;
    }

    // ---------------------------------------------------------------------

    fragmented_states fragment_device::getStateMachine() const {
        return state_machine;
    }

    // ---------------------------------------------------------------------

    void fragment_device::setStateMachine(fragmented_states stateMachine) {
        state_machine = stateMachine;
    }

    // ---------------------------------------------------------------------

    std::streamoff fragment_device::getElementsLeftToRead() const {
        return elements_left_to_read;
    }

    // ---------------------------------------------------------------------

    void fragment_device::setElementsLeftToRead(std::streamoff elementsLeftToRead) {
        elements_left_to_read = elementsLeftToRead;
    }

    // ---------------------------------------------------------------------

} // namespace uh::io