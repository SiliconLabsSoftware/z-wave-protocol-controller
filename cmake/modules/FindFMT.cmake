include(FetchContent)
include(${CMAKE_CURRENT_LIST_DIR}/../include/external_compiler_flags.cmake)

message(NOTICE "-- Finding FMT")
# We use fmt for string formatting (performance and minimal dependency)
# See https://github.com/fmtlib/fmt/blob/master/README.md#benchmarks
# We need a strong, fast, modern, and flexible formatting library for MQTT purposes
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        12.1.0
)
zpc_push_external_compiler_flags()
FetchContent_MakeAvailable(fmt)
zpc_pop_external_compiler_flags()

