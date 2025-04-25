add_library(uh_mock_storage mock_data_store.cpp)
target_link_libraries(uh_mock_storage types storage telemetry utils)
target_include_directories(uh_mock_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

add_library(uh_mock_global_data mock_data_view.cpp)
target_link_libraries(uh_mock_global_data uh_mock_storage)
