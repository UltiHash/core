#include "time_utils.h"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace uh::cluster {

std::string get_current_ISO8601_datetime() {
    boost::posix_time::ptime t =
        boost::posix_time::microsec_clock::universal_time();
    return to_iso_extended_string(t);
}

} // end namespace uh::cluster
