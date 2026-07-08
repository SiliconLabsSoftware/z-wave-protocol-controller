add_library(log OBJECT src/log.cpp)

target_include_directories(log PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

target_link_libraries(
  log
  PUBLIC common)
