add_compile_definitions(UNIT_TEST)

add_library(crypto_test_helpers INTERFACE)
target_link_libraries(crypto_test_helpers INTERFACE crypto_core GTest::gtest GTest::gmock)
target_include_directories(crypto_test_helpers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_zpc_gtest(NAME test_psa_wrappers SRCS test_psa_wrappers.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_crypto_injection SRCS test_crypto.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_psa_smoke SRCS test_psa_smoke.cpp LIBS crypto_test_helpers)

add_zpc_gtest(NAME test_crypto_rand SRCS test_rand.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_psa_aes_ecb SRCS test_psa_aes_ecb.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_psa_aes_ofb SRCS test_psa_aes_ofb.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_psa_aes_cbc_mac SRCS test_psa_aes_cbc_mac.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_psa_aes_cmac SRCS test_psa_aes_cmac.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_psa_ccm SRCS test_psa_ccm.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_psa_curve25519 SRCS test_psa_curve25519.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(
  NAME test_zwave_ctr_drbg
  SRCS test_zwave_ctr_drbg.cpp
  LIBS crypto_test_helpers crypto_ctr_drbg_kat)
target_compile_definitions(test_zwave_ctr_drbg PRIVATE CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE=64)
