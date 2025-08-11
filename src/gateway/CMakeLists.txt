add_library(gateway
            handler.cpp
            service.cpp
            command_factory.cpp
            )

target_link_libraries(gateway entrypoint)
