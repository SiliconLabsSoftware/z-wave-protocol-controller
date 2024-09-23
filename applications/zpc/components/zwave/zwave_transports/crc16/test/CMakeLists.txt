if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # "--allow-multiple-definition" can cause some weird symptons as we leave it
  # up to the linker which symbol gets merged into the test binary.
  add_link_options("-Wl,--unresolved-symbols=ignore-in-object-files"
                   "-Wl,--allow-multiple-definition")
endif()

# CRC16 transport test
target_add_unittest(zwave_crc16
SOURCES zwave_crc16_transport_test.c
DEPENDS
  zwave_command_classes_mock
  zwave_controller_mock
  zwave_tx_mock
  zpc_attribute_store_mock
)
