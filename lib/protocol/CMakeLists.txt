cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)

option(PROTOCOL_USE_SMALL_CHUNKS "Enable small maximum chunk size to allow for testing protocol implementation in environments with restricted memory" OFF)

find_package(Boost 1.80 COMPONENTS unit_test_framework test_exec_monitor REQUIRED)

add_library(uhprotocol
            client.cpp
            client_factory.cpp
            client_pool.cpp
            messages.cpp
            serializer.cpp
            server.cpp)

target_link_libraries(uhprotocol uhLibNetwork uhLibLogging uhutils)

if (PROTOCOL_USE_SMALL_CHUNKS)
    add_compile_definitions(PROTOCOL_MAXIMUM_CHUNK_SIZE=10*1024*1024)
endif()

add_subdirectory(tests)
