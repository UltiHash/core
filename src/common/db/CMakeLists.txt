add_library(uh_common_db connection.cpp row.cpp connstr.cpp)
target_link_libraries(uh_common_db PostgreSQL::PostgreSQL uh_common_debug uh_common_telemetry uh_common_types uh_common_utils)
