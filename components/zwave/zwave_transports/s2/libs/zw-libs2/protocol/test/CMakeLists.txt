# CMakeLists.txt for S2 protocol unit tests

add_compile_definitions(
  UNIT_TEST
)

# Add test for SPAN retry decryption feature (SWPROT-9892)
# This tests the implementation of Z-Wave specification CC:009F.01.00.12.003
# S2.c and s2_inclusion.c are compiled directly without ZW_CONTROLLER so the
# test exercises the end-device decryption path through S2_application_command_handler.
add_unity_test(
  NAME test_span
  FILES
    test_span.c
    ../S2.c
    ../../inclusion/s2_inclusion.c
  LIBRARIES s2crypto mock s2_inclusion_mocks crypto_mocks aes
)

# Additional protocol tests can be added here as needed
