add_compile_definitions(UNIT_TEST)

set(CRYPTO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../crypto/include)

add_unity_test(
  NAME test_zwave_nextnonce
  FILES test_zwave_nextnonce.c
  INCLUDES ../include
  LIBRARIES zwave_s2_next_nonce)

add_unity_test(
  NAME test_zwave_s2_key_derivation
  FILES test_zwave_s2_key_derivation.c
  LIBRARIES zwave_s2_key_derivation)

add_unity_test(
  NAME test_zwave_s2_key_derivation_failures
  FILES
    test_zwave_s2_key_derivation_failures.c
    ../src/zwave_s2_key_derivation.c
  INCLUDES ../include ${CRYPTO_INCLUDE_DIR}
  LIBRARIES common MbedTLS::tfpsacrypto)
