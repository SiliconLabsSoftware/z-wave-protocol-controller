# PSA-backed S2 crypto (mbedtls). Will absorb kderiv / nextnonce / ctr_drbg in step 4.
# The legacy `s2crypto` target from zw-libs2/crypto remains until step 6; this target is
# `s2crypto_psa` until then to avoid a CMake name clash.

find_package(MbedTLS REQUIRED)

add_library(s2crypto_psa INTERFACE)
target_link_libraries(s2crypto_psa INTERFACE MbedTLS::tfpsacrypto)

if(UNIT_TEST)
  add_subdirectory(test)
endif()
