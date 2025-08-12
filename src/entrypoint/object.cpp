#include "object.h"


namespace uh::cluster::ep {

object_state to_object_state(const std::string& s) {
    if (s == "Normal") {
        return object_state::normal;
    }

    if (s == "Deleted") {
        return object_state::deleted;
    }

    if (s == "Collected") {
        return object_state::collected;
    }

    throw std::runtime_error("unsupported object state: " + s);
}

std::string to_string(object_state os) {
    switch (os) {
        case object_state::normal: return "Normal";
        case object_state::deleted: return "Deleted";
        case object_state::collected: return "Collected";
    }

    throw std::runtime_error("unsupport object state");
}

} // namespace uh::cluster::ep
