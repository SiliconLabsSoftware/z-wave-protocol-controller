add_library(utils src/utils.cpp)

target_include_directories(utils PUBLIC include)

target_link_libraries(
  utils
  PUBLIC
  common
  zwave_definitions
  PRIVATE
  fmt::fmt
)

install(TARGETS utils LIBRARY DESTINATION lib)
