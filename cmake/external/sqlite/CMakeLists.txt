# Built from FindSQLite3.cmake under zpc_push_external_compiler_flags()
# so this target does not inherit ZPC -Werror / sanitizers / hardening.

add_library(sqlite3_amalgamation STATIC "${ZPC_SQLITE3_C}")
target_include_directories(sqlite3_amalgamation PUBLIC "${ZPC_SQLITE3_DIR}")
set_target_properties(
  sqlite3_amalgamation
  PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 99)
target_compile_options(sqlite3_amalgamation PRIVATE -w)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(sqlite3_amalgamation PUBLIC dl m pthread)
endif()
