# Crypto architecture

## Role

`crypto` implements the cryptographic operations ZPC needs for Z-Wave S0 and S2 security: AES-128 ECB, CCM, CMAC, OFB, and the S0 CBC-MAC; AES-128 CTR-DRBG; X25519 key agreement; and random generation. It is a C++ library built on the PSA Crypto API, implemented by TF-PSA-Crypto. S0 and S2 are written in C and use it through the [`crypto_c` adapter](../../crypto_c/docs/ARCHITECTURE.md).

Z-Wave protocol logic, network key storage, and S2 key derivation stay in the S0 and S2 modules.

## Structure

```text
S0 / S2 -> crypto_c -> crypto -> TF-PSA-Crypto
```

- `include/crypto.hpp` defines `CryptoInterface`, byte sizes, and the CTR-DRBG context. The interface uses `std::span` and has no PSA types.
- `src/crypto_impl.hpp` declares `Crypto`, the implementation of `CryptoInterface`. Its methods are defined in `src/algorithms/`, one file per operation.
- `src/default_crypto.cpp` defines `default_crypto()`, which builds `Crypto` on top of the real PSA wrappers.
- `src/psa/psa_wrappers.hpp` defines `PsaWrappersInterface`, a span-based interface covering the PSA functions `Crypto` uses. `PsaWrappers` in `psa_wrappers_impl.cpp` forwards each method to the matching PSA function.
- `src/psa/psa_resources.hpp` provides scope guards: `ScopedKey` destroys an imported key and `ScopedCipherOperation` aborts an unfinished cipher operation.
- `src/psa/psa_helpers.hpp` converts PSA status codes to `sl_status_t`.

`Crypto` receives a `PsaWrappersInterface` reference at construction and does not own it. Algorithm code never calls PSA directly, so tests can replace PSA with a mock.

## Call flow

`default_crypto()` creates one shared `Crypto` instance and its `PsaWrappers` on first use. Both live until program exit. Calls run synchronously in the caller's thread, using buffers owned by the caller.

There is no separate initialization step. Operations call `psa_crypto_init()` before using PSA; PSA initializes on the first call, and later calls return immediately. Production code never shuts PSA down.

Keys are imported as volatile PSA keys for a single call and never persisted. `ScopedKey` destroys them, and `ScopedCipherOperation` aborts unfinished cipher operations, on every return path, so no key or operation stays in PSA after a call returns.

`psa_status_to_sl_status()` maps a fixed set of PSA statuses to their `sl_status_t` counterparts, for example `PSA_ERROR_INVALID_SIGNATURE` to `SL_STATUS_INVALID_SIGNATURE` and `PSA_ERROR_BUFFER_TOO_SMALL` to `SL_STATUS_WOULD_OVERFLOW`. Any other PSA error, and any PSA output length that differs from the expected size, returns `SL_STATUS_FAIL`.

CTR-DRBG is AES-128 CTR-DRBG without a derivation function, reseeding, or additional input, as used by S2 for nonce generation. Both ends of an S2 session must produce the same nonce sequence from the same seed, which the PSA random generator cannot do, so `crypto` implements CTR-DRBG itself on top of `aes_ecb_encrypt()`. The caller owns the key and counter context, and `crypto` keeps no DRBG state between calls. The context and output are written only on success. Working buffers in CTR-DRBG and CBC-MAC are zeroed on exit with `common::ScopedZeroize`.

`generate_random()` uses the PSA random generator and is the source for non-deterministic random bytes.

`Crypto` holds no mutable state, and TF-PSA-Crypto is built with `MBEDTLS_THREADING_PTHREAD`, so PSA protects its shared state with mutexes. Callers must not share a CTR-DRBG context or an output buffer between threads without their own locking.

## Integration

`crypto_c` is currently the only production consumer. A C++ consumer would include `crypto.hpp` and link the `crypto` CMake target for `default_crypto()`. Code that only needs `CryptoInterface` can link `crypto_api`, which provides the header and `common` but no implementation, and receive an implementation from its caller.

`crypto` is a static library. It exposes `crypto_api` and links TF-PSA-Crypto privately, so PSA headers are not visible to consumers.

[`cmake/modules/FindTF-PSA-Crypto.cmake`](../../../cmake/modules/FindTF-PSA-Crypto.cmake) downloads the TF-PSA-Crypto release pinned there (version and SHA-256) with FetchContent at configure time. It skips the download when a `TF-PSA-Crypto::tfpsacrypto` target already exists. TF-PSA-Crypto is built with its own compiler settings, not ZPC's warnings and sanitizer flags, so sanitizer builds do not instrument it. [`config/tf_psa_crypto_user_config.h`](../config/tf_psa_crypto_user_config.h) is applied on top of the TF-PSA-Crypto defaults. It enables pthread threading and disables the algorithms and key types ZPC does not use.

Function contracts are documented with Doxygen in [`include/crypto.hpp`](../include/crypto.hpp).

## Tests

With `UNIT_TEST` enabled, `test/` checks `default_crypto()` against published vectors (NIST SP 800-38A, RFC 4493, RFC 7748), a reference S0 CBC-MAC computation, and precomputed CTR-DRBG state and output values. `PsaWrappers` and the status conversion are tested directly against TF-PSA-Crypto. A mock `PsaWrappersInterface` checks error propagation and that `ScopedKey` and `ScopedCipherOperation` release resources exactly once. Test fixtures call `mbedtls_psa_crypto_free()` in teardown, so each test starts with fresh PSA state.

The `crypto_test_support` target provides `crypto::test::CryptoMock` in `test/support/crypto_mock.hpp` for tests of consuming modules.
