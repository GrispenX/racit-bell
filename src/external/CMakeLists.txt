find_package(OpenSSL REQUIRED)

add_library(httplib INTERFACE)
target_include_directories(httplib INTERFACE ${CMAKE_SOURCE_DIR}/include/external/httplib)
target_compile_definitions(httplib INTERFACE CPPHTTPLIB_OPENSSL_SUPPORT)
target_link_libraries(httplib INTERFACE OpenSSL::SSL OpenSSL::Crypto)

add_library(jwt-cpp INTERFACE)
target_include_directories(jwt-cpp INTERFACE ${CMAKE_SOURCE_DIR}/include/external/jwt-cpp)
target_compile_definitions(jwt-cpp INTERFACE JWT_DISABLE_PICOJSON)
target_link_libraries(jwt-cpp INTERFACE OpenSSL::SSL OpenSSL::Crypto)
