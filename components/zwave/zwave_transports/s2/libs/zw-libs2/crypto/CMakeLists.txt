set(CURVE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/curve25519" PARENT_SCOPE)
include_directories(${CURVE_INCLUDE_DIR})

add_compile_definitions(
  CCM_USE_PREDEFINED_VALUES
)

set_source_files_properties(kderiv/kderiv.c PROPERTIES COMPILE_FLAGS
                                                       -Wno-unused-parameter)

set(CRYPTO_SRC
  aes-cmac/aes_cmac.c
  ccm/ccm.c
  ctr_drbg/ctr_drbg.c
  curve25519/base.c
  curve25519/smult.c
  kderiv/kderiv.c
  nextnonce/nextnonce.c)

add_library(aes aes/aes.c)
add_library(s2crypto ${CRYPTO_SRC})
target_compile_definitions(s2crypto PUBLIC "DllExport=extern")
if(WIN32)
  add_library(s2cryptoShared SHARED ${CRYPTO_SRC} aes/aes.c)
endif()

target_include_directories(s2crypto PUBLIC "${CURVE_INCLUDE_DIR}" ../include)
