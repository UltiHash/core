//
// Created by benjamin-elias on 26.05.23.
//

#ifndef CORE_FRAGMENT_ON_SEEKABLE_DEVICE_H
#define CORE_FRAGMENT_ON_SEEKABLE_DEVICE_H

#include "fragment_on_device.h"

#include "io/fragmented_device.h"
#include "io/seekable_device.h"

namespace uh::io{

class fragment_on_seekable_device: public fragment_on_device{

public:
    /**
     * fragment on seekable device will not read the device on skip, but seek over the content
     *
     * @param dev seekable input device
     * @param index of fragment to be written, if that is intended
     */
    explicit fragment_on_seekable_device(io::seekable_device& dev,uint8_t index = 0);

    /**
     * with this function the underlying device is read on its header and then seeked over the content until
     * one position behind the fragment_on_device content
     *
     * @return struct{header size, content size, index}
     */
    uh::serialization::fragment_serialize_size_format skip() override;

protected:
    io::seekable_device& dev2_;
};

} // namespace uh::io

#endif //CORE_FRAGMENT_ON_SEEKABLE_DEVICE_H
