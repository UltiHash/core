find_package(PostgreSQL REQUIRED)

add_library(db connection.cpp connstr.cpp row.cpp)
target_link_libraries(db PostgreSQL::PostgreSQL types)
