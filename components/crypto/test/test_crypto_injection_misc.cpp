#include <algorithm>
#include <array>

#include "crypto.h"
#include "c_api_test_support.hpp"
#include "crypto_mock.hpp"
#include "test_crypto_injection_fixture.hpp"

// Cross-algorithm PSA wrapper injection and legacy vs injectable Crypto behavior.

TEST_F(CryptoInjectionMisc, SameInstanceSupportsMultipleAlgorithms)
{
    EXPECT_CALL(mock, cipher_encrypt(_, PSA_ALG_ECB_NO_PADDING, _, _, _)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::cipher_encrypt));
    EXPECT_CALL(mock, cipher_decrypt(_, PSA_ALG_ECB_NO_PADDING, _, _, _)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::cipher_decrypt));
    EXPECT_CALL(mock, mac_compute(_, PSA_ALG_CMAC, _, _, _)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::mac_compute));
    EXPECT_CALL(mock, destroy_key(_)).Times(3).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::destroy_key));
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, SameBuffer(std::span(key)), _, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::import_key));
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_DECRYPT, PSA_ALG_ECB_NO_PADDING, SameBuffer(std::span(key)), _, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::import_key));
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_SIGN_MESSAGE, PSA_ALG_CMAC, SameBuffer(std::span(key)), _, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::import_key));

    ASSERT_EQ(SL_STATUS_OK, algorithms.aes_ecb_encrypt(key, input, output));
    std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> plaintext {};
    ASSERT_EQ(SL_STATUS_OK, algorithms.aes_ecb_decrypt(key, output, plaintext));
    EXPECT_EQ(input, plaintext);
    EXPECT_EQ(SL_STATUS_OK, algorithms.aes_cmac_compute(key, input, output));
}

TEST_F(CryptoInjectionMisc, InstancesKeepIndependentDependencies)
{
    EXPECT_CALL(mock, generate_random(SameBuffer(std::span(output)))).WillOnce(Return(PSA_ERROR_BAD_STATE));
    StrictMock<crypto::test::PsaWrappersMock> other_mock;
    EXPECT_CALL(other_mock, generate_random(SameBuffer(std::span(output)))).WillOnce(Return(PSA_ERROR_NOT_SUPPORTED));
    EXPECT_CALL(other_mock, crypto_init()).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::crypto_init));
    crypto::Crypto other_algorithms(other_mock);

    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.generate_random(output));
    EXPECT_EQ(SL_STATUS_NOT_SUPPORTED, other_algorithms.generate_random(output));
    EXPECT_EQ(SL_STATUS_OK, crypto_generate_random(output.data(), output.size()));
}

TEST_F(CryptoInjectionMisc, InjectableInstancePropagatesImportFailure)
{
    StrictMock<crypto::test::PsaWrappersMock> mock;
    crypto::Crypto algorithms(mock);
    const std::array<uint8_t, CRYPTO_AES_KEY_SIZE> key {};
    const std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> input {};
    std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> output;
    output.fill(0xa5);
    const auto original_output = output;

    EXPECT_CALL(mock, crypto_init()).Times(2).WillRepeatedly(Return(PSA_SUCCESS));
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, SameBuffer(std::span(key)), _, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS)).Times(2).WillRepeatedly(Return(PSA_ERROR_INSUFFICIENT_MEMORY));

    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_ecb_encrypt(key, input, output));
    EXPECT_EQ(original_output, output);

    crypto::set_c_api_backend(algorithms);
    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, crypto_aes_ecb_encrypt(key.data(), input.data(), output.data()));
    crypto::reset_c_api_backend();
}

TEST_F(CryptoInjectionMisc, CApiUsesDefaultBackendUntilInjected)
{
    EXPECT_CALL(mock, generate_random(SameBuffer(std::span(output)))).WillOnce(Return(PSA_ERROR_BAD_STATE));
    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.generate_random(output));
    EXPECT_EQ(SL_STATUS_OK, crypto_generate_random(output.data(), output.size()));
}

TEST_F(CryptoInjectionMisc, InitFailuresLeaveOutputsUntouched)
{
    EXPECT_CALL(mock, crypto_init()).Times(10).WillRepeatedly(Return(PSA_ERROR_BAD_STATE));
    output.fill(0xa5);
    const auto original_output = output;
    const std::array<uint8_t, 13> nonce {};
    const std::array<uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE> private_key {};
    std::array<uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE> public_key;
    public_key.fill(0xa5);
    const auto original_public_key = public_key;
    std::array<uint8_t, CRYPTO_X25519_SHARED_SECRET_SIZE> shared_secret;
    shared_secret.fill(0xa5);
    const auto original_shared_secret = shared_secret;
    size_t length                     = 123;

    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.aes_ecb_encrypt(key, input, output));
    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.aes_ecb_decrypt(key, input, output));
    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.aes_cmac_compute(key, input, output));
    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.aes_ofb_xor(key, input, output));
    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.aes_cbc_mac(key, input, input, output));
    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.aes_ccm_encrypt(key, 8, nonce, {}, input, output, length));
    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.aes_ccm_decrypt(key, 8, nonce, {}, input, output, length));
    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.generate_random(output));
    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.x25519_derive_public_key(private_key, public_key));
    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.x25519_derive_shared_secret(private_key, public_key, shared_secret));
    EXPECT_EQ(original_output, output);
    EXPECT_EQ(original_public_key, public_key);
    EXPECT_EQ(original_shared_secret, shared_secret);
    EXPECT_EQ(123u, length);
}

TEST(CryptoInterface, ApplicationCanUseMockWithoutPsaBackend)
{
    StrictMock<crypto::test::CryptoMock> mock;
    const crypto::CryptoInterface &algorithms = mock;
    std::array<uint8_t, 8> output {};
    EXPECT_CALL(mock, generate_random(SameBuffer(std::span(output)))).WillOnce([](std::span<uint8_t> bytes) {
        std::fill(bytes.begin(), bytes.end(), 0xa5);
        return SL_STATUS_OK;
    });

    EXPECT_EQ(SL_STATUS_OK, algorithms.generate_random(output));
    EXPECT_THAT(output, ::testing::Each(0xa5));
}
