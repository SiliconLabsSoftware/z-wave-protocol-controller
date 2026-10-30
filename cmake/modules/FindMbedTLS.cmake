set(MbedTLS_VERSION 4.2.0)

if(NOT TARGET MbedTLS::tfpsacrypto)
  message(NOTICE "-- Finding Mbed TLS (FetchContent ${MbedTLS_VERSION})")

  include(FetchContent)
  if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
  endif()
  FetchContent_Declare(
    mbedtls
    URL https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-${MbedTLS_VERSION}/mbedtls-${MbedTLS_VERSION}.tar.bz2
    URL_HASH SHA256=2bed9d713b4668f76553b097e72b8aa30bc8f112a940d7ae228d524bbde6ffea)

  set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
  set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(LINK_WITH_PTHREAD ON CACHE BOOL "" FORCE)
  set(TF_PSA_CRYPTO_USER_CONFIG_FILE
      "${CMAKE_CURRENT_LIST_DIR}/../../components/crypto/backend/config/tf_psa_crypto_user_config.h"
      CACHE FILEPATH "" FORCE)
  set(MBEDTLS_USER_CONFIG_FILE
      "${CMAKE_CURRENT_LIST_DIR}/../../components/crypto/backend/config/mbedtls_user_config.h"
      CACHE FILEPATH "" FORCE)

  get_directory_property(_compile_options COMPILE_OPTIONS)
  add_compile_options(-w)

  FetchContent_MakeAvailable(mbedtls)

  set_directory_properties(PROPERTIES COMPILE_OPTIONS "${_compile_options}")
  unset(_compile_options)

  add_library(MbedTLS::tfpsacrypto ALIAS tfpsacrypto)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  MbedTLS
  REQUIRED_VARS MbedTLS_VERSION
  VERSION_VAR MbedTLS_VERSION)
