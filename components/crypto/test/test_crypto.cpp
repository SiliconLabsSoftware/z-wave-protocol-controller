#include <array>
#include <cstring>

#include "crypto.hpp"
#include "mock_psa_wrappers.hpp"
#include "test_fixture.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

class CryptoInjection : public Crypto
{
    protected:
        crypto::PsaWrappers real_psa_wrappers;
        StrictMock<crypto::test::MockPsaWrappers> mock;
        crypto::Crypto algorithms {mock};
        const std::array<uint8_t, CRYPTO_AES_KEY_SIZE> key {};
        const std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> input {};
        std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> output {};

        void SetUp() override
        {
            Crypto::SetUp();
            EXPECT_CALL(mock, crypto_init()).Times(::testing::AnyNumber()).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::crypto_init));
        }

        void expect_import_failure(psa_key_usage_t usage, psa_algorithm_t algorithm)
        {
            EXPECT_CALL(mock, import_key(usage, algorithm, key.data(), _, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS, CRYPTO_AES_KEY_SIZE)).WillOnce(Return(PSA_ERROR_INSUFFICIENT_MEMORY));
            EXPECT_CALL(mock, psa_status_to_sl_status(PSA_ERROR_INSUFFICIENT_MEMORY)).WillOnce(Return(SL_STATUS_ALLOCATION_FAILED));
        }
};

TEST_F(CryptoInjection, EcbDecryptUsesInjectedPsaWrappers)
{
    expect_import_failure(PSA_KEY_USAGE_DECRYPT, PSA_ALG_ECB_NO_PADDING);
    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_ecb_decrypt(key, input, output));
}

TEST_F(CryptoInjection, CbcMacReturnsInjectedInitFailure)
{
    EXPECT_CALL(mock, crypto_init()).WillOnce(Return(PSA_ERROR_BAD_STATE));
    EXPECT_CALL(mock, psa_status_to_sl_status(PSA_ERROR_BAD_STATE)).WillOnce(Return(SL_STATUS_INVALID_STATE));
    output.fill(0xa5);
    const auto original_output = output;

    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.aes_cbc_mac(key, input, input, output));
    EXPECT_EQ(original_output, output);
}

TEST_F(CryptoInjection, CbcMacUsesInjectedPsaWrappers)
{
    expect_import_failure(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_CBC_NO_PADDING);
    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_cbc_mac(key, input, input, output));
}

TEST_F(CryptoInjection, CmacUsesInjectedPsaWrappers)
{
    expect_import_failure(PSA_KEY_USAGE_SIGN_MESSAGE, PSA_ALG_CMAC);
    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_cmac_compute(key, input, output));
}

TEST_F(CryptoInjection, OfbUsesInjectedPsaWrappers)
{
    expect_import_failure(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_OFB);
    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_ofb_xor(key, input, output));
}

TEST_F(CryptoInjection, CcmEncryptAndDecryptUseSameInjectedPsaWrappers)
{
    const std::array<uint8_t, 13> nonce {};
    size_t length        = 0;
    const auto algorithm = PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, 8);
    expect_import_failure(PSA_KEY_USAGE_ENCRYPT, algorithm);
    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_ccm_encrypt(key, 8, nonce, {}, input, output, length));

    expect_import_failure(PSA_KEY_USAGE_DECRYPT, algorithm);
    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_ccm_decrypt(key, 8, nonce, {}, input, output, length));
}

TEST_F(CryptoInjection, RandomUsesInjectedStatusConversion)
{
    EXPECT_CALL(mock, generate_random(_, _)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::generate_random));
    EXPECT_CALL(mock, psa_status_to_sl_status(PSA_SUCCESS)).WillOnce(Return(SL_STATUS_FAIL));
    EXPECT_EQ(SL_STATUS_FAIL, algorithms.generate_random(output));
}

