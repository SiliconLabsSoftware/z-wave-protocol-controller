find_package(MbedTLS REQUIRED)

set(S2CRYPTO_ZWAVE_SOURCES
  src/s2_psa_primitives.c
  src/aes_cmac_psa.c
  src/kderiv.c)

add_library(s2crypto_zwave STATIC ${S2CRYPTO_ZWAVE_SOURCES})
target_include_directories(s2crypto_zwave PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(s2crypto_zwave PUBLIC MbedTLS::tfpsacrypto)
set_target_properties(s2crypto_zwave PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 99)
set_source_files_properties(src/kderiv.c PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)

add_library(s2crypto_ctr_drbg STATIC src/ctr_drbg.c src/nextnonce.c)
target_include_directories(s2crypto_ctr_drbg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(s2crypto_ctr_drbg PUBLIC s2crypto_zwave)
set_target_properties(s2crypto_ctr_drbg PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 99)

if(UNIT_TEST)
  add_library(s2crypto_ctr_drbg_kat STATIC src/ctr_drbg.c)
  target_compile_definitions(s2crypto_ctr_drbg_kat PRIVATE RANDLEN=64)
  target_include_directories(s2crypto_ctr_drbg_kat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(s2crypto_ctr_drbg_kat PUBLIC s2crypto_zwave)
  set_target_properties(s2crypto_ctr_drbg_kat PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 99)
endif()

add_library(s2crypto INTERFACE)
target_link_libraries(s2crypto INTERFACE s2crypto_zwave s2crypto_ctr_drbg)

if(UNIT_TEST)
  add_subdirectory(test)
endif()
