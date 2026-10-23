SET(lib_source_files
s2_event_handler_mock.c
s2_inclusion_extern_mock.c
s2_keystore_mock.c
s2_dbg_printf.c
)

add_library(s2_inclusion_mocks ${lib_source_files})
target_link_libraries(s2_inclusion_mocks PUBLIC mock crypto_core)

# Kept separate from s2_inclusion_mocks: linking it into a test that also pulls
# the real key-derivation object (e.g. test_span) gives a duplicate zwave_s2_extract_pseudo_random_key on GNU ld.
add_library(crypto_mocks kderiv_mock.c)
target_link_libraries(crypto_mocks mock common)
target_include_directories(crypto_mocks PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include)

