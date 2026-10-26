add_compile_definitions(UNIT_TEST)

add_library(psa_test_support STATIC psa_test_support.cpp)
target_link_libraries(psa_test_support PUBLIC crypto_core GTest::gtest GTest::gmock)
target_include_directories(psa_test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_zpc_gtest(NAME test_psa_smoke SRCS test_psa_smoke.cpp LIBS psa_test_support)

add_zpc_gtest(NAME test_crypto_rand SRCS test_crypto_rand.cpp LIBS psa_test_support crypto)

add_zpc_gtest(NAME test_psa_aes_ecb SRCS test_psa_aes_ecb.cpp LIBS psa_test_support crypto)

add_zpc_gtest(NAME test_psa_aes_ofb SRCS test_psa_aes_ofb.cpp LIBS psa_test_support crypto)

add_zpc_gtest(NAME test_psa_aes_cbc_mac SRCS test_psa_aes_cbc_mac.cpp LIBS psa_test_support crypto)

add_zpc_gtest(NAME test_psa_aes_cmac SRCS test_psa_aes_cmac.cpp LIBS psa_test_support crypto)

add_zpc_gtest(NAME test_psa_ccm SRCS test_psa_ccm.cpp LIBS psa_test_support crypto)

add_zpc_gtest(NAME test_psa_curve25519 SRCS test_psa_curve25519.cpp LIBS psa_test_support crypto)

add_zpc_gtest(
  NAME test_zwave_ctr_drbg
  SRCS test_zwave_ctr_drbg.cpp
  LIBS psa_test_support crypto_ctr_drbg_kat)
target_compile_definitions(test_zwave_ctr_drbg PRIVATE CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE=64)
