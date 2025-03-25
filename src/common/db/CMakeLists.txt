find_package(PostgreSQL REQUIRED)

add_library_bundle(db SOURCES connection.cpp connstr.cpp row.cpp
    PUBLIC PostgreSQL::PostgreSQL types)
