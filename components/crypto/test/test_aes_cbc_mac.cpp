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

#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "crypto.hpp"
#include "test_fixture.hpp"
#include "test_crypto_injection_fixture.hpp"

/* NIST SP 800-38A AES-128 key and IV (same material as OFB/CMAC tests). */

static const uint8_t nist_key_128[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
static const uint8_t nist_iv[]      = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

static const uint8_t nist_plain_16[] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};

/*
 * Z-Wave S0 auth-data layout (IV passed separately to crypto_aes_cbc_mac):
 * command, sender node ID, receiver node ID, payload length, encrypted payload.
 */
static const uint8_t s0_auth_iv[]   = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
static const uint8_t s0_auth_data[] = {
  0x81, /* SECURITY_MESSAGE_ENCAPSULATION */
  0x03, /* sender */
  0x04, /* receiver */
  0x02, /* encrypted payload length */
  0x11,
  0x22, /* encrypted payload (example ciphertext) */
};

namespace
{
    void ExpectBytesEq(const uint8_t *expected, const uint8_t *actual, size_t len)
    {
        EXPECT_THAT(std::vector<uint8_t>(actual, actual + len), ::testing::ElementsAreArray(expected, len));
    }

    void AssertS0CbcMac(const uint8_t *key, const uint8_t *iv, const uint8_t *message, uint8_t message_len, const uint8_t *expected_mac)
    {
        uint8_t calculated_mac[16];

        ASSERT_EQ(SL_STATUS_OK,
                  crypto::default_crypto().aes_cbc_mac(std::span<const uint8_t, crypto::aes_key_size>(key, crypto::aes_key_size),
                                                       std::span<const uint8_t, crypto::aes_block_size>(iv, crypto::aes_block_size),
                                                       std::span<const uint8_t>(message, message_len),
                                                       std::span<uint8_t, crypto::aes_block_size>(calculated_mac, crypto::aes_block_size)));
        ExpectBytesEq(expected_mac, calculated_mac, 16);
    }

    void ComputeS0CbcMacReference(const uint8_t *key, const uint8_t *iv, const uint8_t *data, size_t data_len, uint8_t *mac_out)
    {
        size_t message_len   = crypto::aes_block_size + data_len;
        const size_t pad_len = (crypto::aes_block_size - (message_len % crypto::aes_block_size)) % crypto::aes_block_size;
        message_len += pad_len;
        std::vector<uint8_t> message(message_len);
        uint8_t chain[crypto::aes_block_size] {};

        memcpy(message.data(), iv, crypto::aes_block_size);
        if (data_len > 0) {
            memcpy(message.data() + crypto::aes_block_size, data, data_len);
        }

        for (size_t off = 0; off < message_len; off += crypto::aes_block_size) {
            uint8_t input[crypto::aes_block_size];
            for (size_t i = 0; i < crypto::aes_block_size; i++) {
                input[i] = static_cast<uint8_t>(message[off + i] ^ chain[i]);
            }
            ASSERT_EQ(SL_STATUS_OK,
                      crypto::default_crypto().aes_ecb_encrypt(std::span<const uint8_t, crypto::aes_key_size>(key, crypto::aes_key_size), std::span<const uint8_t, crypto::aes_block_size>(input, crypto::aes_block_size), std::span<uint8_t, crypto::aes_block_size>(chain, crypto::aes_block_size)));
        }
        memcpy(mac_out, chain, crypto::aes_block_size);
    }
}  // namespace

TEST_F(AesCbcMac, S0EmptyMessageEqualsAesEcbOfIv)
{
    const uint8_t expected_mac[] = {0x50, 0xfe, 0x67, 0xcc, 0x99, 0x6d, 0x32, 0xb6, 0xda, 0x09, 0x37, 0xe9, 0x9b, 0xaf, 0xec, 0x60};
    uint8_t ecb_out[16];

    AssertS0CbcMac(nist_key_128, nist_iv, nullptr, 0, expected_mac);
    ASSERT_EQ(SL_STATUS_OK,
              crypto::default_crypto().aes_ecb_encrypt(std::span<const uint8_t, crypto::aes_key_size>(nist_key_128, crypto::aes_key_size), std::span<const uint8_t, crypto::aes_block_size>(nist_iv, crypto::aes_block_size), std::span<uint8_t, crypto::aes_block_size>(ecb_out, crypto::aes_block_size)));
    ExpectBytesEq(expected_mac, ecb_out, 16);
}

