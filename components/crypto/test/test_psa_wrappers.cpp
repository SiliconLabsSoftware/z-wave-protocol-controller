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
#include <cstring>

#include "psa/psa_wrappers.hpp"
#include "psa_test_fixture.hpp"

namespace
{
    constexpr size_t kAes128KeyBytes            = 16;
    constexpr size_t kAes128KeyBits             = 128;
    constexpr size_t kAes256KeyBits             = 256;
    constexpr size_t kRandomOutputBytes         = 16;
    constexpr size_t kImportKeyStorageBytes     = 32;
    constexpr size_t kImportKeyMaterialOffset   = 8;
    constexpr size_t kCipherOutputStorageBytes  = 32;
    constexpr size_t kCipherOutputSubspanOffset = 1;
    constexpr uint8_t kUnmodifiedStorageFill    = 0xa5;

    // NIST SP 800-38A AES-128 ECB, first block.
    const std::array<uint8_t, kAes128KeyBytes> kNistAes128EcbKey {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    const std::array<uint8_t, kAes128KeyBytes> kNistAes128EcbPlaintext {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
    const std::array<uint8_t, kAes128KeyBytes> kNistAes128EcbCiphertext {0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97};

    // NIST SP 800-38A OFB-AES128, first block (Appendix F.4.1).
    const std::array<uint8_t, kAes128KeyBytes> kNistAes128OfbIv {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    const std::array<uint8_t, kAes128KeyBytes> kNistAes128OfbCiphertext {0x3b, 0x3f, 0xd9, 0x2e, 0xb7, 0x2d, 0xad, 0x20, 0x33, 0x34, 0x49, 0xf8, 0xe8, 0x3c, 0xfb, 0x4a};

    // RFC 4493 AES-CMAC, 16-byte message (example 2).
    const std::array<uint8_t, kAes128KeyBytes> kRfc4493Cmac {0x07, 0x0a, 0x16, 0xb4, 0x6b, 0x4d, 0x41, 0x44, 0xf7, 0x9b, 0xdd, 0x9d, 0xd0, 0x4a, 0x28, 0x7c};

    constexpr size_t kCcmNonceBytes         = 8;
    constexpr size_t kCcmAadBytes           = 16;
    constexpr size_t kCcmPlaintextBytes     = 16;
    constexpr size_t kCcmTagBytes           = 6;
    constexpr size_t kCcmCiphertextBytes    = 22;
    constexpr size_t kCcmCiphertextBufBytes = 32;
    constexpr psa_algorithm_t kCcmAlgorithm = PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, kCcmTagBytes);
    const std::array<uint8_t, kAes128KeyBytes> kNistCcmKey {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f};
    const std::array<uint8_t, kCcmNonceBytes> kNistCcmNonce {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
    const std::array<uint8_t, kCcmAadBytes> kNistCcmAad {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    const std::array<uint8_t, kCcmPlaintextBytes> kNistCcmPlaintext {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f};
    const std::array<uint8_t, kCcmCiphertextBytes> kNistCcmCiphertext {0xd2, 0xa1, 0xf0, 0xe0, 0x51, 0xea, 0x5f, 0x62, 0x08, 0x1a, 0x77, 0x92, 0x07, 0x3d, 0x59, 0x3d, 0x1f, 0xc6, 0x4f, 0xbf, 0xac, 0xcd};

    constexpr size_t kX25519PrivateKeyBytes = 32;
    constexpr size_t kX25519KeyBits         = 255;
    constexpr psa_key_type_t kX25519KeyType = PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY);
    const std::array<uint8_t, kX25519PrivateKeyBytes> kAlicePrivateKey {0x77, 0x07, 0x6d, 0x0a, 0x73, 0x18, 0xa5, 0x7d, 0x3c, 0x16, 0xc1, 0x72, 0x51, 0xb2, 0x66, 0x45, 0xdf, 0x4c, 0x2f, 0x87, 0xeb, 0xc0, 0x99, 0x2a, 0xb1, 0x77, 0xfb, 0xa5, 0x1d, 0xb9, 0x2c, 0x2a};
    const std::array<uint8_t, kX25519PrivateKeyBytes> kAlicePublicKey {0x85, 0x20, 0xf0, 0x09, 0x89, 0x30, 0xa7, 0x54, 0x74, 0x8b, 0x7d, 0xdc, 0xb4, 0x3e, 0xf7, 0x5a, 0x0d, 0xbf, 0x3a, 0x0d, 0x26, 0x38, 0x1a, 0xf4, 0xeb, 0xa4, 0xa9, 0x8e, 0xaa, 0x9b, 0x4e, 0x6a};
    const std::array<uint8_t, kX25519PrivateKeyBytes> kBobPublicKey {0xde, 0x9e, 0xdb, 0x7d, 0x7b, 0x7d, 0xc1, 0xb4, 0xd3, 0x5b, 0x61, 0xc2, 0xec, 0xe4, 0x35, 0x37, 0x3f, 0x83, 0x43, 0xc8, 0x5b, 0x78, 0x67, 0x4d, 0xad, 0xfc, 0x7e, 0x14, 0x6f, 0x88, 0x2b, 0x4f};
    const std::array<uint8_t, kX25519PrivateKeyBytes> kX25519SharedSecret {0x4a, 0x5d, 0x9d, 0x5b, 0xa4, 0xce, 0x2d, 0xe1, 0x72, 0x8e, 0x3b, 0xf4, 0x80, 0x35, 0x0f, 0x25, 0xe0, 0x7e, 0x21, 0xc9, 0x47, 0xd1, 0x9e, 0x33, 0x76, 0xf0, 0x9b, 0x3c, 0x1e, 0x16, 0x17, 0x42};
}  // namespace

TEST_F(PsaWrappersTest, GenerateRandom)
{
    std::array<uint8_t, kRandomOutputBytes> output {};
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.generate_random(output));
}

TEST_F(PsaWrappersTest, ImportAesKeyAttributes)
{
    const std::array<uint8_t, kImportKeyStorageBytes> storage {};
    const auto key = std::span(storage).subspan(kImportKeyMaterialOffset, kAes128KeyBytes);
    psa_key_id_t key_id;

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key, key_id, PSA_KEY_TYPE_AES, kAes128KeyBits));

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    const auto status               = psa_get_key_attributes(key_id, &attributes);
    EXPECT_EQ(PSA_SUCCESS, status);
    if (status == PSA_SUCCESS) {
        EXPECT_EQ(PSA_KEY_TYPE_AES, psa_get_key_type(&attributes));
        EXPECT_EQ(kAes128KeyBits, psa_get_key_bits(&attributes));
        EXPECT_EQ(PSA_KEY_USAGE_ENCRYPT, psa_get_key_usage_flags(&attributes));
        EXPECT_EQ(PSA_ALG_ECB_NO_PADDING, psa_get_key_algorithm(&attributes));
    }
    psa_reset_key_attributes(&attributes);
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.destroy_key(key_id));
}

