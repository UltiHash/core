add_library(uh_common_debug INTERFACE class_name.h debug.h monitor.h)
target_link_libraries(uh_common_debug INTERFACE uh_common_telemetry)
