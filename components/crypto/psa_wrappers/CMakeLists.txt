find_package(MbedTLS REQUIRED)

add_library(psa_wrappers STATIC src/psa_wrappers.cpp)
target_include_directories(psa_wrappers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(psa_wrappers PUBLIC MbedTLS::tfpsacrypto)
set_target_properties(psa_wrappers PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(UNIT_TEST)
  add_subdirectory(test)
endif()
