add_library(proxy service.cpp forward_stream.cpp request_factory.cpp handler.cpp)
target_link_libraries(proxy types utils network entrypoint)
