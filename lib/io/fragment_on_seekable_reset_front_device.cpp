//
// Created by benjamin-elias on 27.05.23.
//

#include "fragment_on_seekable_reset_front_device.h"

namespace uh::io
{

// ---------------------------------------------------------------------

fragment_on_seekable_reset_front_device::fragment_on_seekable_reset_front_device(seekable_device& dev, uint8_t index,
                                                                                 std::streamoff starts_at)
    :
    start_pos(starts_at), fragment_on_seekable_device(dev, index)
{}

// ---------------------------------------------------------------------

void fragment_on_seekable_reset_front_device::reset()
{
    fragment_on_seekable_device::reset();
    dev_seek.seek(start_pos, std::ios_base::beg);
}

} // namespace uh::io