add_static_and_shared(
    uh_storage
    SOURCES
    reference_counter.cpp
    data_store.cpp
    PRIVATE
    uh_telemetry
    PUBLIC
    uh_coroutines
    lmdbxx
    lmdb
    uh_etcd
    uh_ec
    uh_registry
    uh_debug
    uh_utils
    uh_types
    uh_network)
# uh_recovery uh_ec_groups uh_service_discovery uh_network uh_service_interfaces
# uh_etcd uh_ec uh_coroutines
