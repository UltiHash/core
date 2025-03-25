add_library_bundle(conf
                    SOURCES configuration.cpp
                    PUBLIC db etcd telemetry utils network license CLI11::CLI11 coordinator entrypoint deduplicator storage)
