#include <algorithm>
#include <array>
#include <cstring>

#include "crypto.h"
#include "crypto.hpp"
#include "crypto_mock.hpp"
#include "c_api_test_support.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

namespace
{
    MATCHER_P(SameBuffer, expected, "")
    {
        return arg.data() == expected.data() && arg.size() == expected.size();
    }

    class CryptoCReal : public ::testing::Test
    {
        protected:
            void TearDown() override
            {
                crypto_c::test::reset_backend();
            }
    };

    class CryptoC : public CryptoCReal
    {
        protected:
            StrictMock<crypto::test::CryptoMock> mock;
            const std::array<uint8_t, CRYPTO_AES_KEY_SIZE> key {};
            const std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> block {};
            std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> output {};

            void SetUp() override
            {
                crypto_c::test::set_backend(mock);
            }
    };

    struct BoundaryBuffers {
            std::array<uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> input;
            std::array<uint8_t, CRYPTO_X25519_SHARED_SECRET_SIZE> output;
            crypto_aes_ctr_drbg_ctx_t ctx;
            size_t length = 123;

            BoundaryBuffers()
            {
                input.fill(0x37);
                output.fill(0xa5);
                std::memset(&ctx, 0x91, sizeof(ctx));
            }
    };

    struct NullPointerCase {
            const char *name;
            sl_status_t (*call)(BoundaryBuffers &);
    };

    class CryptoCNullPointer : public CryptoC, public ::testing::WithParamInterface<NullPointerCase>
    {};

    TEST_P(CryptoCNullPointer, RejectsBeforeDispatchWithoutChangingCallerStorage)
    {
        BoundaryBuffers buffers;
        const auto original = buffers;

        EXPECT_EQ(SL_STATUS_NULL_POINTER, GetParam().call(buffers));
        EXPECT_EQ(original.input, buffers.input);
        EXPECT_EQ(original.output, buffers.output);
        EXPECT_EQ(0, std::memcmp(&original.ctx, &buffers.ctx, sizeof(buffers.ctx)));
        EXPECT_EQ(original.length, buffers.length);
    }