TEST_F(CryptoInjection, X25519PublicKeyUsesInjectedStatusConversion)
{
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_DERIVE, PSA_ALG_ECDH, _, _, CRYPTO_X25519_KEY_TYPE, CRYPTO_X25519_KEY_BITS, CRYPTO_X25519_PRIVATE_KEY_SIZE)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::import_key));
    EXPECT_CALL(mock, export_public_key(_, _, _, _)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::export_public_key));
    EXPECT_CALL(mock, destroy_key(_)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::destroy_key));
    const std::array<uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE> private_key {};
    std::array<uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE> public_key {};
    EXPECT_CALL(mock, psa_status_to_sl_status(PSA_SUCCESS)).WillOnce(Return(SL_STATUS_FAIL));
    EXPECT_EQ(SL_STATUS_FAIL, algorithms.x25519_derive_public_key(private_key, public_key));
}

TEST_F(CryptoInjection, X25519SharedSecretUsesInjectedStatusConversion)
{
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_DERIVE, PSA_ALG_ECDH, _, _, CRYPTO_X25519_KEY_TYPE, CRYPTO_X25519_KEY_BITS, CRYPTO_X25519_PRIVATE_KEY_SIZE)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::import_key));
    EXPECT_CALL(mock, raw_key_agreement(PSA_ALG_ECDH, _, _, _, _, _, _)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::raw_key_agreement));
    EXPECT_CALL(mock, destroy_key(_)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::destroy_key));
    const std::array<uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE> private_key {};
    std::array<uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE> public_key {};
    std::array<uint8_t, CRYPTO_X25519_SHARED_SECRET_SIZE> shared_secret {};
    crypto::Crypto production(real_psa_wrappers);
    ASSERT_EQ(SL_STATUS_OK, production.x25519_derive_public_key(private_key, public_key));

    EXPECT_CALL(mock, psa_status_to_sl_status(PSA_SUCCESS)).WillOnce(Return(SL_STATUS_FAIL));
    EXPECT_EQ(SL_STATUS_FAIL, algorithms.x25519_derive_shared_secret(private_key, public_key, shared_secret));
}

TEST_F(CryptoInjection, InstancesKeepIndependentDependencies)
{
    EXPECT_CALL(mock, generate_random(_, _)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::generate_random));
    StrictMock<crypto::test::MockPsaWrappers> other_mock;
    EXPECT_CALL(other_mock, generate_random(_, _)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::generate_random));
    EXPECT_CALL(other_mock, crypto_init()).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::crypto_init));
    crypto::Crypto other_algorithms(other_mock);
    EXPECT_CALL(mock, psa_status_to_sl_status(PSA_SUCCESS)).WillOnce(Return(SL_STATUS_FAIL));
    EXPECT_CALL(other_mock, psa_status_to_sl_status(PSA_SUCCESS)).WillOnce(Return(SL_STATUS_NOT_SUPPORTED));

    EXPECT_EQ(SL_STATUS_FAIL, algorithms.generate_random(output));
    EXPECT_EQ(SL_STATUS_NOT_SUPPORTED, other_algorithms.generate_random(output));
    // Legacy callers keep using the production implementation.
    EXPECT_EQ(SL_STATUS_OK, crypto_generate_random(output.data(), output.size()));
}

