add_compile_definitions(
  UNIT_TEST
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

add_unity_test(NAME test_inclusion_joining_node_slave
  TEST_BASE
    test_inclusion_joining_node.c
  INCLUDES
    ..
  FILES
    ../s2_inclusion.c
    test_inclusion_joining_node.c
  LIBRARIES
    mock
    s2_mock
    s2_inclusion_mocks
    zwave_s2_key_derivation_mock
    crypto_c
)
