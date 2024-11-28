add_static_and_shared(
    uh_config
    SOURCES
    configuration.cpp
    PUBLIC
    # uh_db uh_etcd uh_license uh_telemetry uh_utils uh_deduplicator
    # uh_entrypoint uh_storage
    CLI11::CLI11
    # uh_recovery
)
