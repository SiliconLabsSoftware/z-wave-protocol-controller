# CMakeLists.txt for S2 protocol unit tests

add_compile_definitions(
  UNIT_TEST
)

# Add test for SPAN retry decryption feature (SWPROT-9892)
# This tests the implementation of Z-Wave specification CC:009F.01.00.12.003
# Uses the real S2 implementation (s2_slave) to test actual SPAN decryption flow
# through S2_application_command_handler entry point.
add_unity_test(
  NAME test_span
  FILES test_span.c
  LIBRARIES s2_slave s2crypto mock s2_inclusion_mocks crypto_mocks aes
)

# Additional protocol tests can be added here as needed
