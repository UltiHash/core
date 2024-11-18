add_library(uh_db connection.cpp connstr.cpp row.cpp)
target_link_libraries(uh_db PUBLIC uh_utils uh_telemetry uh_types
                                   PostgreSQL::PostgreSQL)