TEST_F(CryptoInjection, SameInstanceSupportsMultipleAlgorithms)
{
    EXPECT_CALL(mock, cipher_encrypt(_, PSA_ALG_ECB_NO_PADDING, _, _, _, _, _)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::cipher_encrypt));
    EXPECT_CALL(mock, cipher_decrypt(_, PSA_ALG_ECB_NO_PADDING, _, _, _, _, _)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::cipher_decrypt));
    EXPECT_CALL(mock, mac_compute(_, PSA_ALG_CMAC, _, _, _, _, _)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::mac_compute));
    EXPECT_CALL(mock, destroy_key(_)).Times(3).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::destroy_key));
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key.data(), _, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS, CRYPTO_AES_KEY_SIZE)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::import_key));
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_DECRYPT, PSA_ALG_ECB_NO_PADDING, key.data(), _, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS, CRYPTO_AES_KEY_SIZE)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::import_key));
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_SIGN_MESSAGE, PSA_ALG_CMAC, key.data(), _, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS, CRYPTO_AES_KEY_SIZE)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::import_key));
    EXPECT_CALL(mock, psa_status_to_sl_status(PSA_SUCCESS)).Times(3).WillRepeatedly(Return(SL_STATUS_OK));

    ASSERT_EQ(SL_STATUS_OK, algorithms.aes_ecb_encrypt(key, input, output));
    std::array<uint8_t, CRYPTO_AES_BLOCK_SIZE> plaintext {};
    ASSERT_EQ(SL_STATUS_OK, algorithms.aes_ecb_decrypt(key, output, plaintext));
    EXPECT_EQ(input, plaintext);
    EXPECT_EQ(SL_STATUS_OK, algorithms.aes_cmac_compute(key, input, output));
}

TEST_F(CryptoInjection, DrbgInitKeepsContextWhenNestedEcbFails)
{
    EXPECT_CALL(mock, cipher_encrypt(_, PSA_ALG_ECB_NO_PADDING, _, _, _, _, _)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::cipher_encrypt));
    EXPECT_CALL(mock, destroy_key(_)).Times(1).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::destroy_key));
    const std::array<uint8_t, CRYPTO_AES_CTR_DRBG_SEED_SIZE> seed {};
    crypto_aes_ctr_drbg_ctx_t ctx {};
    ctx.key[0]          = 0xa5;
    const auto original = ctx;
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, _, _, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS, CRYPTO_AES_KEY_SIZE)).WillOnce(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::import_key)).WillOnce(Return(PSA_ERROR_INSUFFICIENT_MEMORY));
    EXPECT_CALL(mock, psa_status_to_sl_status(PSA_SUCCESS)).WillOnce(Return(SL_STATUS_OK));
    EXPECT_CALL(mock, psa_status_to_sl_status(PSA_ERROR_INSUFFICIENT_MEMORY)).WillOnce(Return(SL_STATUS_ALLOCATION_FAILED));

    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_ctr_drbg_init(ctx, seed, seed));
    EXPECT_EQ(0, std::memcmp(&original, &ctx, sizeof(ctx)));
}

TEST_F(CryptoInjection, DrbgGenerateKeepsContextAndOutputWhenUpdateFails)
{
    crypto_aes_ctr_drbg_ctx_t ctx {};
    ctx.counter[0]      = 0x42;
    const auto original = ctx;
    std::array<uint8_t, CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE> generated;
    generated.fill(0xa5);
    const auto original_output     = generated;
    constexpr int generated_blocks = CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE / CRYPTO_AES_BLOCK_SIZE;
    EXPECT_CALL(mock, cipher_encrypt(_, PSA_ALG_ECB_NO_PADDING, _, _, _, _, _)).Times(generated_blocks).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::cipher_encrypt));
    EXPECT_CALL(mock, destroy_key(_)).Times(generated_blocks).WillRepeatedly(Invoke(&real_psa_wrappers, &crypto::PsaWrappers::destroy_key));
    int imports = 0;
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, _, _, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS, CRYPTO_AES_KEY_SIZE))
      .Times(generated_blocks + 1)
      .WillRepeatedly([&](psa_key_usage_t usage, psa_algorithm_t algorithm, const uint8_t *data, psa_key_id_t *key_id, psa_key_type_t type, size_t bits, size_t key_size) {
          if (imports++ == generated_blocks) {
              return PSA_ERROR_INSUFFICIENT_MEMORY;
          }
          return real_psa_wrappers.import_key(usage, algorithm, data, key_id, type, bits, key_size);
      });
    EXPECT_CALL(mock, psa_status_to_sl_status(PSA_SUCCESS)).Times(generated_blocks).WillRepeatedly(Return(SL_STATUS_OK));
    EXPECT_CALL(mock, psa_status_to_sl_status(PSA_ERROR_INSUFFICIENT_MEMORY)).WillOnce(Return(SL_STATUS_ALLOCATION_FAILED));

    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.aes_ctr_drbg_generate(ctx, generated));
    EXPECT_EQ(0, std::memcmp(&original, &ctx, sizeof(ctx)));
    EXPECT_EQ(original_output, generated);
}

