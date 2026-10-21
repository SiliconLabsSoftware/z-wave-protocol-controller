add_compile_definitions(UNIT_TEST)

add_library(psa_test_support STATIC psa_test_support.c)
target_link_libraries(psa_test_support PUBLIC s2crypto s2crypto_zwave)
target_include_directories(psa_test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_unity_test(NAME test_psa_smoke FILES test_psa_smoke.c LIBRARIES psa_test_support)

add_unity_test(NAME test_psa_aes_ecb FILES test_psa_aes_ecb.c LIBRARIES psa_test_support)

add_unity_test(NAME test_psa_aes_cmac FILES test_psa_aes_cmac.c LIBRARIES psa_test_support)

add_unity_test(NAME test_psa_ccm FILES test_psa_ccm.c LIBRARIES psa_test_support)

add_unity_test(NAME test_psa_curve25519 FILES test_psa_curve25519.c LIBRARIES psa_test_support)

add_unity_test(NAME test_psa_kderiv FILES test_psa_kderiv.c LIBRARIES psa_test_support s2crypto_zwave)

add_unity_test(NAME test_zwave_ctr_drbg FILES test_zwave_ctr_drbg.c LIBRARIES psa_test_support s2crypto_ctr_drbg_kat)

add_unity_test(NAME test_zwave_nextnonce FILES test_zwave_nextnonce.c LIBRARIES s2crypto_zwave s2crypto_ctr_drbg)
