include(FetchContent)
include(${CMAKE_CURRENT_LIST_DIR}/../include/external_compiler_flags.cmake)

message(NOTICE "-- Finding FMT")
# Static by default (BUILD_SHARED_LIBS is unset). Do not install into the .deb.
# We use fmt for string formatting (performance and minimal dependency)
# See https://github.com/fmtlib/fmt/blob/master/README.md#benchmarks
# We need a strong, fast, modern, and flexible formatting library for MQTT purposes
set(FMT_INSTALL OFF CACHE BOOL "Install fmt" FORCE)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        12.1.0
)
zpc_push_external_compiler_flags()
FetchContent_MakeAvailable(fmt)
zpc_pop_external_compiler_flags()