TEST_F(CryptoInjection, DrbgRejectsWrongOutputSizeBeforeCallingPsaWrappers)
{
    crypto_aes_ctr_drbg_ctx_t ctx {};
    const auto original = ctx;
    std::array<uint8_t, CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE + 1> generated {};

    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, algorithms.aes_ctr_drbg_generate(ctx, {}));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, algorithms.aes_ctr_drbg_generate(ctx, generated));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, algorithms.aes_ctr_drbg_generate(ctx, std::span<uint8_t>(generated).first(CRYPTO_AES_CTR_DRBG_OUTPUT_SIZE - 1)));
    EXPECT_EQ(0, std::memcmp(&original, &ctx, sizeof(ctx)));
}

TEST_F(CryptoInjection, RandomReturnsInjectedGenerationFailure)
{
    EXPECT_CALL(mock, generate_random(output.data(), output.size())).WillOnce(Return(PSA_ERROR_INSUFFICIENT_ENTROPY));
    EXPECT_CALL(mock, psa_status_to_sl_status(PSA_ERROR_INSUFFICIENT_ENTROPY)).WillOnce(Return(SL_STATUS_FAIL));
    output.fill(0xa5);
    const auto original_output = output;

    EXPECT_EQ(SL_STATUS_FAIL, algorithms.generate_random(output));
    EXPECT_EQ(original_output, output);
}

TEST_F(CryptoInjection, X25519ReturnsInjectedImportFailure)
{
    const std::array<uint8_t, CRYPTO_X25519_PRIVATE_KEY_SIZE> private_key {};
    std::array<uint8_t, CRYPTO_X25519_PUBLIC_KEY_SIZE> public_key {};
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_DERIVE, PSA_ALG_ECDH, private_key.data(), _, CRYPTO_X25519_KEY_TYPE, CRYPTO_X25519_KEY_BITS, private_key.size())).WillOnce(Return(PSA_ERROR_INSUFFICIENT_MEMORY));
    EXPECT_CALL(mock, psa_status_to_sl_status(PSA_ERROR_INSUFFICIENT_MEMORY)).WillOnce(Return(SL_STATUS_ALLOCATION_FAILED));

    EXPECT_EQ(SL_STATUS_ALLOCATION_FAILED, algorithms.x25519_derive_public_key(private_key, public_key));
}

TEST_F(CryptoInjection, EcbDestroysImportedKeyWhenEncryptionFails)
{
    ::testing::InSequence sequence;
    EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key.data(), _, PSA_KEY_TYPE_AES, CRYPTO_AES_KEY_BITS, key.size())).WillOnce(::testing::DoAll(::testing::SetArgPointee<3>(42), Return(PSA_SUCCESS)));
    EXPECT_CALL(mock, cipher_encrypt(42, PSA_ALG_ECB_NO_PADDING, input.data(), input.size(), output.data(), output.size(), _)).WillOnce(Return(PSA_ERROR_BAD_STATE));
    EXPECT_CALL(mock, destroy_key(42)).WillOnce(Return(PSA_SUCCESS));
    EXPECT_CALL(mock, psa_status_to_sl_status(PSA_ERROR_BAD_STATE)).WillOnce(Return(SL_STATUS_INVALID_STATE));

    EXPECT_EQ(SL_STATUS_INVALID_STATE, algorithms.aes_ecb_encrypt(key, input, output));
}
