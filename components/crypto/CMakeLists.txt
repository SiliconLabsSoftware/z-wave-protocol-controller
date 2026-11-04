set(TF_PSA_CRYPTO_USER_CONFIG_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/config/tf_psa_crypto_user_config.h"
    CACHE FILEPATH "" FORCE)
set(MBEDTLS_USER_CONFIG_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/config/mbedtls_user_config.h"
    CACHE FILEPATH "" FORCE)
find_package(MbedTLS REQUIRED)

add_library(crypto_api INTERFACE)
target_include_directories(crypto_api INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(crypto_api INTERFACE common)

set(CRYPTO_SOURCES
    src/default_crypto.cpp
    src/algorithms/aes_ctr_drbg.cpp
    src/algorithms/x25519.cpp
    src/algorithms/aes_cmac.cpp
    src/algorithms/aes_cbc_mac.cpp
    src/algorithms/aes_ccm.cpp
    src/algorithms/aes_ecb.cpp
    src/algorithms/aes_ofb.cpp
    src/algorithms/rand.cpp
    src/psa/psa_wrappers.cpp
    src/psa/util.cpp)

add_library(crypto STATIC ${CRYPTO_SOURCES})
target_include_directories(crypto PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(crypto PUBLIC crypto_api PRIVATE MbedTLS::tfpsacrypto)
set_target_properties(crypto PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(UNIT_TEST)
  target_compile_definitions(crypto PRIVATE UNIT_TEST)
endif()

if(UNIT_TEST)
  add_subdirectory(test)
endif()
