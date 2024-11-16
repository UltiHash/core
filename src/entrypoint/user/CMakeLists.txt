add_library(uh_user db.cpp)
target_link_libraries(uh_user uh_http uh_crypto uh_types uh_db uh_policy)
