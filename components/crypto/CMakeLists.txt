add_subdirectory(psa_wrappers)

# Consumers that provide their own crypto implementation need only the API.
add_library(crypto_api INTERFACE)
target_include_directories(crypto_api INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(crypto_api INTERFACE common)

# Keep every virtual method in the same library as the Crypto vtable.
set(CRYPTO_SOURCES src/c_api_backend.cpp src/aes_ctr_drbg.cpp src/x25519.cpp src/aes_cmac.cpp src/aes_cbc_mac.cpp src/aes_ccm.cpp src/aes_ecb.cpp src/aes_ofb.cpp src/rand.cpp src/util.cpp)
if(UNIT_TEST)
  list(APPEND CRYPTO_SOURCES src/c_api_override_test.cpp)
else()
  list(APPEND CRYPTO_SOURCES src/c_api_override_none.cpp)
endif()

add_library(crypto STATIC ${CRYPTO_SOURCES})
target_include_directories(crypto PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(crypto PUBLIC crypto_api PRIVATE psa_wrappers)
set_target_properties(crypto PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(UNIT_TEST)
  target_compile_definitions(crypto PRIVATE UNIT_TEST)
endif()

if(UNIT_TEST)
  add_subdirectory(test)
endif()
