# Library
add_library(
  zwave_tx
  src/zwave_tx.cpp
  src/zwave_tx_callbacks.cpp
  src/zwave_tx_process.cpp
  src/zwave_tx_queue.cpp
  src/zwave_tx_state_logging.c
  src/zwave_tx_route_cache.cpp)

target_include_directories(
  zwave_tx
  PUBLIC include
  PUBLIC src)

target_link_libraries(
  zwave_tx
  PUBLIC zwave_tx_groups zwave_definitions zwave_api
  PRIVATE zwave_controller timer threading main)
install(TARGETS zwave_tx LIBRARY DESTINATION lib)