TEST_F(AesCbcMac, S0NistPlaintextOneBlock)
{
    const uint8_t expected_mac[] = {0x66, 0x8b, 0xcf, 0x60, 0xbe, 0xb0, 0x05, 0xa3, 0x53, 0x54, 0xa2, 0x01, 0xda, 0xb3, 0x6b, 0xda};

    AssertS0CbcMac(nist_key_128, nist_iv, nist_plain_16, sizeof(nist_plain_16), expected_mac);
}

TEST_F(AesCbcMac, S0NistPlaintextSevenBytes)
{
    const uint8_t expected_mac[] = {0x13, 0x73, 0x94, 0x01, 0x8c, 0xdf, 0xe1, 0x7f, 0x59, 0xf9, 0x77, 0x97, 0x9e, 0xff, 0x71, 0x08};

    AssertS0CbcMac(nist_key_128, nist_iv, nist_plain_16, 7, expected_mac);
}

TEST_F(AesCbcMac, S0NistPlaintextTwoBlocks)
{
    uint8_t message[32];
    const uint8_t expected_mac[] = {0x11, 0xe0, 0xad, 0xbc, 0x92, 0x31, 0xe1, 0x3d, 0xd8, 0x43, 0x6e, 0x69, 0xd9, 0xf3, 0x14, 0xcc};

    memcpy(message, nist_plain_16, 16);
    memcpy(message + 16, nist_plain_16, 16);

    AssertS0CbcMac(nist_key_128, nist_iv, message, sizeof(message), expected_mac);
}

TEST_F(AesCbcMac, S0MessageEncapsulationAuthData)
{
    const uint8_t expected_mac[] = {0x4c, 0x5c, 0xb1, 0x62, 0xa5, 0x17, 0x4c, 0x77, 0xf4, 0x11, 0x84, 0xa2, 0x5a, 0xe7, 0xb6, 0x0c};

    AssertS0CbcMac(nist_key_128, s0_auth_iv, s0_auth_data, sizeof(s0_auth_data), expected_mac);
}

TEST_F(AesCbcMac, CppApi)
{
    std::array<uint8_t, crypto::aes_key_size> key {};
    memcpy(key.data(), nist_key_128, sizeof(key));
    std::array<uint8_t, crypto::aes_block_size> iv {};
    memcpy(iv.data(), s0_auth_iv, sizeof(iv));
    const uint8_t expected_mac[] = {0x4c, 0x5c, 0xb1, 0x62, 0xa5, 0x17, 0x4c, 0x77, 0xf4, 0x11, 0x84, 0xa2, 0x5a, 0xe7, 0xb6, 0x0c};
    std::array<uint8_t, crypto::aes_block_size> mac {};

    ASSERT_EQ(SL_STATUS_OK, crypto::default_crypto().aes_cbc_mac(key, iv, std::span<const uint8_t>(s0_auth_data, sizeof(s0_auth_data)), mac));
    ExpectBytesEq(expected_mac, mac.data(), mac.size());
}

