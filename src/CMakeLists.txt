include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(config)
add_subdirectory(common)
add_subdirectory(entrypoint)
add_subdirectory(deduplicator)
add_subdirectory(storage)
add_subdirectory(recovery)

add_executable(uh-cluster-2 main.cpp)

target_link_libraries(uh-cluster-2
  uh_config
  uh_telemetry
  uh_license
  uh_utils 
  uh_entrypoint
  uh_deduplicator
  uh_storage
  )
