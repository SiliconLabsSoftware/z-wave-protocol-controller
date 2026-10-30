add_subdirectory(backend)

# Keep every virtual method in the same library as the Crypto vtable.
set(CRYPTO_CORE_SOURCES src/crypto.cpp src/aes_ctr_drbg.cpp src/x25519.cpp src/aes_cmac.cpp src/aes_cbc_mac.cpp src/aes_ccm.cpp src/aes_ecb.cpp src/aes_ofb.cpp src/rand.cpp src/psa_status.cpp)

add_library(crypto_core STATIC ${CRYPTO_CORE_SOURCES})
target_include_directories(crypto_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(crypto_core PUBLIC common crypto_backend)
set_target_properties(crypto_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(UNIT_TEST)
  target_compile_definitions(crypto_core PRIVATE UNIT_TEST)
endif()

# Preserve the existing DRBG target for callers.
add_library(crypto_ctr_drbg INTERFACE)
target_link_libraries(crypto_ctr_drbg INTERFACE crypto_core)

if(UNIT_TEST)
  # The KAT executable needs a complete implementation with 64-byte DRBG output.
  add_library(crypto_ctr_drbg_kat STATIC ${CRYPTO_CORE_SOURCES})
  target_compile_definitions(crypto_ctr_drbg_kat PUBLIC CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE=64 PRIVATE UNIT_TEST)
  target_include_directories(crypto_ctr_drbg_kat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(crypto_ctr_drbg_kat PUBLIC common crypto_backend)
  set_target_properties(crypto_ctr_drbg_kat PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

add_library(crypto INTERFACE)
target_link_libraries(crypto INTERFACE crypto_core crypto_ctr_drbg)

if(UNIT_TEST)
  add_subdirectory(test)
endif()
