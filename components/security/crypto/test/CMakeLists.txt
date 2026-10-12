add_compile_definitions(UNIT_TEST)

add_unity_test(NAME test_psa_smoke FILES test_psa_smoke.c LIBRARIES s2crypto_psa)
