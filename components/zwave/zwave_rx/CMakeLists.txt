SET(SOURCE_LIST "src/zwave_rx.c"
                "src/zwave_rx_callbacks.c"
                "src/zwave_rx_process.cpp")

# Library
add_library(zwave_rx ${SOURCE_LIST} src/zwave_rx_zwapi_callbacks.c)

target_include_directories(zwave_rx
  PUBLIC include
  PUBLIC src)

target_link_libraries(zwave_rx
  PRIVATE zwave_controller
          zwave_definitions
          threading
          main
  PUBLIC  config)
