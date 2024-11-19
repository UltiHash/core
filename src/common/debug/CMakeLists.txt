add_library(uh_debug INTERFACE)
target_link_libraries(uh_debug INTERFACE uh_telemetry)
# target_precompile_headers(
#     uh_debug
#     INTERFACE
#     <string>
#     <typeinfo>
#     <boost/algorithm/string.hpp>
#     <ostream>
#     <source_location>
#     <atomic>
#     <functional>
#     <map>
#     <sstream>
#     <thread>)
