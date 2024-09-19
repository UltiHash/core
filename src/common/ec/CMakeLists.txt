add_library(uh_common_ec INTERFACE ec_scheme.h ec_interface.h reedsolomon_c.h no_ec.h ec_factory.h)
target_link_libraries(uh_common_ec INTERFACE reedsolomon uh_common_types uh_common_utils)
