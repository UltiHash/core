#ifndef CONFIG_H
#define CONFIG_H

#include <cstddef>

namespace uh::cluster {

struct recovery_config {
    size_t thread_count = 4;
};

} // end namespace uh::cluster

#endif // CONFIG_H
