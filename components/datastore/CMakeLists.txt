find_package(SQLite3 3.53.0 REQUIRED)

# ZPC datastore library core (OBJECT library for shared usage)
add_library(datastore OBJECT src/datastore.c src/datastore_attributes.c
                                 src/datastore_fixt.c src/datastore_internals.c)
target_include_directories(
  datastore
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)
target_link_libraries(
  datastore
  PUBLIC log config
  PRIVATE SQLite::SQLite3 zpc_version)

target_compile_definitions(
  datastore PRIVATE CMAKE_PROJECT_VERSION="${CMAKE_PROJECT_VERSION}")

# ZPC datastore library (wrapper around datastore)
add_library(zpc_datastore src/zpc_datastore.c)
target_link_libraries(zpc_datastore PUBLIC datastore config)
target_include_directories(zpc_datastore PUBLIC include)

