add_library(
  zwave_api
  src/zwapi_connection.c
  src/zwapi_init.c
  src/zwapi_protocol_basis.c
  src/zwapi_protocol_controller.c
  src/zwapi_protocol_mem.c
  src/zwapi_protocol_nvm.c
  src/zwapi_protocol_rx_dispatch.c
  src/zwapi_protocol_transport.c
  src/zwapi_session.c
  src/zwapi_utils.c
  platform/${COMPATIBLE_PLATFORM}/zwapi_serial.c
  platform/${COMPATIBLE_PLATFORM}/zwapi_ip.c
  platform/${COMPATIBLE_PLATFORM}/zwapi_connection_log.c
  platform/${COMPATIBLE_PLATFORM}/zwapi_timestamp.c
)

target_link_libraries(zwave_api PUBLIC zwave_definitions log pthread)
target_include_directories(
  zwave_api
  PRIVATE src platform/${COMPATIBLE_PLATFORM}
  PUBLIC include
  # Needed for Z-Wave controller endian, we do not want to pull the whole Z-Wave Controller library here.
  $<TARGET_PROPERTY:zwave_controller,INTERFACE_INCLUDE_DIRECTORIES>
  )
install(TARGETS zwave_api LIBRARY DESTINATION lib)
