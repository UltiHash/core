find_package(OpenSSL REQUIRED)

add_library_bundle(license
                    SOURCES license.cpp usage.cpp internal/util.cpp
                    PUBLIC OpenSSL::Crypto db network nlohmann_json)
