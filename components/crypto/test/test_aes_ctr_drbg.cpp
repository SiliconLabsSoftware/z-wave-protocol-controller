/* © 2017 Silicon Laboratories Inc.
 */
/**
 * @file
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, __data OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
#include <array>
#include <cstdint>
#include <cstring>

#include <gtest/gtest.h>

#include "crypto.hpp"
#include "test_fixture.hpp"

// Different entropy inputs produce different DRBG output.
TEST_F(AesCtrDrbg, DifferentEntropyYieldsDifferentOutput)
{
    crypto_aes_ctr_drbg_ctx_t ctx;
    const uint8_t personalization[CRYPTO_AES_CTR_DRBG_SEED_SIZE] = {0};
    uint8_t entropy_a[32]                                        = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
    uint8_t entropy_b[32];
    uint8_t out_a[CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE];
    uint8_t out_b[CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE];

    memcpy(entropy_b, entropy_a, sizeof(entropy_b));
    entropy_b[0] ^= 0xff;

    ASSERT_EQ(SL_STATUS_OK, crypto_aes_ctr_drbg_init(&ctx, entropy_a, personalization));
    ASSERT_EQ(SL_STATUS_OK, crypto_aes_ctr_drbg_generate(&ctx, out_a));

    ASSERT_EQ(SL_STATUS_OK, crypto_aes_ctr_drbg_init(&ctx, entropy_b, personalization));
    ASSERT_EQ(SL_STATUS_OK, crypto_aes_ctr_drbg_generate(&ctx, out_b));

    EXPECT_NE(0, memcmp(out_a, out_b, CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE));
}

// NULL output on generate is rejected and internal state is unchanged.
TEST_F(AesCtrDrbg, GenerateRejectsNullOutputWithoutAdvancingState)
{
    crypto_aes_ctr_drbg_ctx_t ctx;
    crypto_aes_ctr_drbg_ctx_t saved_ctx;
    const uint8_t entropy[CRYPTO_AES_CTR_DRBG_SEED_SIZE]         = {1};
    const uint8_t personalization[CRYPTO_AES_CTR_DRBG_SEED_SIZE] = {2};

    ASSERT_EQ(SL_STATUS_OK, crypto_aes_ctr_drbg_init(&ctx, entropy, personalization));
    memcpy(&saved_ctx, &ctx, sizeof(saved_ctx));

    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, crypto_aes_ctr_drbg_generate(&ctx, nullptr));
    EXPECT_EQ(0, memcmp(&saved_ctx, &ctx, sizeof(ctx)));
}

TEST_F(AesCtrDrbg, RejectsNullArgumentsWithoutChangingContext)
{
    crypto_aes_ctr_drbg_ctx_t ctx {};
    std::memset(&ctx, 0xa5, sizeof(ctx));
    const auto original = ctx;
    std::array<uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> seed {};
    std::array<uint8_t, CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE> output {};
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, crypto_aes_ctr_drbg_init(nullptr, seed.data(), seed.data()));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, crypto_aes_ctr_drbg_init(&ctx, nullptr, seed.data()));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, crypto_aes_ctr_drbg_init(&ctx, seed.data(), nullptr));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, crypto_aes_ctr_drbg_generate(nullptr, output.data()));
    EXPECT_EQ(0, std::memcmp(&original, &ctx, sizeof(ctx)));
}

TEST_F(AesCtrDrbg, GenerateCApiMatchesSixteenByteCppRequest)
{
    static_assert(CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE == 16);
    crypto_aes_ctr_drbg_ctx_t c_ctx {};
    const std::array<uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> entropy {1};
    const std::array<uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> personal {2};
    ASSERT_EQ(SL_STATUS_OK, crypto_aes_ctr_drbg_init(&c_ctx, entropy.data(), personal.data()));
    auto cpp_ctx = c_ctx;
    std::array<uint8_t, 16> cpp_output {};

    for (int i = 0; i < 2; ++i) {
        std::array<uint8_t, 18> c_output;
        c_output.fill(0xa5);
        ASSERT_EQ(SL_STATUS_OK, crypto_aes_ctr_drbg_generate(&c_ctx, c_output.data() + 1));
        ASSERT_EQ(SL_STATUS_OK, crypto::c_api().aes_ctr_drbg_generate(cpp_ctx, cpp_output));
        EXPECT_THAT(std::span<uint8_t>(c_output).subspan(1, cpp_output.size()), ::testing::ElementsAreArray(cpp_output));
        EXPECT_EQ(0xa5, c_output.front());
        EXPECT_EQ(0xa5, c_output.back());
        EXPECT_EQ(0, std::memcmp(&c_ctx, &cpp_ctx, sizeof(c_ctx)));
    }
}

#include "test_crypto_injection_fixture.hpp"

TEST_F(AesCtrDrbgInjection, InitKeepsContextWhenNestedEcbFails)
{
    EXPECT_CALL(mock, cipher_encrypt(_, PSA_ALG_ECB_NO_PADDING, _, _, _)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::cipher_encrypt));
    EXPECT_CALL(mock, destroy_key(_)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::destroy_key));
    const std::array<uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> seed {};
    crypto_aes_ctr_drbg_ctx_t ctx {};
    ctx.key[0]          = 0xa5;
    const auto original = ctx;
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, ::testing::SizeIs(CRYPTO_AES_KEY_SIZE), _, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::import_key)).WillOnce(Return(PSA_ERROR_INSUFFICIENT_MEMORY));

    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_ctr_drbg_init(ctx, seed, seed));
    EXPECT_EQ(0, std::memcmp(&original, &ctx, sizeof(ctx)));
}

class AesCtrDrbgGenerateFailure : public AesCtrDrbgInjection, public ::testing::WithParamInterface<int>
{};

TEST_P(AesCtrDrbgGenerateFailure, KeepsContextAndOutputOnFailure)
{
    const int successful_blocks = GetParam();
    crypto_aes_ctr_drbg_ctx_t ctx {};
    ctx.counter[0]      = 0x42;
    const auto original = ctx;
    std::array<uint8_t, CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE> generated;
    generated.fill(0xa5);
    const auto original_output = generated;
    EXPECT_CALL(mock, cipher_encrypt(_, PSA_ALG_ECB_NO_PADDING, _, _, _)).Times(successful_blocks).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::cipher_encrypt));
    EXPECT_CALL(mock, destroy_key(_)).Times(successful_blocks).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::destroy_key));
    int imports = 0;
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, ::testing::SizeIs(CRYPTO_AES_KEY_SIZE), _, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS))
      .Times(successful_blocks + 1)
      .WillRepeatedly([&](psa_key_usage_t usage, psa_algorithm_t algorithm, std::span<const uint8_t> data, psa_key_id_t &key_id, psa_key_type_t type, size_t bits) {
          if (imports++ == successful_blocks) {
              return PSA_ERROR_INSUFFICIENT_MEMORY;
          }
          return real_psa_wrappers.import_key(usage, algorithm, data, key_id, type, bits);
      });

    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_ctr_drbg_generate(ctx, generated));
    EXPECT_EQ(0, std::memcmp(&original, &ctx, sizeof(ctx)));
    EXPECT_EQ(original_output, generated);
}

// Fail in the output block or either block of the final state update.
INSTANTIATE_TEST_SUITE_P(OutputAndUpdateBlocks, AesCtrDrbgGenerateFailure, ::testing::Values(0, 1, 2));

TEST_F(AesCtrDrbgInjection, RejectsWrongOutputSizeBeforeCallingPsaWrappers)
{
    crypto_aes_ctr_drbg_ctx_t ctx {};
    const auto original = ctx;
    std::array<uint8_t, CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE + 1> generated;
    generated.fill(0xa5);
    const auto original_output = generated;

    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, algorithms.aes_ctr_drbg_generate(ctx, {}));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, algorithms.aes_ctr_drbg_generate(ctx, generated));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, algorithms.aes_ctr_drbg_generate(ctx, std::span<uint8_t>(generated).first(CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE - 1)));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, algorithms.aes_ctr_drbg_generate(ctx, std::span<uint8_t>(generated).first(0)));
    EXPECT_EQ(0, std::memcmp(&original, &ctx, sizeof(ctx)));
    EXPECT_EQ(original_output, generated);
}
