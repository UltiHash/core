add_library(uh_common_caches INTERFACE lfu_cache.h plain_cache.h lru_cache.h)
target_link_libraries(uh_common_caches INTERFACE uh_common_types zpp_bits)
