SET(SOURCE_LIST "src/zwave_rx.c"
                "src/zwave_rx_fixt.c"
                "src/zwave_rx_process.c"
                "src/zwave_rx_callbacks.c")

# Library
add_library(zwave_rx ${SOURCE_LIST} src/zwave_rx_zwapi_callbacks.c)

target_include_directories(zwave_rx PUBLIC include)

target_link_libraries(zwave_rx
  PRIVATE zwave_controller
          zwave_definitions
          unify)

IF(NOT ZWAVE_BUILD_SYSTEM EQUAL 1)
  target_link_libraries(zwave_rx
    PUBLIC zpc_config
    PRIVATE unify)
ENDIF()

IF(ZWAVE_BUILD_SYSTEM EQUAL 1)
  target_link_libraries(zwave_rx
    PUBLIC  zpc_config)
ENDIF()

if(BUILD_TESTING)
  # CMock for Z - Wave RX
  target_add_mock(zwave_rx)

  add_subdirectory(test)
endif()

# zwave_rx_testlib library
add_library(zwave_rx_testlib ${SOURCE_LIST} src/zwave_rx_zwapi_callbacks.c)

target_include_directories(zwave_rx_testlib PUBLIC include)

target_link_libraries(zwave_rx_testlib
  PRIVATE zwave_controller_testlib
          zwave_definitions
          unify_testlib)
  
target_link_libraries(zwave_rx_testlib
  PRIVATE uic_main_fd_testlib)

target_compile_definitions(zwave_rx_testlib PRIVATE ZWAVE_TESTLIB)