TEST_F(PsaWrappersTest, CipherEncryptBufferTooSmall)
{
    const std::array<uint8_t, kAes128KeyBytes> key {};
    const std::array<uint8_t, kAes128KeyBytes> plaintext {};
    std::array<uint8_t, kCipherOutputStorageBytes> storage;
    storage.fill(kUnmodifiedStorageFill);

    const auto output    = std::span(storage).subspan(kCipherOutputSubspanOffset, plaintext.size() - 1);
    size_t output_length = 0;
    psa_key_id_t key_id  = 0;

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key, key_id, PSA_KEY_TYPE_AES, kAes128KeyBits));
    EXPECT_EQ(PSA_ERROR_BUFFER_TOO_SMALL, psa_wrappers.cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, plaintext, output, output_length));

    EXPECT_EQ(kUnmodifiedStorageFill, storage.front());
    for (size_t index = kCipherOutputSubspanOffset + output.size(); index < storage.size(); ++index) {
        SCOPED_TRACE(index);
        EXPECT_EQ(kUnmodifiedStorageFill, storage[index]);
    }
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.destroy_key(key_id));
}

TEST_F(PsaWrappersTest, ImportKeyBitLengthMismatch)
{
    const std::array<uint8_t, kAes128KeyBytes> key {};
    psa_key_id_t key_id = 0;

    EXPECT_EQ(PSA_ERROR_INVALID_ARGUMENT, psa_wrappers.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_ECB_NO_PADDING, key, key_id, PSA_KEY_TYPE_AES, kAes256KeyBits));
}

TEST_F(PsaWrappersTest, EcbNistVector)
{
    std::array<uint8_t, kAes128KeyBytes> ciphertext {};
    std::array<uint8_t, kAes128KeyBytes> decrypted {};
    size_t output_length = 0;
    psa_key_id_t key_id  = 0;

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.import_key(PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT, PSA_ALG_ECB_NO_PADDING, kNistAes128EcbKey, key_id, PSA_KEY_TYPE_AES, kAes128KeyBits));

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, kNistAes128EcbPlaintext, ciphertext, output_length));
    EXPECT_EQ(ciphertext.size(), output_length);
    EXPECT_EQ(kNistAes128EcbCiphertext, ciphertext);

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_decrypt(key_id, PSA_ALG_ECB_NO_PADDING, ciphertext, decrypted, output_length));
    EXPECT_EQ(decrypted.size(), output_length);
    EXPECT_EQ(kNistAes128EcbPlaintext, decrypted);
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.destroy_key(key_id));
}

TEST_F(PsaWrappersTest, CipherAbortReuse)
{
    const std::array<uint8_t, kAes128KeyBytes> key {};
    const std::array<uint8_t, kAes128KeyBytes> iv {};
    psa_key_id_t key_id              = 0;
    psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_CBC_NO_PADDING, key, key_id, PSA_KEY_TYPE_AES, kAes128KeyBits));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_encrypt_setup(operation, key_id, PSA_ALG_CBC_NO_PADDING));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_set_iv(operation, iv));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_abort(operation));

    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.cipher_abort(operation));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_encrypt_setup(operation, key_id, PSA_ALG_CBC_NO_PADDING));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_set_iv(operation, iv));
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.cipher_abort(operation));
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.destroy_key(key_id));
}

