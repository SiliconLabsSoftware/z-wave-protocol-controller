# This is for generating include/zpc_version.h to add to source package
configure_file(${CMAKE_CURRENT_LIST_DIR}/../../components/common/include/zpc_version.h.in
    ${CMAKE_BINARY_DIR}/include/zpc_version.h)

if(NOT TARGET zpc_version)
    add_library(zpc_version INTERFACE IMPORTED GLOBAL)
    target_include_directories(zpc_version INTERFACE ${CMAKE_BINARY_DIR}/include)
    add_dependencies(zpc_version ${CMAKE_BINARY_DIR}/include/zpc_version.h)
endif()