TEST_F(AesCbcMac, CppApiMatchesReferenceAcrossBlockAndChunkBoundaries)
{
    const std::array<size_t, 20> lengths {0, 1, 15, 16, 17, 31, 32, 33, 254, 255, 256, 257, 271, 272, 273, 511, 512, 513, 4096, 4097};
    for (const auto length: lengths) {
        SCOPED_TRACE(length);
        std::vector<uint8_t> data(length);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = static_cast<uint8_t>(13 * i + length);
        }
        std::array<uint8_t, crypto::aes_block_size> expected;
        ComputeS0CbcMacReference(nist_key_128, nist_iv, data.data(), data.size(), expected.data());
        std::array<uint8_t, crypto::aes_block_size + 2> storage;
        storage.fill(0xa5);
        auto mac = std::span(storage).subspan<1, crypto::aes_block_size>();

        ASSERT_EQ(SL_STATUS_OK, crypto::default_crypto().aes_cbc_mac(nist_key_128, nist_iv, data, mac));
        ExpectBytesEq(expected.data(), mac.data(), mac.size());
        EXPECT_EQ(0xa5, storage.front());
        EXPECT_EQ(0xa5, storage.back());
    }
}

TEST_F(AesCbcMac, S0MaxPayloadLength255)
{
    uint8_t data[UINT8_MAX];
    uint8_t expected_mac[crypto::aes_block_size];
    uint8_t calculated_mac[crypto::aes_block_size];

    for (size_t i = 0; i < UINT8_MAX; i++) {
        data[i] = static_cast<uint8_t>(i);
    }
    ComputeS0CbcMacReference(nist_key_128, nist_iv, data, UINT8_MAX, expected_mac);

    ASSERT_EQ(SL_STATUS_OK,
              crypto::default_crypto().aes_cbc_mac(std::span<const uint8_t, crypto::aes_key_size>(nist_key_128, crypto::aes_key_size),
                                                   std::span<const uint8_t, crypto::aes_block_size>(nist_iv, crypto::aes_block_size),
                                                   std::span<const uint8_t>(data, UINT8_MAX),
                                                   std::span<uint8_t, crypto::aes_block_size>(calculated_mac, crypto::aes_block_size)));
    ExpectBytesEq(expected_mac, calculated_mac, crypto::aes_block_size);
}

TEST_F(AesCbcMac, CppApiMaxPayloadLength255)
{
    std::array<uint8_t, crypto::aes_key_size> key {};
    memcpy(key.data(), nist_key_128, sizeof(key));
    std::array<uint8_t, crypto::aes_block_size> iv {};
    memcpy(iv.data(), nist_iv, sizeof(iv));
    std::array<uint8_t, UINT8_MAX> data {};
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = static_cast<uint8_t>(i);
    }
    uint8_t expected_mac[crypto::aes_block_size];
    std::array<uint8_t, crypto::aes_block_size> mac_cpp {};
    uint8_t calculated_mac[crypto::aes_block_size];

    ComputeS0CbcMacReference(nist_key_128, nist_iv, data.data(), UINT8_MAX, expected_mac);
    ASSERT_EQ(SL_STATUS_OK,
              crypto::default_crypto().aes_cbc_mac(std::span<const uint8_t, crypto::aes_key_size>(nist_key_128, crypto::aes_key_size),
                                                   std::span<const uint8_t, crypto::aes_block_size>(nist_iv, crypto::aes_block_size),
                                                   std::span<const uint8_t>(data.data(), UINT8_MAX),
                                                   std::span<uint8_t, crypto::aes_block_size>(calculated_mac, crypto::aes_block_size)));
    ASSERT_EQ(SL_STATUS_OK, crypto::default_crypto().aes_cbc_mac(key, iv, std::span<const uint8_t>(data.data(), data.size()), mac_cpp));

    ExpectBytesEq(expected_mac, calculated_mac, crypto::aes_block_size);
    ExpectBytesEq(expected_mac, mac_cpp.data(), mac_cpp.size());
    ExpectBytesEq(calculated_mac, mac_cpp.data(), mac_cpp.size());
}

TEST_F(AesCbcMacInjection, ReturnsInjectedInitFailure)
{
    EXPECT_CALL(mock, crypto_init()).WillOnce(Return(PSA_ERROR_BAD_STATE));
    output.fill(0xa5);
    const auto original_output = output;

    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.aes_cbc_mac(key, input, input, output));
    EXPECT_EQ(original_output, output);
}

