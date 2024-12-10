cpmaddpackage(
    NAME
    benchmark
    GITHUB_REPOSITORY
    google/benchmark
    VERSION
    1.8.5
    OPTIONS
    "BENCHMARK_ENABLE_TESTING Off")

function(uh_add_profiler name)
    # Parse Arguments
    set(options "")
    set(multi_value_args PRIVATE PUBLIC)
    cmake_parse_arguments(ARGS "${options}" "${one_value_args}"
                          "${multi_value_args}" ${ARGN})

    set(target_name "${PROJECT_NAME}_${name}")

    add_executable(${target_name} ${name}.cpp)

    target_link_libraries(
        ${target_name}
        PRIVATE ${PROJECT_NAME}_shared benchmark::benchmark ${ARGS_PRIVATE}
        PUBLIC ${ARGS_PUBLIC})

    target_include_directories(
        ${target_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
                               ${CMAKE_CURRENT_SOURCE_DIR})
    target_precompile_headers(${target_name} REUSE_FROM uh_precompile_headers)
    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

uh_add_profiler(profile_deduplicator PRIVATE uh_fake_global_data)
