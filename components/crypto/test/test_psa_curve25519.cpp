/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

#include "test_fixture.hpp"
#include "x25519.hpp"

#define KEY_SIZE 32U

static const uint8_t alice_secret_key[KEY_SIZE] = {0x77, 0x07, 0x6d, 0x0a, 0x73, 0x18, 0xa5, 0x7d, 0x3c, 0x16, 0xc1, 0x72, 0x51, 0xb2, 0x66, 0x45, 0xdf, 0x4c, 0x2f, 0x87, 0xeb, 0xc0, 0x99, 0x2a, 0xb1, 0x77, 0xfb, 0xa5, 0x1d, 0xb9, 0x2c, 0x2a};

static const uint8_t bob_secret_key[KEY_SIZE] = {0x5d, 0xab, 0x08, 0x7e, 0x62, 0x4a, 0x8a, 0x4b, 0x79, 0xe1, 0x7f, 0x8b, 0x83, 0x80, 0x0e, 0xe6, 0x6f, 0x3b, 0xb1, 0x29, 0x26, 0x18, 0xb6, 0xfd, 0x1c, 0x2f, 0x8b, 0x27, 0xff, 0x88, 0xe0, 0xeb};

namespace
{
    void ExpectBytesEq(const uint8_t *expected, const uint8_t *actual, size_t len)
    {
        EXPECT_THAT(std::vector<uint8_t>(actual, actual + len), ::testing::ElementsAreArray(expected, len));
    }
}  // namespace

// RFC 7748 Alice public key from fixed private key.
TEST_F(Crypto, PsaX25519AlicePublicKey)
{
    const uint8_t expected_alice_public_key[KEY_SIZE] = {0x85, 0x20, 0xf0, 0x09, 0x89, 0x30, 0xa7, 0x54, 0x74, 0x8b, 0x7d, 0xdc, 0xb4, 0x3e, 0xf7, 0x5a, 0x0d, 0xbf, 0x3a, 0x0d, 0x26, 0x38, 0x1a, 0xf4, 0xeb, 0xa4, 0xa9, 0x8e, 0xaa, 0x9b, 0x4e, 0x6a};
    uint8_t alice_public_key[KEY_SIZE];

    ASSERT_EQ(SL_STATUS_OK, crypto_x25519_derive_public_key(alice_secret_key, alice_public_key));
    ExpectBytesEq(expected_alice_public_key, alice_public_key, KEY_SIZE);
}

// RFC 7748 Bob public key from fixed private key.
TEST_F(Crypto, PsaX25519BobPublicKey)
{
    const uint8_t expected_bob_public_key[KEY_SIZE] = {0xde, 0x9e, 0xdb, 0x7d, 0x7b, 0x7d, 0xc1, 0xb4, 0xd3, 0x5b, 0x61, 0xc2, 0xec, 0xe4, 0x35, 0x37, 0x3f, 0x83, 0x43, 0xc8, 0x5b, 0x78, 0x67, 0x4d, 0xad, 0xfc, 0x7e, 0x14, 0x6f, 0x88, 0x2b, 0x4f};
    uint8_t bob_public_key[KEY_SIZE];

    ASSERT_EQ(SL_STATUS_OK, crypto_x25519_derive_public_key(bob_secret_key, bob_public_key));
    ExpectBytesEq(expected_bob_public_key, bob_public_key, KEY_SIZE);
}

// RFC 7748 shared secret matches for Alice and Bob.
TEST_F(Crypto, PsaX25519SharedSecretRfc7748)
{
    const uint8_t expected_shared_secret[KEY_SIZE] = {0x4a, 0x5d, 0x9d, 0x5b, 0xa4, 0xce, 0x2d, 0xe1, 0x72, 0x8e, 0x3b, 0xf4, 0x80, 0x35, 0x0f, 0x25, 0xe0, 0x7e, 0x21, 0xc9, 0x47, 0xd1, 0x9e, 0x33, 0x76, 0xf0, 0x9b, 0x3c, 0x1e, 0x16, 0x17, 0x42};
    uint8_t alice_public_key[KEY_SIZE];
    uint8_t bob_public_key[KEY_SIZE];
    uint8_t shared_alice[KEY_SIZE];
    uint8_t shared_bob[KEY_SIZE];

    ASSERT_EQ(SL_STATUS_OK, crypto_x25519_derive_public_key(alice_secret_key, alice_public_key));
    ASSERT_EQ(SL_STATUS_OK, crypto_x25519_derive_public_key(bob_secret_key, bob_public_key));

    ASSERT_EQ(SL_STATUS_OK, crypto_x25519_derive_shared_secret(alice_secret_key, bob_public_key, shared_alice));
    ASSERT_EQ(SL_STATUS_OK, crypto_x25519_derive_shared_secret(bob_secret_key, alice_public_key, shared_bob));

    ExpectBytesEq(expected_shared_secret, shared_alice, KEY_SIZE);
    ExpectBytesEq(expected_shared_secret, shared_bob, KEY_SIZE);
}

// C++ X25519 API derives the RFC 7748 shared secret.
TEST_F(Crypto, X25519DeriveSharedSecretCppApi)
{
    std::array<uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE> alice_priv {};
    memcpy(alice_priv.data(), alice_secret_key, alice_priv.size());
    std::array<uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE> bob_pub {};
    std::array<uint8_t, CRYPTO_X25519_SHARED_SECRET_SIZE> shared {};
    const uint8_t expected_shared_secret[KEY_SIZE] = {0x4a, 0x5d, 0x9d, 0x5b, 0xa4, 0xce, 0x2d, 0xe1, 0x72, 0x8e, 0x3b, 0xf4, 0x80, 0x35, 0x0f, 0x25, 0xe0, 0x7e, 0x21, 0xc9, 0x47, 0xd1, 0x9e, 0x33, 0x76, 0xf0, 0x9b, 0x3c, 0x1e, 0x16, 0x17, 0x42};

    ASSERT_EQ(SL_STATUS_OK, crypto_x25519_derive_public_key(bob_secret_key, bob_pub.data()));
    ASSERT_EQ(SL_STATUS_OK, crypto::x25519_derive_shared_secret(alice_priv, bob_pub, shared));
    ExpectBytesEq(expected_shared_secret, shared.data(), shared.size());
}

TEST_F(Crypto, X25519RejectsLowOrderPeerPublicKeys)
{
    // u=0 and u=1 produce an all-zero shared secret and must be rejected.
    for (const uint8_t u: {0, 1}) {
        SCOPED_TRACE(u);
        std::array<uint8_t, KEY_SIZE> peer {};
        std::array<uint8_t, KEY_SIZE> secret {};
        peer[0] = u;
        EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, crypto_x25519_derive_shared_secret(alice_secret_key, peer.data(), secret.data()));
        ASSERT_EQ(SL_STATUS_OK, crypto_x25519_derive_public_key(bob_secret_key, peer.data()));
        EXPECT_EQ(SL_STATUS_OK, crypto_x25519_derive_shared_secret(alice_secret_key, peer.data(), secret.data()));
    }
}
