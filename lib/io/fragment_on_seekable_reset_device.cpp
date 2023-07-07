//
// Created by benjamin-elias on 27.05.23.
//

#include "fragment_on_seekable_reset_device.h"

namespace uh::io
{

// ---------------------------------------------------------------------

fragment_on_seekable_reset_device::fragment_on_seekable_reset_device(seekable_device& dev, uint8_t index,
                                                                     std::streamoff starts_at)
    :
    start_pos(starts_at), fragment_on_seekable_device(dev, index)
{}

// ---------------------------------------------------------------------

bool fragment_on_seekable_reset_device::reset()
{
    dev_seek.seek(start_pos, std::ios_base::beg);
    return fragment_on_device::reset();
}

} // namespace uh::io