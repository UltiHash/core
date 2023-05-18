//
// Created by benjamin-elias on 18.05.23.
//

#ifndef CORE_FRAGMENT_H
#define CORE_FRAGMENT_H

#include "fragment_on_seekable_device.h"

namespace uh::io{

    template <typename Fragment = fragment_on_seekable_device>
    requires (std::is_base_of_v<fragmented_device,Fragment>)
    class fragment : public Fragment {
    public:
        explicit fragment(Fragment &dev): Fragment(dev) {}
    };

} // namespace uh::io

#endif //CORE_FRAGMENT_H
