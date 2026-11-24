# Crypto C architecture

## Role

`crypto_c` is the C interface to the cryptographic operations implemented by [`crypto`](../../crypto/docs/ARCHITECTURE.md). It adapts C calls to the C++ `CryptoInterface`. The Z-Wave S0 and S2 modules, which are written in C, use it for encryption, authentication, key agreement, S2 nonce generation, and random generation.

`crypto_c` contains no cryptographic algorithms. Z-Wave protocol logic, network key storage, and S2 key derivation stay in the S0 and S2 modules.

## Structure

```text
S0 / S2 -> crypto_c -> crypto -> TF-PSA-Crypto
```

- `include/crypto.h` defines the C functions, byte sizes, and CTR-DRBG context. The header has no C++ or PSA types. It covers every `CryptoInterface` operation except AES-128 ECB decryption.
- `src/crypto_impl.cpp` implements the C functions. It checks pointers, wraps arguments in `std::span`, and forwards calls through `crypto_c::backend()`. Compile-time checks keep the C and C++ byte sizes equal.
- `src/c_api_backend.cpp` defines `crypto_c::backend()`. It returns the object from `crypto_c::backend_override()` when that is not null, and `crypto::default_crypto()` otherwise.
- `src/c_api_override_default.cpp` defines `backend_override()` to return null. CMake compiles it into builds without `UNIT_TEST`, so those builds always use `default_crypto()`.

The implementation is compiled as C++ and exports its functions with C linkage.

## Call flow

The adapter rejects a null required pointer with `SL_STATUS_NULL_POINTER` without calling the backend. A data pointer may be null when its length is zero, and `crypto_aes_ofb_xor()` with zero length returns `SL_STATUS_OK` without checking any pointer. Otherwise, the adapter forwards the call to the selected backend and returns its `sl_status_t` unchanged. Calls run synchronously in the caller's thread, using buffers owned by the caller.

In builds without `UNIT_TEST`, the backend is the shared `Crypto` instance created by `crypto::default_crypto()` on first use. PSA initialization, key import, and resource cleanup are handled by `crypto`.

The C and C++ CTR-DRBG contexts have different types, so CTR-DRBG calls copy the C context into a temporary C++ context, call the backend, and copy the updated state and generated output back only on success. The context is written before the output, so if the output buffer overlaps the context, the output bytes take precedence. Both temporaries are cleared with `secure_memzero()` before returning.

S2 keeps one CTR-DRBG context per span in its span table, so each peer has its own nonce sequence.

## Integration

C consumers include `crypto.h` and link the `crypto_c` CMake target. Current consumers are the S0 transport, the libs2 S2 protocol and inclusion code, and the ZPC S2 inclusion controller, keystore, key derivation, and next-nonce modules.

`crypto_c` is a static library. It exposes the C header and `common`, and links `crypto` privately, so consumers cannot include `crypto.hpp` through it. Because the library is static, CMake still passes `crypto` and TF-PSA-Crypto to the final link.

Function contracts are documented with Doxygen in [`include/crypto.h`](../include/crypto.h).

## Tests

With `UNIT_TEST` enabled, CMake compiles `test/support/c_api_override_test.cpp` into `crypto_c` in place of the default override source. Every test executable that links `crypto_c`, directly or through another component, gets this version. It returns null until a test sets a backend, so calls use `crypto::default_crypto()` by default.

Tests link `crypto_c_test_support` and call `crypto_c::test::set_backend()` to route C calls to a `CryptoInterface` mock, usually `crypto::test::CryptoMock`. S2 key derivation failure tests use this to inject backend errors. The override is a single unsynchronized pointer that does not own the backend, so tests call `crypto_c::test::reset_backend()` before the mock is destroyed.

`test/` checks pointer validation, argument forwarding, status forwarding, and CTR-DRBG state handling, including overlapping output. Tests using the real backend also compare C and C++ CTR-DRBG results.
