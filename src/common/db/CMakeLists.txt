add_library(uh_db connection.cpp row.cpp connstr.cpp)
target_link_libraries(uh_db uh_telemetry uh_utils uh_types PostgreSQL::PostgreSQL)
