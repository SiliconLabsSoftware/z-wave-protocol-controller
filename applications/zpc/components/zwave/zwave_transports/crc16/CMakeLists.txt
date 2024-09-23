# Library
add_library(zwave_crc16
  src/zwave_crc16_transport.c
)

target_include_directories(zwave_crc16
  PUBLIC include
)

target_link_libraries(zwave_crc16
  PUBLIC zwave_controller unify
  PRIVATE zpc_attribute_store zwave_command_classes
)

if(BUILD_TESTING)
  #target_add_mock(zwave_crc16)
  add_subdirectory(test)
endif()
