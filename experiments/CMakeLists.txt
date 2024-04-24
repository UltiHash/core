cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)

add_executable(${PROJECT_NAME}-dd-prefix-expr dedupe_prefix_scale.cpp)
add_executable(${PROJECT_NAME}-dd-dumb-expr dedupe_dumb_scale.cpp)
add_executable(${PROJECT_NAME}-dd-prefix-isolate-expr dedupe_prefix_isolate_scale.cpp)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
find_package(RocksDB REQUIRED)
add_executable(${PROJECT_NAME}-rocksdb rocksdb.cpp ${CMAKE_SOURCE_DIR}/src/common/utils/random.cpp)
target_link_libraries(${PROJECT_NAME}-rocksdb ${OPENSSL_LIBRARIES} ${ROCKSDB_LIBRARIES})
target_include_directories(${PROJECT_NAME}-rocksdb PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_compile_options(${PROJECT_NAME}-rocksdb PUBLIC "-Wno-error=deprecated-declarations")

find_package(SQLite3 REQUIRED)
add_executable(${PROJECT_NAME}-sqlite sqlite.cpp)
target_link_libraries(${PROJECT_NAME}-sqlite ${OPENSSL_LIBRARIES} ${SQLite3_LIBRARIES})
target_compile_options(${PROJECT_NAME}-sqlite PUBLIC "-Wno-error=deprecated-declarations")

