#include <array>

#include "aes_ecb.hpp"
#include "mock_helpers.hpp"
#include "test_fixture.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

TEST_F(Crypto, HelpersTranslatePsaStatuses)
{
    crypto::Helpers helpers;
    EXPECT_EQ(SL_STATUS_OK, helpers.psa_status_to_sl_status(PSA_SUCCESS));
    EXPECT_EQ(SL_STATUS_INVALID_SIGNATURE, helpers.psa_status_to_sl_status(PSA_ERROR_INVALID_SIGNATURE));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, helpers.psa_status_to_sl_status(PSA_ERROR_INVALID_ARGUMENT));
    EXPECT_EQ(SL_STATUS_NOT_SUPPORTED, helpers.psa_status_to_sl_status(PSA_ERROR_NOT_SUPPORTED));
    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, helpers.psa_status_to_sl_status(PSA_ERROR_INSUFFICIENT_MEMORY));
    EXPECT_EQ(SL_STATUS_INVALID_STATE, helpers.psa_status_to_sl_status(PSA_ERROR_BAD_STATE));
    EXPECT_EQ(SL_STATUS_FAIL, helpers.psa_status_to_sl_status(PSA_ERROR_GENERIC_ERROR));
}

TEST_F(Crypto, HelpersImportAesKeyWithRequestedAttributes)
{
    crypto::Helpers helpers;
    const std::array<uint8_t, CRYPTO_AES_KEY_SIZE> key {};
    psa_key_id_t key_id;
    ASSERT_EQ(PSA_SUCCESS, helpers.import_aes_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key.data(), &key_id));

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    const auto status               = psa_get_key_attributes(key_id, &attributes);
    EXPECT_EQ(PSA_SUCCESS, status);
    if (status == PSA_SUCCESS) {
        EXPECT_EQ(PSA_KEY_TYPE_AES, psa_get_key_type(&attributes));
        EXPECT_EQ(CRYPTO_AES_KEY_BITS, psa_get_key_bits(&attributes));
        EXPECT_EQ(PSA_KEY_USAGE_ENCRYPT, psa_get_key_usage_flags(&attributes));
        EXPECT_EQ(PSA_ALG_ECB_NO_PADDING, psa_get_key_algorithm(&attributes));
    }
    psa_reset_key_attributes(&attributes);
    EXPECT_EQ(PSA_SUCCESS, psa_destroy_key(key_id));
}

TEST_F(Crypto, HelpersInterceptCallsFromCppCryptoApi)
{
    StrictMock<crypto::test::MockHelpers> mock;
    crypto::Crypto algorithms(mock);
    const std::array<uint8_t, CRYPTO_AES_KEY_SIZE> key {};
    const std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> input {};
    std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> output;
    output.fill(0xa5);
    const auto original_output = output;

    ::testing::InSequence sequence;
    EXPECT_CALL(mock, import_aes_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key.data(), _)).WillOnce(Return(PSA_ERROR_INSUFFICIENT_MEMORY));
    EXPECT_CALL(mock, psa_status_to_sl_status(PSA_ERROR_INSUFFICIENT_MEMORY)).WillOnce(Return(SL_STATUS_ALLOCATION_FAILED));

    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_ecb_encrypt(key, input, output));
    EXPECT_EQ(original_output, output);

    // Supplying a mock for one call does not affect callers using the default.
    EXPECT_EQ(SL_STATUS_OK, crypto_aes_ecb_encrypt(key.data(), input.data(), output.data()));
}
