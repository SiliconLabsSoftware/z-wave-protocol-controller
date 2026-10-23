find_package(MbedTLS REQUIRED)

set(CRYPTO_CORE_SOURCES src/x25519.c src/aes_cmac.c src/aes_cbc_mac.c src/aes_ccm.c src/aes_ecb.c src/aes_ofb.c src/rand.c src/helpers.c)

add_library(crypto_core STATIC ${CRYPTO_CORE_SOURCES})
target_include_directories(crypto_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(crypto_core PUBLIC common MbedTLS::tfpsacrypto)
set_target_properties(crypto_core PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 99)

add_library(crypto_ctr_drbg STATIC src/aes_ctr_drbg.c)
target_include_directories(crypto_ctr_drbg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(crypto_ctr_drbg PUBLIC crypto_core)
set_target_properties(crypto_ctr_drbg PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 99)

if(UNIT_TEST)
  add_library(crypto_ctr_drbg_kat STATIC src/aes_ctr_drbg.c)
  target_compile_definitions(crypto_ctr_drbg_kat PRIVATE CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE=64)
  target_include_directories(crypto_ctr_drbg_kat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(crypto_ctr_drbg_kat PUBLIC crypto_core)
  set_target_properties(crypto_ctr_drbg_kat PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 99)
endif()

add_library(crypto INTERFACE)
target_link_libraries(crypto INTERFACE crypto_core crypto_ctr_drbg)

if(UNIT_TEST)
  add_subdirectory(test)
endif()
