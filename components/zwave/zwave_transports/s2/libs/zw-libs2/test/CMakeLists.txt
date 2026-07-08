
add_compile_definitions(
  UNIT_TEST
)

#include_directories( ../crypto/aes )
#add_executable( test_ccm test_ccm.c ../crypto/ccm/ccm.c ../crypto/aes/aes.c )
#add_test( test_ccm test_ccm )

# Add test for AES-CMAC
add_unity_test(NAME test_aes_cmac FILES test_aes_cmac.c LIBRARIES s2crypto aes)

# Add test for Curve25519
include_directories(.)
add_unity_test(NAME test_curve25519 FILES test_curve25519.c LIBRARIES s2crypto aes)

# Add test for CCM
add_unity_test(NAME test_ccm FILES test_ccm.c ../crypto/ccm/ccm.c ../crypto/aes/aes.c)

add_compile_definitions(NEW_TEST_T2 )
add_executable(new_test_t2
        new_test_t2.c
        clock_time.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../transport_service/transport_service2.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../transport_service/transport2_fsm.c)
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/../transport_service/
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/ )
if (${CMAKE_PROJECT_NAME} MATCHES "zipgateway")
  target_link_libraries(new_test_t2 zipgateway-lib)
endif()
add_test(test_transport_service2 new_test_t2)

add_compile_definitions(
  RANDLEN=64 
)

add_unity_test(NAME test_ctr_dbrg FILES test_ctr_dbrg.c ../crypto/ctr_drbg/ctr_drbg.c ../crypto/aes/aes.c)

add_unity_test(NAME test_kderiv FILES test_kderiv.c ../crypto/kderiv/kderiv.c ../crypto/aes-cmac/aes_cmac.c ../crypto/aes/aes.c)