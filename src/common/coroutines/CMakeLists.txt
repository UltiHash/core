add_library(uh_coroutines INTERFACE)
target_link_libraries(
    uh_coroutines
    INTERFACE opentelemetry_api
              uh_debug)
target_precompile_headers(uh_coroutines INTERFACE
                          <opentelemetry/context/context.h>)
