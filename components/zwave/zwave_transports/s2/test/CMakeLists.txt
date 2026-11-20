add_compile_definitions(UNIT_TEST)

# Keep this separate from s2_inclusion_mocks: test_span needs the real
# key-derivation object, which defines the same extraction symbol.
add_library(zwave_s2_key_derivation_mock kderiv_mock.c)
target_link_libraries(zwave_s2_key_derivation_mock PUBLIC zwave_s2_key_derivation PRIVATE mock)

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