TEST_F(AesCbcMacInjection, UsesInjectedPsaWrappers)
{
    expect_import_failure(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_CBC_NO_PADDING);
    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_cbc_mac(key, input, input, output));
}

TEST_F(AesCbcMacInjection, RejectsIncompleteOutputAndReleasesKey)
{
    output.fill(0xa5);
    const auto original_output = output;
    ::testing::InSequence sequence;
    EXPECT_CALL(mock, crypto_init()).WillOnce(Return(PSA_SUCCESS));
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_CBC_NO_PADDING, SameBuffer(std::span(key)), _, PSA_KEY_TYPE_AES, crypto::aes_key_bits)).WillOnce(::testing::DoAll(::testing::SetArgReferee<3>(42), Return(PSA_SUCCESS)));
    EXPECT_CALL(mock, cipher_encrypt_setup(_, 42, PSA_ALG_CBC_NO_PADDING)).WillOnce(Return(PSA_SUCCESS));
    EXPECT_CALL(mock, cipher_set_iv(_, ::testing::SizeIs(crypto::aes_block_size))).WillOnce(Return(PSA_SUCCESS));
    EXPECT_CALL(mock, cipher_update(_, _, _, _)).WillOnce(::testing::DoAll(::testing::SetArgReferee<3>(0), Return(PSA_SUCCESS)));
    EXPECT_CALL(mock, cipher_finish(_, _, _)).WillOnce(::testing::DoAll(::testing::SetArgReferee<2>(0), Return(PSA_SUCCESS)));
    EXPECT_CALL(mock, destroy_key(42)).WillOnce(Return(PSA_SUCCESS));

    EXPECT_EQ(SL_STATUS_FAIL, algorithms.aes_cbc_mac(key, input, {}, output));
    EXPECT_EQ(original_output, output);
}

TEST_F(AesCbcMacInjection, RetainsLastBlockWhenPsaBuffersOutput)
{
    ::testing::InSequence sequence;
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_CBC_NO_PADDING, _, _, PSA_KEY_TYPE_AES, crypto::aes_key_bits)).WillOnce(::testing::DoAll(::testing::SetArgReferee<3>(42), Return(PSA_SUCCESS)));
    EXPECT_CALL(mock, cipher_encrypt_setup(_, 42, PSA_ALG_CBC_NO_PADDING)).WillOnce(Return(PSA_SUCCESS));
    EXPECT_CALL(mock, cipher_set_iv(_, ::testing::Each(0))).WillOnce(Return(PSA_SUCCESS));
    EXPECT_CALL(mock, cipher_update(_, SameBuffer(std::span(input)), _, _)).WillOnce(::testing::DoAll(::testing::SetArgReferee<3>(0), Return(PSA_SUCCESS)));
    EXPECT_CALL(mock, cipher_update(_, SameBuffer(std::span(input)), _, _)).WillOnce([](auto &, auto, auto ciphertext, size_t &length) {
        std::fill_n(ciphertext.begin(), crypto::aes_block_size, 0x37);
        std::fill_n(ciphertext.begin() + crypto::aes_block_size, crypto::aes_block_size, 0x91);
        length = 2 * crypto::aes_block_size;
        return PSA_SUCCESS;
    });
    EXPECT_CALL(mock, cipher_finish(_, _, _)).WillOnce(::testing::DoAll(::testing::SetArgReferee<2>(0), Return(PSA_SUCCESS)));
    EXPECT_CALL(mock, destroy_key(42)).WillOnce(Return(PSA_SUCCESS));

    EXPECT_EQ(SL_STATUS_OK, algorithms.aes_cbc_mac(key, input, input, output));
    EXPECT_THAT(output, ::testing::Each(0x91));
}

class AesCbcMacInvalidOutputLength : public AesCbcMacInjection, public ::testing::WithParamInterface<size_t>
{};

