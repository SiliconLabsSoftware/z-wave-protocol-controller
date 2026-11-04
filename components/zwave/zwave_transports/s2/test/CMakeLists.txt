add_compile_definitions(UNIT_TEST)

add_unity_test(
  NAME test_zwave_nextnonce
  FILES test_zwave_nextnonce.c
  INCLUDES ../include
  LIBRARIES zwave_s2_next_nonce)

add_unity_test(
  NAME test_zwave_s2_key_derivation
  FILES test_zwave_s2_key_derivation.c
  LIBRARIES zwave_s2_key_derivation)

add_zpc_gtest(
  NAME test_zwave_s2_key_derivation_failures
  SRCS test_zwave_s2_key_derivation_failures.cpp
  LIBS crypto_c_test_support zwave_s2_key_derivation zwave_s2_next_nonce)
