add_library(proxy service.cpp command_factory.cpp forward_command.cpp request_factory.cpp handler.cpp)
target_link_libraries(proxy types utils network entrypoint)