TEST_F(PsaWrappersTest, MacComputeCmac)
{
    std::array<uint8_t, kAes128KeyBytes> mac {};
    size_t mac_length = 0;
    psa_key_id_t key_id;

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.import_key(PSA_KEY_USAGE_SIGN_MESSAGE, PSA_ALG_CMAC, kNistAes128EcbKey, key_id, PSA_KEY_TYPE_AES, kAes128KeyBits));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.mac_compute(key_id, PSA_ALG_CMAC, kNistAes128EcbPlaintext, mac, mac_length));
    EXPECT_EQ(mac.size(), mac_length);
    EXPECT_EQ(kRfc4493Cmac, mac);
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.destroy_key(key_id));
}

TEST_F(PsaWrappersTest, AeadCcmNistExample1)
{
    std::array<uint8_t, kCcmCiphertextBufBytes> ciphertext {};
    std::array<uint8_t, kCcmCiphertextBufBytes> plaintext {};
    size_t ciphertext_length = 0;
    size_t plaintext_length  = 0;
    psa_key_id_t encrypt_key_id;
    psa_key_id_t decrypt_key_id;

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.import_key(PSA_KEY_USAGE_ENCRYPT, kCcmAlgorithm, kNistCcmKey, encrypt_key_id, PSA_KEY_TYPE_AES, kAes128KeyBits));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.aead_encrypt(encrypt_key_id, kCcmAlgorithm, kNistCcmNonce, kNistCcmAad, kNistCcmPlaintext, ciphertext, ciphertext_length));
    EXPECT_EQ(kCcmCiphertextBytes, ciphertext_length);
    EXPECT_EQ(0, std::memcmp(kNistCcmCiphertext.data(), ciphertext.data(), kCcmCiphertextBytes));
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.destroy_key(encrypt_key_id));

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.import_key(PSA_KEY_USAGE_DECRYPT, kCcmAlgorithm, kNistCcmKey, decrypt_key_id, PSA_KEY_TYPE_AES, kAes128KeyBits));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.aead_decrypt(decrypt_key_id, kCcmAlgorithm, kNistCcmNonce, kNistCcmAad, std::span(ciphertext).first(ciphertext_length), plaintext, plaintext_length));
    EXPECT_EQ(kNistCcmPlaintext.size(), plaintext_length);
    EXPECT_EQ(0, std::memcmp(kNistCcmPlaintext.data(), plaintext.data(), plaintext_length));
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.destroy_key(decrypt_key_id));
}

// export_public_key, RFC 7748 Alice
TEST_F(PsaWrappersTest, ExportPublicKey)
{
    std::array<uint8_t, kX25519PrivateKeyBytes> public_key {};
    size_t public_key_length = 0;
    psa_key_id_t key_id;

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.import_key(PSA_KEY_USAGE_DERIVE, PSA_ALG_ECDH, kAlicePrivateKey, key_id, kX25519KeyType, kX25519KeyBits));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.export_public_key(key_id, public_key, public_key_length));
    EXPECT_EQ(public_key.size(), public_key_length);
    EXPECT_EQ(kAlicePublicKey, public_key);
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.destroy_key(key_id));
}

// raw_key_agreement, RFC 7748 Alice and Bob
TEST_F(PsaWrappersTest, RawKeyAgreement)
{
    std::array<uint8_t, kX25519PrivateKeyBytes> shared_secret {};
    size_t shared_secret_length = 0;
    psa_key_id_t key_id;

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.import_key(PSA_KEY_USAGE_DERIVE, PSA_ALG_ECDH, kAlicePrivateKey, key_id, kX25519KeyType, kX25519KeyBits));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.raw_key_agreement(PSA_ALG_ECDH, key_id, kBobPublicKey, shared_secret, shared_secret_length));
    EXPECT_EQ(shared_secret.size(), shared_secret_length);
    EXPECT_EQ(kX25519SharedSecret, shared_secret);
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.destroy_key(key_id));
}

TEST_F(PsaWrappersTest, CipherOfbUpdateFinish)
{
    std::array<uint8_t, kAes128KeyBytes> buffer = kNistAes128EcbPlaintext;
    size_t update_length                        = 0;
    size_t finish_length                        = 0;
    psa_key_id_t key_id                         = 0;
    psa_cipher_operation_t operation            = PSA_CIPHER_OPERATION_INIT;

    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_OFB, kNistAes128EcbKey, key_id, PSA_KEY_TYPE_AES, kAes128KeyBits));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_encrypt_setup(operation, key_id, PSA_ALG_OFB));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_set_iv(operation, kNistAes128OfbIv));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_update(operation, buffer, buffer, update_length));
    ASSERT_EQ(PSA_SUCCESS, psa_wrappers.cipher_finish(operation, std::span(buffer).subspan(update_length), finish_length));
    EXPECT_EQ(buffer.size(), update_length + finish_length);
    EXPECT_EQ(kNistAes128OfbCiphertext, buffer);
    EXPECT_EQ(PSA_SUCCESS, psa_wrappers.destroy_key(key_id));
}
