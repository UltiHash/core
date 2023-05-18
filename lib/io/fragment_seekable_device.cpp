//
// Created by benjamin-elias on 18.05.23.
//

#include "fragment_seekable_device.h"

#include <serialization/serialization.h>
#include <util/exception.h>

namespace uh::io {

    fragment_seekable_device::fragment_seekable_device(seekable_device &device):
            dev_(device), fragment_device(device) {}

    // ---------------------------------------------------------------------

    std::streamsize fragment_seekable_device::skip() {
        std::streamsize accumulate_read{};
        std::streamoff buffer_size{};

        try{
            std::streamoff header_elements{};

            if(getStateMachine() == UNDEFINED_STATE || getStateMachine() == READING_COMPLETE){
                auto ser = serialization::serialization(dev_);
                auto data_size = ser.get_data_size();
                setStateMachine(READING_BEGIN);

                header_elements = static_cast<std::streamoff>(std::get<0>(data_size));
                setElementsLeftToRead(static_cast<std::streamoff>(std::get<1>(data_size)));
            }

            accumulate_read += header_elements;
            dev_.seek(getElementsLeftToRead(),std::ios_base::cur);
            accumulate_read += getElementsLeftToRead();

            setStateMachine(READING_COMPLETE);
            setElementsLeftToRead(0);
        }
        catch(std::exception &e){
            THROW(util::exception,"fragment_seekable_device serialization failed on skip! "
                                  "Buffer size was "+std::to_string(buffer_size)+
                                  " The fragment_seekable_device state was "+std::to_string(getStateMachine())+
                                  " and the error code was: "+e.what());
        }
        return accumulate_read;
    }

} // namespace uh::io