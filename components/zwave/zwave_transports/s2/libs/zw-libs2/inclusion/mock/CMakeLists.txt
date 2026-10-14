SET(lib_source_files
s2_event_handler_mock.c
s2_inclusion_extern_mock.c
s2_keystore_mock.c
s2_dbg_printf.c
)

add_library(s2_inclusion_mocks ${lib_source_files})
target_link_libraries(s2_inclusion_mocks mock)

# Kept separate from s2_inclusion_mocks: linking it into a test that also pulls
# the real kderiv.o (e.g. test_span) gives a duplicate tempkey_extract on GNU ld.
add_library(crypto_mocks kderiv_mock.c)
target_link_libraries(crypto_mocks mock)

