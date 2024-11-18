add_library(
    uh_iam STATIC
    list_user_policies.cpp
    get_user_policy.cpp
    delete_user_policy.cpp
    create_access_key.cpp
    delete_access_key.cpp
    put_user_policy.cpp
    delete_user.cpp
    create_user.cpp)
target_link_libraries(
    uh_iam
    PRIVATE uh_utils uh_http uh_aws uh_policy
    PUBLIC uh_user)
