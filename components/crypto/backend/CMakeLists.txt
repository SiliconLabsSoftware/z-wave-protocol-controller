find_package(MbedTLS REQUIRED)

add_library(crypto_backend STATIC src/psa_wrappers.cpp)
target_include_directories(crypto_backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(crypto_backend PUBLIC MbedTLS::tfpsacrypto)
set_target_properties(crypto_backend PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(UNIT_TEST)
  add_subdirectory(test)
endif()
