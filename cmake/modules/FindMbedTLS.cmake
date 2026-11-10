set(MbedTLS_VERSION 4.2.0)

set(TF_PSA_CRYPTO_USER_CONFIG_FILE
    "${CMAKE_CURRENT_LIST_DIR}/../../components/crypto/config/tf_psa_crypto_user_config.h"
    CACHE FILEPATH "TF-PSA-Crypto user config file (appended to default).")
set(MBEDTLS_USER_CONFIG_FILE
    "${CMAKE_CURRENT_LIST_DIR}/../../components/crypto/config/mbedtls_user_config.h"
    CACHE FILEPATH "Mbed TLS user config file (appended to default).")

if(NOT TARGET MbedTLS::tfpsacrypto)
  message(NOTICE "-- Finding Mbed TLS (FetchContent ${MbedTLS_VERSION})")

  include(FetchContent)
  include(${CMAKE_CURRENT_LIST_DIR}/../include/external_compiler_flags.cmake)
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

  zpc_push_external_compiler_flags()
  FetchContent_MakeAvailable(mbedtls)
  zpc_pop_external_compiler_flags()

  add_library(MbedTLS::tfpsacrypto ALIAS tfpsacrypto)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  MbedTLS
  REQUIRED_VARS MbedTLS_VERSION
  VERSION_VAR MbedTLS_VERSION)
