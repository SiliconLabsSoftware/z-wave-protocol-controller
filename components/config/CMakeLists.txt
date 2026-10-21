# Dependencies
find_package(Yaml-cpp REQUIRED)

# Library
set(CONFIG_SRC platform/${COMPATIBLE_PLATFORM}/config.cpp src/zpc_config.c platform/${COMPATIBLE_PLATFORM}/device_id.c)
set(CONFIG_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/include/)
add_library(config ${CONFIG_SRC})
target_include_directories(
  config
  PUBLIC $<BUILD_INTERFACE:${CONFIG_INCLUDE}>
  PRIVATE platform/${COMPATIBLE_PLATFORM})
target_link_libraries(config yaml-cpp::yaml-cpp zwave_definitions common log)
target_compile_definitions(
  config PRIVATE CMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}" CMAKE_PROJECT_VERSION="${CMAKE_PROJECT_VERSION}")
