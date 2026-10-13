# Header-only CRC16-CCITT library
add_library(crc16_ccitt INTERFACE)

target_include_directories(crc16_ccitt INTERFACE include)

if(UNIT_TEST)
  add_subdirectory(test)
endif()
