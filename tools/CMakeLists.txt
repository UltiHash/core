cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)

add_executable(${PROJECT_NAME}-basic-client basic/main.cpp)
target_link_libraries(${PROJECT_NAME}-basic-client uh_libraries)
target_precompile_headers(${PROJECT_NAME}-basic-client REUSE_FROM uh_precompile_headers)

add_executable(${PROJECT_NAME}-storage-stress-test storage_stress_test/main.cpp)
target_link_libraries(${PROJECT_NAME}-storage-stress-test uh_libraries)
target_precompile_headers(${PROJECT_NAME}-storage-stress-test REUSE_FROM uh_precompile_headers)

add_executable(${PROJECT_NAME}-dd-stress-test dd_stress_test/main.cpp)
target_link_libraries(${PROJECT_NAME}-dd-stress-test uh_libraries)
target_precompile_headers(${PROJECT_NAME}-dd-stress-test REUSE_FROM uh_precompile_headers)

add_executable(${PROJECT_NAME}-file-upload file_upload/main.cpp)
target_link_libraries(${PROJECT_NAME}-file-upload uh_libraries)
target_precompile_headers(${PROJECT_NAME}-file-upload REUSE_FROM uh_precompile_headers)

add_executable(${PROJECT_NAME}-uh-dir uh-dir/main.cpp)
target_link_libraries(${PROJECT_NAME}-uh-dir uh_libraries)
target_precompile_headers(${PROJECT_NAME}-uh-dir REUSE_FROM uh_precompile_headers)

add_executable(${PROJECT_NAME}-storage storage/main.cpp)
target_link_libraries(${PROJECT_NAME}-storage uh_libraries)
target_precompile_headers(${PROJECT_NAME}-storage REUSE_FROM uh_precompile_headers)

add_executable(${PROJECT_NAME}-access-client access/main.cpp)
target_link_libraries(${PROJECT_NAME}-access-client uh_libraries)
target_precompile_headers(${PROJECT_NAME}-access-client REUSE_FROM uh_precompile_headers)
