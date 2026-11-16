set(TF-PSA-Crypto_VERSION 1.2.0)

set(TF_PSA_CRYPTO_USER_CONFIG_FILE
    "${CMAKE_CURRENT_LIST_DIR}/../../components/crypto/config/tf_psa_crypto_user_config.h"
    CACHE FILEPATH "TF-PSA-Crypto user config file (appended to default).")

if(NOT TARGET TF-PSA-Crypto::tfpsacrypto)
  message(NOTICE "-- Finding TF-PSA-Crypto (FetchContent ${TF-PSA-Crypto_VERSION})")

  include(FetchContent)
  include(${CMAKE_CURRENT_LIST_DIR}/../include/external_compiler_flags.cmake)
  if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
  endif()
  FetchContent_Declare(
    tf-psa-crypto
    URL https://github.com/Mbed-TLS/TF-PSA-Crypto/releases/download/tf-psa-crypto-${TF-PSA-Crypto_VERSION}/tf-psa-crypto-${TF-PSA-Crypto_VERSION}.tar.bz2
    URL_HASH SHA256=4de95b3719874625ed4649bfc824b9271c13fc358ca88d5e227134469ad02007)

  set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
  set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(LINK_WITH_PTHREAD ON CACHE BOOL "" FORCE)

  zpc_push_external_compiler_flags()
  FetchContent_MakeAvailable(tf-psa-crypto)
  zpc_pop_external_compiler_flags()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  TF-PSA-Crypto
  REQUIRED_VARS TF-PSA-Crypto_VERSION
  VERSION_VAR TF-PSA-Crypto_VERSION)
