add_zpc_gtest(
  NAME test_jamming_detection
  SRCS test_jamming_detection.cpp
  LIBS jamming_detection zwave_api nlohmann_json::nlohmann_json)

add_zpc_gtest(
  NAME test_jamming_serial_api
  SRCS test_jamming_detection_sapi.cpp
  LIBS zwave_api
  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/../../zwave_api/src)
