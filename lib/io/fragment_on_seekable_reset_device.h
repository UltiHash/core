//
// Created by benjamin-elias on 27.05.23.
//

#ifndef CORE_FRAGMENT_ON_SEEKABLE_RESET_DEVICE_H
#define CORE_FRAGMENT_ON_SEEKABLE_RESET_DEVICE_H

#include "io/fragment_on_seekable_device.h"

namespace uh::io
{

class fragment_on_seekable_reset_device: public fragment_on_seekable_device
{

public:

    /**
     * fragment on seekable reset device is a virtual device running on a seekable host device
     *
     * @param dev incoming seekable device to build this virtual device upon
     * @param index position that is either given on write or read from underlying device
     * @param starts_at fragment starting position to be seeked to
     */
    explicit fragment_on_seekable_reset_device(io::seekable_device& dev, uint8_t index = 0,
                                               std::streamoff starts_at = 0);

    /**
     * reset the state machine to the stored fragment beginning
     * @return if the underlying device is still valid to deliver the fragment structure of the stored position
     */
    bool reset() override;

private:
    std::streamoff start_pos;
};

} // namespace uh::io

#endif //CORE_FRAGMENT_ON_SEEKABLE_RESET_DEVICE_H
