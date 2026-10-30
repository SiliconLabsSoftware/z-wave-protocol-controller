#include <array>

#include "psa_wrappers.hpp"
#include "test_fixture.hpp"

TEST_F(CryptoBackend, PsaWrappersImportAesKeyWithRequestedAttributes)
{
    const std::array<uint8_t, 32> storage {};
    const auto key = std::span(storage).subspan(8, 16);
    psa_key_id_t key_id;
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key, key_id, PSA_KEY_TYPE_AES, key.size() * 8));

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    const auto status               = psa_get_key_attributes(key_id, &attributes);
    EXPECT_EQ(PSA_SUCCESS, status);
    if (status == PSA_SUCCESS) {
        EXPECT_EQ(PSA_KEY_TYPE_AES, psa_get_key_type(&attributes));
        EXPECT_EQ(128U, psa_get_key_bits(&attributes));
        EXPECT_EQ(PSA_KEY_USAGE_ENCRYPT, psa_get_key_usage_flags(&attributes));
        EXPECT_EQ(PSA_ALG_ECB_NO_PADDING, psa_get_key_algorithm(&attributes));
    }
    psa_reset_key_attributes(&attributes);
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.destroy_key(key_id));
}

TEST_F(CryptoBackend, PsaWrappersRespectOutputSpanCapacity)
{
    const std::array<uint8_t, 16> key {};
    const std::array<uint8_t, 16> plaintext {};
    std::array<uint8_t, 32> storage;
    storage.fill(0xa5);
    const auto output    = std::span(storage).subspan(1, plaintext.size() - 1);
    size_t output_length = 0;
    psa_key_id_t key_id  = 0;

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key, key_id, PSA_KEY_TYPE_AES, key.size() * 8));
    EXPECT_EQ(PSA_ERROR_BUFFER_TOO_SMALL, psa_wrappers.cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, plaintext, output, output_length));
    EXPECT_EQ(0xa5, storage.front());
    for (size_t index = 1 + output.size(); index < storage.size(); ++index) {
        SCOPED_TRACE(index);
        EXPECT_EQ(0xa5, storage[index]);
    }
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.destroy_key(key_id));
}

TEST_F(CryptoBackend, PsaWrappersRejectMismatchedKeySize)
{
    const std::array<uint8_t, 16> key {};
    psa_key_id_t key_id = 0;

    EXPECT_EQ(PSA_ERROR_INVALID_ARGUMENT, psa_wrappers.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key, key_id, PSA_KEY_TYPE_AES, 256));
}

TEST_F(CryptoBackend, PsaWrappersEcbEncryptAndDecryptMatchNistVector)
{
    // NIST SP 800-38A AES-128 ECB, first block.
    const std::array<uint8_t, 16> key {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    const std::array<uint8_t, 16> plaintext {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
    const std::array<uint8_t, 16> expected {0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97};
    std::array<uint8_t, 16> ciphertext {};
    std::array<uint8_t, 16> decrypted {};
    size_t output_length = 0;
    psa_key_id_t key_id  = 0;

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.import_key(PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT, PSA_ALG_ECB_NO_PADDING, key, key_id, PSA_KEY_TYPE_AES, key.size() * 8));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, plaintext, ciphertext, output_length));
    EXPECT_EQ(ciphertext.size(), output_length);
    EXPECT_EQ(expected, ciphertext);

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_decrypt(key_id, PSA_ALG_ECB_NO_PADDING, ciphertext, decrypted, output_length));
    EXPECT_EQ(decrypted.size(), output_length);
    EXPECT_EQ(plaintext, decrypted);
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.destroy_key(key_id));
}

TEST_F(CryptoBackend, PsaWrappersAbortAllowsCipherOperationReuse)
{
    const std::array<uint8_t, 16> key {};
    const std::array<uint8_t, 16> iv {};
    psa_key_id_t key_id              = 0;
    psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_CBC_NO_PADDING, key, key_id, PSA_KEY_TYPE_AES, key.size() * 8));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_encrypt_setup(operation, key_id, PSA_ALG_CBC_NO_PADDING));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_set_iv(operation, iv));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_abort(operation));

    // Aborting releases the active operation and is safe to repeat.
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.cipher_abort(operation));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_encrypt_setup(operation, key_id, PSA_ALG_CBC_NO_PADDING));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_set_iv(operation, iv));
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.cipher_abort(operation));
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.destroy_key(key_id));
}