TEST_P(AesCbcMacInvalidOutputLength, RejectsInvalidUpdateLengthWithoutChangingMac)
{
    output.fill(0xa5);
    const auto original = output;
    ::testing::InSequence sequence;
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_CBC_NO_PADDING, _, _, PSA_KEY_TYPE_AES, crypto::aes_key_bits)).WillOnce(::testing::DoAll(::testing::SetArgReferee<3>(42), Return(PSA_SUCCESS)));
    EXPECT_CALL(mock, cipher_encrypt_setup(_, 42, PSA_ALG_CBC_NO_PADDING)).WillOnce(Return(PSA_SUCCESS));
    EXPECT_CALL(mock, cipher_set_iv(_, _)).WillOnce(Return(PSA_SUCCESS));
    EXPECT_CALL(mock, cipher_update(_, _, _, _)).WillOnce(::testing::DoAll(::testing::SetArgReferee<3>(GetParam()), Return(PSA_SUCCESS)));
    EXPECT_CALL(mock, cipher_abort(_)).WillOnce(Return(PSA_SUCCESS));
    EXPECT_CALL(mock, destroy_key(42)).WillOnce(Return(PSA_SUCCESS));

    EXPECT_EQ(SL_STATUS_FAIL, algorithms.aes_cbc_mac(key, input, {}, output));
    EXPECT_EQ(original, output);
}

INSTANTIATE_TEST_SUITE_P(AesCbcMac, AesCbcMacInvalidOutputLength, ::testing::Values(1, crypto::aes_block_size + 1, 2 * crypto::aes_block_size, 257));

class AesCbcMacStreamingFailure : public CryptoRealInjection, public ::testing::WithParamInterface<int>
{};

TEST_P(AesCbcMacStreamingFailure, PreservesMacAndReleasesResourcesAfterPartialProgress)
{
    const std::array<uint8_t, 513> data {};
    const int successful_updates = GetParam();
    output.fill(0xa5);
    const auto original = output;
    ::testing::InSequence sequence;
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_CBC_NO_PADDING, _, _, PSA_KEY_TYPE_AES, crypto::aes_key_bits)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::import_key));
    EXPECT_CALL(mock, cipher_encrypt_setup(_, _, PSA_ALG_CBC_NO_PADDING)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::cipher_encrypt_setup));
    EXPECT_CALL(mock, cipher_set_iv(_, _)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::cipher_set_iv));
    EXPECT_CALL(mock, cipher_update(_, _, _, _)).Times(successful_updates).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::cipher_update));
    if (successful_updates < 4) {
        EXPECT_CALL(mock, cipher_update(_, _, _, _)).WillOnce(Return(PSA_ERROR_BAD_STATE));
    } else {
        EXPECT_CALL(mock, cipher_finish(_, _, _)).WillOnce(Return(PSA_ERROR_BAD_STATE));
    }
    EXPECT_CALL(mock, cipher_abort(_)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::cipher_abort));
    EXPECT_CALL(mock, destroy_key(_)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::destroy_key));

    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.aes_cbc_mac(key, input, data, output));
    EXPECT_EQ(original, output);
}

INSTANTIATE_TEST_SUITE_P(AesCbcMac, AesCbcMacStreamingFailure, ::testing::Values(1, 2, 3, 4));

TEST_P(AesCbcMacMultipartFailure, ReleasesResourcesAndReturnsFailure)
{
    expect_multipart_failure(PSA_ALG_CBC_NO_PADDING, crypto::aes_block_size);
    output.fill(0xa5);
    const auto original_output = output;

    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.aes_cbc_mac(key, input, {}, output));
    EXPECT_EQ(original_output, output);
}

INSTANTIATE_TEST_SUITE_P(AesCbcMac, AesCbcMacMultipartFailure, ::testing::Values(CipherFailureStage::Setup, CipherFailureStage::SetIv, CipherFailureStage::Update, CipherFailureStage::Finish), [](const ::testing::TestParamInfo<CipherFailureStage> &info) {
    return CipherFailureStageName(info.param);
});
