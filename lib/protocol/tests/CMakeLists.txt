add_executable(test_messages test_messages.cpp)
target_link_libraries(test_messages ${Boost_LIBRARIES} uhprotocol)
target_include_directories(test_messages PRIVATE ${Boost_INCLUDE_DIR})
add_test(NAME test_messages COMMAND test_messages)

add_executable(test_serializer test_serializer.cpp)
target_link_libraries(test_serializer ${Boost_LIBRARIES} uhprotocol)
target_include_directories(test_serializer PRIVATE ${Boost_INCLUDE_DIR})
add_test(NAME test_serializer COMMAND test_serializer)
