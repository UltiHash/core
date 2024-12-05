add_library(mock_storage SHARED data_store_mock.cpp)
target_include_directories(mock_storage PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(mock_storage PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES})
