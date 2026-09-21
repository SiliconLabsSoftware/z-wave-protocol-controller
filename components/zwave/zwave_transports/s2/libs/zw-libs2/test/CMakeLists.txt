
add_compile_definitions(
  UNIT_TEST
)

#include_directories( ../crypto/aes )
#add_executable( test_ccm test_ccm.c ../crypto/ccm/ccm.c ../crypto/aes/aes.c )
#add_test( test_ccm test_ccm )

# AES-128 ECB/CBC (vectors from legacy aes_test.c)
add_unity_test(NAME test_aes FILES test_aes.c LIBRARIES aes)

# Add test for AES-CMAC
add_unity_test(NAME test_aes_cmac FILES test_aes_cmac.c LIBRARIES s2crypto aes)

# Add test for Curve25519
include_directories(.)
add_unity_test(NAME test_curve25519 FILES test_curve25519.c LIBRARIES s2crypto aes)

# Add test for CCM
add_unity_test(NAME test_ccm FILES test_ccm.c ../crypto/ccm/ccm.c ../crypto/aes/aes.c)

add_compile_definitions(
  RANDLEN=64 
)

add_unity_test(NAME test_ctr_dbrg FILES test_ctr_dbrg.c ../crypto/ctr_drbg/ctr_drbg.c ../crypto/aes/aes.c)

add_unity_test(NAME test_kderiv FILES test_kderiv.c ../crypto/kderiv/kderiv.c ../crypto/aes-cmac/aes_cmac.c ../crypto/aes/aes.c)