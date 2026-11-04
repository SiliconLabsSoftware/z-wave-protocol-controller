/* © 2017 Silicon Laboratories Inc.
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
#include "test_crypto_injection_fixture.hpp"

TEST_F(AesCtrDrbg, DifferentEntropyYieldsDifferentOutput)
{
    crypto::CtrDrbgContext ctx;
    const uint8_t personalization[crypto::aes_ctr_drbg_seed_size] = {0};
    uint8_t entropy_a[32]                                         = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
    uint8_t entropy_b[32];
    uint8_t out_a[crypto::aes_ctr_drbg_output_size];
    uint8_t out_b[crypto::aes_ctr_drbg_output_size];

    memcpy(entropy_b, entropy_a, sizeof(entropy_b));
    entropy_b[0] ^= 0xff;

    ASSERT_EQ(SL_STATUS_OK, crypto::default_crypto().aes_ctr_drbg_init(ctx, std::span<const uint8_t, crypto::aes_ctr_drbg_seed_size>(entropy_a, crypto::aes_ctr_drbg_seed_size), std::span<const uint8_t, crypto::aes_ctr_drbg_seed_size>(personalization, crypto::aes_ctr_drbg_seed_size)));
    ASSERT_EQ(SL_STATUS_OK, crypto::default_crypto().aes_ctr_drbg_generate(ctx, std::span<uint8_t, crypto::aes_ctr_drbg_output_size>(out_a, crypto::aes_ctr_drbg_output_size)));

    ASSERT_EQ(SL_STATUS_OK, crypto::default_crypto().aes_ctr_drbg_init(ctx, std::span<const uint8_t, crypto::aes_ctr_drbg_seed_size>(entropy_b, crypto::aes_ctr_drbg_seed_size), std::span<const uint8_t, crypto::aes_ctr_drbg_seed_size>(personalization, crypto::aes_ctr_drbg_seed_size)));
    ASSERT_EQ(SL_STATUS_OK, crypto::default_crypto().aes_ctr_drbg_generate(ctx, std::span<uint8_t, crypto::aes_ctr_drbg_output_size>(out_b, crypto::aes_ctr_drbg_output_size)));

    EXPECT_NE(0, memcmp(out_a, out_b, crypto::aes_ctr_drbg_output_size));
}

TEST_F(AesCtrDrbgRealInjection, InitKeepsContextWhenNestedEcbFails)
{
    EXPECT_CALL(mock, cipher_encrypt(_, PSA_ALG_ECB_NO_PADDING, _, _, _)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::cipher_encrypt));
    EXPECT_CALL(mock, destroy_key(_)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::destroy_key));
    const std::array<uint8_t, crypto::aes_ctr_drbg_seed_size> seed {};
    crypto::CtrDrbgContext ctx {};
    ctx.key[0]          = 0xa5;
    const auto original = ctx;
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, ::testing::SizeIs(crypto::aes_key_size), _, PSA_KEY_TYPE_AES, crypto::aes_key_bits)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::import_key)).WillOnce(Return(PSA_ERROR_INSUFFICIENT_MEMORY));

    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_ctr_drbg_init(ctx, seed, seed));
    EXPECT_EQ(original.key, ctx.key);
    EXPECT_EQ(original.counter, ctx.counter);
}

class AesCtrDrbgGenerateFailure : public AesCtrDrbgRealInjection, public ::testing::WithParamInterface<int>
{};

TEST_P(AesCtrDrbgGenerateFailure, KeepsContextAndOutputOnFailure)
{
    const int successful_blocks = GetParam();
    crypto::CtrDrbgContext ctx {};
    ctx.counter[0]      = 0x42;
    const auto original = ctx;
    std::array<uint8_t, crypto::aes_ctr_drbg_output_size> generated;
    generated.fill(0xa5);
    const auto original_output = generated;
    EXPECT_CALL(mock, cipher_encrypt(_, PSA_ALG_ECB_NO_PADDING, _, _, _)).Times(successful_blocks).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::cipher_encrypt));
    EXPECT_CALL(mock, destroy_key(_)).Times(successful_blocks).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::destroy_key));
    int imports = 0;
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, ::testing::SizeIs(crypto::aes_key_size), _, PSA_KEY_TYPE_AES, crypto::aes_key_bits))
      .Times(successful_blocks + 1)
      .WillRepeatedly([&](psa_key_usage_t usage, psa_algorithm_t algorithm, std::span<const uint8_t> data, psa_key_id_t &key_id, psa_key_type_t type, size_t bits) {
          if (imports++ == successful_blocks) {
              return PSA_ERROR_INSUFFICIENT_MEMORY;
          }
          return real_psa_wrappers.import_key(usage, algorithm, data, key_id, type, bits);
      });

    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_ctr_drbg_generate(ctx, generated));
    EXPECT_EQ(original.key, ctx.key);
    EXPECT_EQ(original.counter, ctx.counter);
    EXPECT_EQ(original_output, generated);
}

// Fail in the output block or either block of the final state update.
INSTANTIATE_TEST_SUITE_P(OutputAndUpdateBlocks, AesCtrDrbgGenerateFailure, ::testing::Values(0, 1, 2));

TEST_F(AesCtrDrbgInjection, RejectsWrongOutputSizeBeforeCallingPsaWrappers)
{
    crypto::CtrDrbgContext ctx {};
    const auto original = ctx;
    std::array<uint8_t, crypto::aes_ctr_drbg_output_size + 1> generated;
    generated.fill(0xa5);
    const auto original_output = generated;

    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, algorithms.aes_ctr_drbg_generate(ctx, {}));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, algorithms.aes_ctr_drbg_generate(ctx, generated));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, algorithms.aes_ctr_drbg_generate(ctx, std::span<uint8_t>(generated).first(crypto::aes_ctr_drbg_output_size - 1)));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, algorithms.aes_ctr_drbg_generate(ctx, std::span<uint8_t>(generated).first(0)));
    EXPECT_EQ(original.key, ctx.key);
    EXPECT_EQ(original.counter, ctx.counter);
    EXPECT_EQ(original_output, generated);
}
