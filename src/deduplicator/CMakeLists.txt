add_library(deduplicator
            handler.cpp
            fragmentation.cpp
            dedupe_set/fragment_set_element.cpp
            dedupe_set/fragment_set.cpp
            interfaces/local_deduplicator.cpp
            interfaces/remote_deduplicator.cpp
            interfaces/noop_deduplicator.cpp
            interfaces/dedupe_array.cpp)

target_link_libraries(deduplicator network storage)
