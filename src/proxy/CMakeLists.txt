add_library(proxy
            handler.cpp
            service.cpp
            command_factory.cpp
            )

target_link_libraries(proxy entrypoint)
