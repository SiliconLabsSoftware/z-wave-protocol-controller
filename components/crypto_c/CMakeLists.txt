set(CRYPTO_C_SOURCES src/c_api.cpp src/c_api_backend.cpp)
if(UNIT_TEST)
  list(APPEND CRYPTO_C_SOURCES test/support/c_api_override_test.cpp)
else()
  list(APPEND CRYPTO_C_SOURCES src/c_api_override_none.cpp)
endif()

add_library(crypto_c STATIC ${CRYPTO_C_SOURCES})
target_include_directories(
  crypto_c
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(crypto_c PUBLIC common PRIVATE crypto)
set_target_properties(crypto_c PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(UNIT_TEST)
  add_subdirectory(test)
endif()