    INSTANTIATE_TEST_SUITE_P(RequiredPointers,
                             CryptoCNullPointer,
                             ::testing::Values(NullPointerCase {"EcbKey",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ecb_encrypt(nullptr, b.input.data(), b.output.data());
                                                                }},
                                               NullPointerCase {"EcbInput",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ecb_encrypt(b.input.data(), nullptr, b.output.data());
                                                                }},
                                               NullPointerCase {"EcbOutput",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ecb_encrypt(b.input.data(), b.input.data(), nullptr);
                                                                }},
                                               NullPointerCase {"CmacKey",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_cmac_compute(nullptr, b.input.data(), 1, b.output.data());
                                                                }},
                                               NullPointerCase {"CmacMessage",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_cmac_compute(b.input.data(), nullptr, 1, b.output.data());
                                                                }},
                                               NullPointerCase {"CmacMac",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_cmac_compute(b.input.data(), b.input.data(), 1, nullptr);
                                                                }},
                                               NullPointerCase {"CbcMacKey",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_cbc_mac(nullptr, b.input.data(), b.input.data(), 1, b.output.data());
                                                                }},
                                               NullPointerCase {"CbcMacIv",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_cbc_mac(b.input.data(), nullptr, b.input.data(), 1, b.output.data());
                                                                }},
                                               NullPointerCase {"CbcMacData",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_cbc_mac(b.input.data(), b.input.data(), nullptr, 1, b.output.data());
                                                                }},
                                               NullPointerCase {"CbcMacMac",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_cbc_mac(b.input.data(), b.input.data(), b.input.data(), 1, nullptr);
                                                                }},
                                               NullPointerCase {"OfbKey",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ofb_xor(nullptr, b.input.data(), b.output.data(), 1);
                                                                }},
                                               NullPointerCase {"OfbIv",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ofb_xor(b.input.data(), nullptr, b.output.data(), 1);
                                                                }},
                                               NullPointerCase {"OfbData",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ofb_xor(b.input.data(), b.input.data(), nullptr, 1);
                                                                }},
                                               NullPointerCase {"DrbgInitContext",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ctr_drbg_init(nullptr, b.input.data(), b.input.data());
                                                                }},
                                               NullPointerCase {"DrbgInitEntropy",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ctr_drbg_init(&b.ctx, nullptr, b.input.data());
                                                                }},
                                               NullPointerCase {"DrbgInitPersonalization",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ctr_drbg_init(&b.ctx, b.input.data(), nullptr);
                                                                }},
                                               NullPointerCase {"DrbgGenerateContext",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ctr_drbg_generate(nullptr, b.output.data());
                                                                }},
                                               NullPointerCase {"DrbgGenerateOutput",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ctr_drbg_generate(&b.ctx, nullptr);
                                                                }},
                                               NullPointerCase {"RandomOutput",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_generate_random(nullptr, 1);
                                                                }},
                                               NullPointerCase {"X25519PublicPrivateKey",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_x25519_derive_public_key(nullptr, b.output.data());
                                                                }},
                                               NullPointerCase {"X25519PublicPublicKey",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_x25519_derive_public_key(b.input.data(), nullptr);
                                                                }},
                                               NullPointerCase {"X25519SecretPrivateKey",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_x25519_derive_shared_secret(nullptr, b.input.data(), b.output.data());
                                                                }},
                                               NullPointerCase {"X25519SecretPeerPublicKey",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_x25519_derive_shared_secret(b.input.data(), nullptr, b.output.data());
                                                                }},
                                               NullPointerCase {"X25519SecretSecret",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_x25519_derive_shared_secret(b.input.data(), b.input.data(), nullptr);
                                                                }},
                                               NullPointerCase {"CcmEncryptKey",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ccm_encrypt(nullptr, 6, b.input.data(), 13, b.input.data(), 1, b.input.data(), 1, b.output.data(), b.output.size(), &b.length);
                                                                }},
                                               NullPointerCase {"CcmEncryptNonce",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ccm_encrypt(b.input.data(), 6, nullptr, 13, b.input.data(), 1, b.input.data(), 1, b.output.data(), b.output.size(), &b.length);
                                                                }},
                                               NullPointerCase {"CcmEncryptAad",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ccm_encrypt(b.input.data(), 6, b.input.data(), 13, nullptr, 1, b.input.data(), 1, b.output.data(), b.output.size(), &b.length);
                                                                }},
                                               NullPointerCase {"CcmEncryptPlaintext",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ccm_encrypt(b.input.data(), 6, b.input.data(), 13, b.input.data(), 1, nullptr, 1, b.output.data(), b.output.size(), &b.length);
                                                                }},
                                               NullPointerCase {"CcmEncryptCiphertext",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ccm_encrypt(b.input.data(), 6, b.input.data(), 13, b.input.data(), 1, b.input.data(), 1, nullptr, b.output.size(), &b.length);
                                                                }},
                                               NullPointerCase {"CcmEncryptLength",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ccm_encrypt(b.input.data(), 6, b.input.data(), 13, b.input.data(), 1, b.input.data(), 1, b.output.data(), b.output.size(), nullptr);
                                                                }},
                                               NullPointerCase {"CcmDecryptKey",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ccm_decrypt(nullptr, 6, b.input.data(), 13, b.input.data(), 1, b.input.data(), 7, b.output.data(), b.output.size(), &b.length);
                                                                }},
                                               NullPointerCase {"CcmDecryptNonce",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ccm_decrypt(b.input.data(), 6, nullptr, 13, b.input.data(), 1, b.input.data(), 7, b.output.data(), b.output.size(), &b.length);
                                                                }},
                                               NullPointerCase {"CcmDecryptAad",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ccm_decrypt(b.input.data(), 6, b.input.data(), 13, nullptr, 1, b.input.data(), 7, b.output.data(), b.output.size(), &b.length);
                                                                }},
                                               NullPointerCase {"CcmDecryptCiphertext",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ccm_decrypt(b.input.data(), 6, b.input.data(), 13, b.input.data(), 1, nullptr, 7, b.output.data(), b.output.size(), &b.length);
                                                                }},
                                               NullPointerCase {"CcmDecryptPlaintext",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ccm_decrypt(b.input.data(), 6, b.input.data(), 13, b.input.data(), 1, b.input.data(), 7, nullptr, b.output.size(), &b.length);
                                                                }},
                                               NullPointerCase {"CcmDecryptLength",
                                                                [](BoundaryBuffers &b) {
                                                                    return crypto_aes_ccm_decrypt(b.input.data(), 6, b.input.data(), 13, b.input.data(), 1, b.input.data(), 7, b.output.data(), b.output.size(), nullptr);
                                                                }}),
                             [](const auto &info) { return info.param.name; });

    class CryptoCEmptyBuffer : public CryptoC, public ::testing::WithParamInterface<bool>
    {
        protected:
            uint8_t *empty_buffer()
            {
                return GetParam() ? nullptr : output.data();
            }
    };

    TEST_P(CryptoCEmptyBuffer, CmacForwardsEmptyMessage)
    {
        EXPECT_CALL(mock, aes_cmac_compute(_, SameBuffer(std::span<const uint8_t>(empty_buffer(), 0)), _)).WillOnce(Return(SL_STATUS_OK));
        EXPECT_EQ(SL_STATUS_OK, crypto_aes_cmac_compute(key.data(), empty_buffer(), 0, output.data()));
    }

    TEST_P(CryptoCEmptyBuffer, CbcMacForwardsEmptyMessage)
    {
        EXPECT_CALL(mock, aes_cbc_mac(_, _, SameBuffer(std::span<const uint8_t>(empty_buffer(), 0)), _)).WillOnce(Return(SL_STATUS_OK));
        EXPECT_EQ(SL_STATUS_OK, crypto_aes_cbc_mac(key.data(), block.data(), empty_buffer(), 0, output.data()));
    }

    TEST_P(CryptoCEmptyBuffer, RandomForwardsEmptyOutput)
    {
        EXPECT_CALL(mock, generate_random(SameBuffer(std::span<uint8_t>(empty_buffer(), 0)))).WillOnce(Return(SL_STATUS_OK));
        EXPECT_EQ(SL_STATUS_OK, crypto_generate_random(empty_buffer(), 0));
    }

    TEST_P(CryptoCEmptyBuffer, OfbSkipsDispatchAndPreservesStorage)
    {
        output.fill(0xa5);
        const auto original = output;
        EXPECT_EQ(SL_STATUS_OK, crypto_aes_ofb_xor(empty_buffer(), empty_buffer(), empty_buffer(), 0));
        EXPECT_EQ(original, output);
    }

    TEST_P(CryptoCEmptyBuffer, CcmEncryptForwardsEmptyBuffersAndInvalidSizeStatus)
    {
        size_t length     = 123;
        const auto input  = std::span<const uint8_t>(empty_buffer(), 0);
        const auto output = std::span<uint8_t>(empty_buffer(), 0);
        EXPECT_CALL(mock, aes_ccm_encrypt(_, 6, SameBuffer(input), SameBuffer(input), SameBuffer(input), SameBuffer(output), _)).WillOnce(Return(SL_STATUS_INVALID_PARAMETER));
        EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, crypto_aes_ccm_encrypt(key.data(), 6, empty_buffer(), 0, empty_buffer(), 0, empty_buffer(), 0, empty_buffer(), 0, &length));
        EXPECT_EQ(123u, length);
    }

    TEST_P(CryptoCEmptyBuffer, CcmDecryptForwardsEmptyBuffersAndInvalidSizeStatus)
    {
        size_t length     = 123;
        const auto input  = std::span<const uint8_t>(empty_buffer(), 0);
        const auto output = std::span<uint8_t>(empty_buffer(), 0);
        EXPECT_CALL(mock, aes_ccm_decrypt(_, 6, SameBuffer(input), SameBuffer(input), SameBuffer(input), SameBuffer(output), _)).WillOnce(Return(SL_STATUS_INVALID_PARAMETER));
        EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, crypto_aes_ccm_decrypt(key.data(), 6, empty_buffer(), 0, empty_buffer(), 0, empty_buffer(), 0, empty_buffer(), 0, &length));
        EXPECT_EQ(123u, length);
    }

    INSTANTIATE_TEST_SUITE_P(EmptyBuffers, CryptoCEmptyBuffer, ::testing::Bool(), [](const auto &info) { return info.param ? "Null" : "NonNull"; });

    TEST_F(CryptoCReal, CcmRoundTripsEmptyPlaintextAndAad)
    {
        const std::array<uint8_t, CRYPTO_AES_KEY_SIZE> key {};
        const std::array<uint8_t, 13> nonce {};
        std::array<uint8_t, 6> ciphertext {};
        size_t length = 123;
        ASSERT_EQ(SL_STATUS_OK, crypto_aes_ccm_encrypt(key.data(), ciphertext.size(), nonce.data(), nonce.size(), nullptr, 0, nullptr, 0, ciphertext.data(), ciphertext.size(), &length));
        ASSERT_EQ(ciphertext.size(), length);
        EXPECT_EQ(SL_STATUS_OK, crypto_aes_ccm_decrypt(key.data(), ciphertext.size(), nonce.data(), nonce.size(), nullptr, 0, ciphertext.data(), ciphertext.size(), nullptr, 0, &length));
        EXPECT_EQ(0u, length);
    }

    TEST_F(CryptoCReal, CcmRejectsEmptyNonceAsInvalidSize)
    {
        const std::array<uint8_t, CRYPTO_AES_KEY_SIZE> key {};
        std::array<uint8_t, 6> ciphertext {};
        size_t length = 123;
        EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, crypto_aes_ccm_encrypt(key.data(), ciphertext.size(), nullptr, 0, nullptr, 0, nullptr, 0, ciphertext.data(), ciphertext.size(), &length));
        EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, crypto_aes_ccm_decrypt(key.data(), ciphertext.size(), nullptr, 0, nullptr, 0, ciphertext.data(), ciphertext.size(), nullptr, 0, &length));
    }

    TEST_F(CryptoC, EcbForwardsBuffersAndFailureWithoutChangingOutput)
    {
        output.fill(0xa5);
        const auto original = output;
        EXPECT_CALL(mock, aes_ecb_encrypt(SameBuffer(std::span(key)), SameBuffer(std::span(block)), SameBuffer(std::span(output)))).WillOnce(Return(SL_STATUS_ALLOCATION_FAILED));

        EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, crypto_aes_ecb_encrypt(key.data(), block.data(), output.data()));
        EXPECT_EQ(original, output);
    }

    TEST_F(CryptoC, CmacForwardsMessageLengthAndOutput)
    {
        const std::array<uint8_t, 7> message {};
        EXPECT_CALL(mock, aes_cmac_compute(SameBuffer(std::span(key)), SameBuffer(std::span(message)), SameBuffer(std::span(output)))).WillOnce([](auto, auto, auto mac) {
            mac[0] = 0x42;
            return SL_STATUS_OK;
        });

        EXPECT_EQ(SL_STATUS_OK, crypto_aes_cmac_compute(key.data(), message.data(), message.size(), output.data()));
        EXPECT_EQ(0x42, output[0]);
    }

    TEST_F(CryptoC, CbcMacForwardsIvAndMessageLength)
    {
        const std::array<uint8_t, UINT8_MAX> message {};
        for (const uint8_t length: {0, 7, UINT8_MAX}) {
            SCOPED_TRACE(length);
            const auto data = std::span(message).first(length);
            EXPECT_CALL(mock, aes_cbc_mac(SameBuffer(std::span(key)), SameBuffer(std::span(block)), SameBuffer(data), SameBuffer(std::span(output)))).WillOnce(Return(SL_STATUS_NOT_SUPPORTED));

            EXPECT_EQ(SL_STATUS_NOT_SUPPORTED, crypto_aes_cbc_mac(key.data(), block.data(), message.data(), length, output.data()));
        }
    }

    TEST_F(CryptoC, OfbForwardsCallerStorageInPlace)
    {
        auto iv                = block;
        const auto original_iv = iv;
        EXPECT_CALL(mock, aes_ofb_xor(SameBuffer(std::span(key)), SameBuffer(std::span(iv)), SameBuffer(std::span(output)))).WillOnce([](auto, auto, auto data) {
            data[0] = 0x42;
            return SL_STATUS_OK;
        });

        EXPECT_EQ(SL_STATUS_OK, crypto_aes_ofb_xor(key.data(), iv.data(), output.data(), output.size()));
        EXPECT_EQ(0x42, output[0]);
        EXPECT_EQ(original_iv, iv);
    }

    TEST_F(CryptoC, CcmEncryptForwardsTagBuffersAndReturnedLength)
    {
        const std::array<uint8_t, 13> nonce {};
        const std::array<uint8_t, 3> aad {};
        const std::array<uint8_t, 7> plaintext {};
        size_t length = 0;
        EXPECT_CALL(mock, aes_ccm_encrypt(SameBuffer(std::span(key)), 6, SameBuffer(std::span(nonce)), SameBuffer(std::span(aad)), SameBuffer(std::span(plaintext)), SameBuffer(std::span(output)), _)).WillOnce([](auto, auto, auto, auto, auto, auto ciphertext, size_t &returned_length) {
            ciphertext[0]   = 0x42;
            returned_length = 13;
            return SL_STATUS_OK;
        });

        EXPECT_EQ(SL_STATUS_OK, crypto_aes_ccm_encrypt(key.data(), 6, nonce.data(), nonce.size(), aad.data(), aad.size(), plaintext.data(), plaintext.size(), output.data(), output.size(), &length));
        EXPECT_EQ(13u, length);
        EXPECT_EQ(0x42, output[0]);
    }

    TEST_F(CryptoC, CcmDecryptForwardsEmptyAadAndInPlaceBuffer)
    {
        const std::array<uint8_t, 13> nonce {};
        size_t length = 123;
        EXPECT_CALL(mock, aes_ccm_decrypt(SameBuffer(std::span(key)), 6, SameBuffer(std::span(nonce)), SameBuffer(std::span<const uint8_t> {}), SameBuffer(std::span(output)), SameBuffer(std::span(output)), _)).WillOnce([](auto, auto, auto, auto, auto, auto, size_t &returned_length) {
            returned_length = 0;
            return SL_STATUS_INVALID_SIGNATURE;
        });

        EXPECT_EQ(SL_STATUS_INVALID_SIGNATURE, crypto_aes_ccm_decrypt(key.data(), 6, nonce.data(), nonce.size(), nullptr, 0, output.data(), output.size(), output.data(), output.size(), &length));
        EXPECT_EQ(0u, length);
    }

    TEST_F(CryptoC, RandomForwardsBufferAndStatus)
    {
        EXPECT_CALL(mock, generate_random(SameBuffer(std::span(output)))).WillOnce(Return(SL_STATUS_FAIL));
        EXPECT_EQ(SL_STATUS_FAIL, crypto_generate_random(output.data(), output.size()));
    }

    TEST_F(CryptoC, X25519ForwardsPrivatePublicAndSecretBuffers)
    {
        const std::array<uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE> private_key {};
        std::array<uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE> public_key {};
        std::array<uint8_t, CRYPTO_X25519_SHARED_SECRET_SIZE> secret {};
        EXPECT_CALL(mock, x25519_derive_public_key(SameBuffer(std::span(private_key)), SameBuffer(std::span(public_key)))).WillOnce([](auto, auto output) {
            output[0] = 0x42;
            return SL_STATUS_OK;
        });
        EXPECT_CALL(mock, x25519_derive_shared_secret(SameBuffer(std::span(private_key)), SameBuffer(std::span(public_key)), SameBuffer(std::span(secret)))).WillOnce(Return(SL_STATUS_NOT_SUPPORTED));

        EXPECT_EQ(SL_STATUS_OK, crypto_x25519_derive_public_key(private_key.data(), public_key.data()));
        EXPECT_EQ(0x42, public_key[0]);
        EXPECT_EQ(SL_STATUS_NOT_SUPPORTED, crypto_x25519_derive_shared_secret(private_key.data(), public_key.data(), secret.data()));
    }

    TEST_F(CryptoC, DrbgInitStartsFreshAndCopiesBothFields)
    {
        const std::array<uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> seed {};
        crypto_aes_ctr_drbg_ctx_t ctx;
        std::memset(&ctx, 0xa5, sizeof(ctx));
        EXPECT_CALL(mock, aes_ctr_drbg_init(_, SameBuffer(std::span(seed)), SameBuffer(std::span(seed)))).WillOnce([](auto &next, auto, auto) {
            EXPECT_THAT(next.key, ::testing::Each(0));
            EXPECT_THAT(next.counter, ::testing::Each(0));
            next.key.fill(0x37);
            next.counter.fill(0x91);
            return SL_STATUS_OK;
        });

        EXPECT_EQ(SL_STATUS_OK, crypto_aes_ctr_drbg_init(&ctx, seed.data(), seed.data()));
        EXPECT_THAT(ctx.key, ::testing::Each(0x37));
        EXPECT_THAT(ctx.counter, ::testing::Each(0x91));
    }

    TEST_F(CryptoC, DrbgInitFailureDoesNotCommitConvertedState)
    {
        const std::array<uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> seed {};
        crypto_aes_ctr_drbg_ctx_t ctx;
        std::memset(&ctx, 0xa5, sizeof(ctx));
        const auto original = ctx;
        EXPECT_CALL(mock, aes_ctr_drbg_init(_, _, _)).WillOnce([](auto &next, auto, auto) {
            next.key.fill(0x37);
            next.counter.fill(0x91);
            return SL_STATUS_ALLOCATION_FAILED;
        });

        EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, crypto_aes_ctr_drbg_init(&ctx, seed.data(), seed.data()));
        EXPECT_EQ(0, std::memcmp(&original, &ctx, sizeof(ctx)));
    }

    TEST_F(CryptoC, DrbgGenerateConvertsStateAndRespectsOutputBoundaries)
    {
        crypto_aes_ctr_drbg_ctx_t ctx;
        std::fill_n(ctx.key, CRYPTO_AES_KEY_SIZE, 0x37);
        std::fill_n(ctx.counter, CRYPTO_AES_BLOCK_SIZE, 0x91);
        std::array<uint8_t, CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE + 2> storage;
        storage.fill(0xa5);
        const auto output = std::span(storage).subspan(1, CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE);
        EXPECT_CALL(mock, aes_ctr_drbg_generate(_, ::testing::SizeIs(CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE))).WillOnce([](auto &next, auto generated) {
            EXPECT_THAT(next.key, ::testing::Each(0x37));
            EXPECT_THAT(next.counter, ::testing::Each(0x91));
            next.key.fill(0x38);
            next.counter.fill(0x92);
            std::fill(generated.begin(), generated.end(), 0x42);
            return SL_STATUS_OK;
        });

        EXPECT_EQ(SL_STATUS_OK, crypto_aes_ctr_drbg_generate(&ctx, output.data()));
        EXPECT_THAT(ctx.key, ::testing::Each(0x38));
        EXPECT_THAT(ctx.counter, ::testing::Each(0x92));
        EXPECT_THAT(output, ::testing::Each(0x42));
        EXPECT_EQ(0xa5, storage.front());
        EXPECT_EQ(0xa5, storage.back());
    }

    TEST_F(CryptoC, DrbgGenerateFailureDoesNotCommitConvertedState)
    {
        crypto_aes_ctr_drbg_ctx_t ctx;
        std::memset(&ctx, 0xa5, sizeof(ctx));
        const auto original = ctx;
        output.fill(0xa5);
        const auto original_output = output;
        EXPECT_CALL(mock, aes_ctr_drbg_generate(_, ::testing::SizeIs(CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE))).WillOnce([](auto &next, auto generated) {
            next.key.fill(0x37);
            next.counter.fill(0x91);
            std::fill(generated.begin(), generated.end(), 0x42);
            return SL_STATUS_INVALID_STATE;
        });

        EXPECT_EQ(SL_STATUS_INVALID_STATE, crypto_aes_ctr_drbg_generate(&ctx, output.data()));
        EXPECT_EQ(0, std::memcmp(&original, &ctx, sizeof(ctx)));
        EXPECT_EQ(original_output, output);
    }

    TEST_F(CryptoCReal, DrbgOutputMayOverlapContextFields)
    {
        const std::array<uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> entropy {1};
        const std::array<uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> personal {2};
        for (const bool output_in_key: {true, false}) {
            SCOPED_TRACE(output_in_key);
            crypto_aes_ctr_drbg_ctx_t c_ctx;
            crypto::CtrDrbgContext cpp_ctx;
            ASSERT_EQ(SL_STATUS_OK, crypto_aes_ctr_drbg_init(&c_ctx, entropy.data(), personal.data()));
            ASSERT_EQ(SL_STATUS_OK, crypto::default_crypto().aes_ctr_drbg_init(cpp_ctx, entropy, personal));
            auto cpp_output = output_in_key ? std::span(cpp_ctx.key) : std::span(cpp_ctx.counter);
            ASSERT_EQ(SL_STATUS_OK, crypto_aes_ctr_drbg_generate(&c_ctx, output_in_key ? c_ctx.key : c_ctx.counter));
            ASSERT_EQ(SL_STATUS_OK, crypto::default_crypto().aes_ctr_drbg_generate(cpp_ctx, cpp_output));
            EXPECT_THAT(c_ctx.key, ::testing::ElementsAreArray(cpp_ctx.key));
            EXPECT_THAT(c_ctx.counter, ::testing::ElementsAreArray(cpp_ctx.counter));
        }
    }

    TEST_F(CryptoC, OverrideDoesNotReplaceTheSharedCoreInstance)
    {
        auto &core = crypto::default_crypto();
        EXPECT_EQ(&core, &crypto::default_crypto());
        EXPECT_CALL(mock, generate_random(_)).WillOnce(Return(SL_STATUS_FAIL));
        EXPECT_EQ(SL_STATUS_FAIL, crypto_generate_random(output.data(), output.size()));
        EXPECT_EQ(SL_STATUS_OK, core.generate_random(output));
        crypto_c::test::reset_backend();
        EXPECT_EQ(SL_STATUS_OK, crypto_generate_random(output.data(), output.size()));
    }
    TEST_F(CryptoCReal, GenerateRejectsNullOutputWithoutAdvancingState)
    {
        crypto_aes_ctr_drbg_ctx_t ctx;
        crypto_aes_ctr_drbg_ctx_t saved_ctx;
        const uint8_t entropy[CRYPTO_AES_CTR_DRBG_SEED_SIZE]         = {1};
        const uint8_t personalization[CRYPTO_AES_CTR_DRBG_SEED_SIZE] = {2};

        ASSERT_EQ(SL_STATUS_OK, crypto_aes_ctr_drbg_init(&ctx, entropy, personalization));
        memcpy(&saved_ctx, &ctx, sizeof(saved_ctx));

        EXPECT_EQ(SL_STATUS_NULL_POINTER, crypto_aes_ctr_drbg_generate(&ctx, nullptr));
        EXPECT_EQ(0, memcmp(&saved_ctx, &ctx, sizeof(ctx)));
    }

    TEST_F(CryptoCReal, GenerateCApiMatchesSixteenByteCppRequest)
    {
        static_assert(CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE == 16);
        crypto_aes_ctr_drbg_ctx_t c_ctx {};
        const std::array<uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> entropy {1};
        const std::array<uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> personal {2};
        ASSERT_EQ(SL_STATUS_OK, crypto_aes_ctr_drbg_init(&c_ctx, entropy.data(), personal.data()));
        crypto::CtrDrbgContext cpp_ctx {};
        ASSERT_EQ(SL_STATUS_OK, crypto::default_crypto().aes_ctr_drbg_init(cpp_ctx, entropy, personal));
        std::array<uint8_t, 16> cpp_output {};

        for (int i = 0; i < 2; ++i) {
            std::array<uint8_t, 18> c_output;
            c_output.fill(0xa5);
            ASSERT_EQ(SL_STATUS_OK, crypto_aes_ctr_drbg_generate(&c_ctx, c_output.data() + 1));
            ASSERT_EQ(SL_STATUS_OK, crypto::default_crypto().aes_ctr_drbg_generate(cpp_ctx, cpp_output));
            EXPECT_THAT(std::span<uint8_t>(c_output).subspan(1, cpp_output.size()), ::testing::ElementsAreArray(cpp_output));
            EXPECT_EQ(0xa5, c_output.front());
            EXPECT_EQ(0xa5, c_output.back());
            EXPECT_THAT(c_ctx.counter, ::testing::ElementsAreArray(cpp_ctx.counter));
            EXPECT_THAT(c_ctx.key, ::testing::ElementsAreArray(cpp_ctx.key));
        }
    }

}  // namespace
