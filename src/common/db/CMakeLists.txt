add_library(uh_db STATIC connection.cpp connstr.cpp row.cpp)
target_link_libraries(uh_db PUBLIC uh_utils uh_telemetry uh_types
                                   PostgreSQL::PostgreSQL)
target_precompile_headers(uh_db REUSE_FROM uh_precompile_headers)
