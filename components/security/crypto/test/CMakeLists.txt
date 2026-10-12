add_compile_definitions(UNIT_TEST)

set(ZW_LIBS2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../zwave/zwave_transports/s2/libs/zw-libs2")

add_library(psa_test_support STATIC psa_test_support.c psa_aes_cmac_shim.c)
target_link_libraries(psa_test_support PUBLIC s2crypto_psa)
target_include_directories(psa_test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(psa_test_support PRIVATE ${ZW_LIBS2_DIR}/include)

add_unity_test(NAME test_psa_smoke FILES test_psa_smoke.c LIBRARIES psa_test_support)

add_unity_test(NAME test_psa_aes_ecb FILES test_psa_aes_ecb.c LIBRARIES psa_test_support)

add_unity_test(NAME test_psa_aes_cmac FILES test_psa_aes_cmac.c LIBRARIES psa_test_support)

add_unity_test(NAME test_psa_ccm FILES test_psa_ccm.c LIBRARIES psa_test_support)

add_unity_test(NAME test_psa_curve25519 FILES test_psa_curve25519.c LIBRARIES psa_test_support)

add_unity_test(
  NAME test_psa_kderiv
  FILES
    test_psa_kderiv.c
    ${ZW_LIBS2_DIR}/crypto/kderiv/kderiv.c
  INCLUDES
    ${ZW_LIBS2_DIR}/include
  LIBRARIES
    psa_test_support)
