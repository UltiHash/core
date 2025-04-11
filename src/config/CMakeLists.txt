add_library(conf
            configuration.cpp)

target_link_libraries(conf
                      db
                      etcd
                      telemetry
                      utils
                      network
                      license
                      CLI11::CLI11
                      coordinator
                      entrypoint
                      deduplicator
                      storage)
