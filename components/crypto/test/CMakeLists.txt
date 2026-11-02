add_compile_definitions(UNIT_TEST)

add_library(crypto_c_api_test_support INTERFACE)
target_link_libraries(crypto_c_api_test_support INTERFACE crypto)
target_include_directories(crypto_c_api_test_support INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(crypto_test_helpers INTERFACE)
target_link_libraries(crypto_test_helpers INTERFACE GTest::gtest GTest::gmock psa_wrappers)
target_include_directories(crypto_test_helpers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_zpc_gtest(NAME test_crypto_injection SRCS test_crypto_injection_misc.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_util SRCS test_util.cpp LIBS crypto MbedTLS::tfpsacrypto INCLUDES ../src)

add_zpc_gtest(NAME test_rand SRCS test_rand.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_aes_ecb SRCS test_aes_ecb.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_aes_ofb SRCS test_aes_ofb.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_aes_cbc_mac SRCS test_aes_cbc_mac.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_aes_cmac SRCS test_aes_cmac.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_aes_ccm SRCS test_aes_ccm.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(NAME test_x25519 SRCS test_x25519.cpp LIBS crypto_test_helpers crypto)

add_zpc_gtest(
  NAME test_aes_ctr_drbg
  SRCS test_aes_ctr_drbg.cpp
  LIBS crypto_test_helpers crypto)
