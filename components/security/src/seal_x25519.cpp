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

#include "seal_x25519.hpp"

#include "log.h"

#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <cstring>
#include <memory>

namespace security
{
    namespace
    {
        constexpr std::string_view LOG_TAG = "seal_x25519";

        struct EvpPkeyDeleter {
                void operator()(EVP_PKEY *p) const noexcept
                {
                    EVP_PKEY_free(p);
                }
        };
        struct EvpPkeyCtxDeleter {
                void operator()(EVP_PKEY_CTX *p) const noexcept
                {
                    EVP_PKEY_CTX_free(p);
                }
        };
        struct EvpCipherCtxDeleter {
                void operator()(EVP_CIPHER_CTX *p) const noexcept
                {
                    EVP_CIPHER_CTX_free(p);
                }
        };
        struct BioDeleter {
                void operator()(BIO *p) const noexcept
                {
                    BIO_free(p);
                }
        };
        using PkeyPtr      = std::unique_ptr<EVP_PKEY, EvpPkeyDeleter>;
        using PkeyCtxPtr   = std::unique_ptr<EVP_PKEY_CTX, EvpPkeyCtxDeleter>;
        using CipherCtxPtr = std::unique_ptr<EVP_CIPHER_CTX, EvpCipherCtxDeleter>;
        using BioPtr       = std::unique_ptr<BIO, BioDeleter>;

        sl_status_t load_recipient_pubkey_raw(const std::string &pem_path, std::array<uint8_t, X25519_KEY_BYTES> &out_pub)
        {
            BioPtr bio(BIO_new_file(pem_path.c_str(), "r"));
            if (!bio) {
                sl_log_error(LOG_TAG.data(), "Cannot open recipient pubkey PEM: %s", pem_path.c_str());
                return SL_STATUS_NOT_FOUND;
            }

            PkeyPtr pkey(PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr));
            if (!pkey) {
                sl_log_error(LOG_TAG.data(), "Failed to parse recipient public key from PEM: %s", pem_path.c_str());
                return SL_STATUS_FAIL;
            }

            if (EVP_PKEY_id(pkey.get()) != EVP_PKEY_X25519) {
                sl_log_error(LOG_TAG.data(), "Recipient public key is not X25519 (id=%d)", EVP_PKEY_id(pkey.get()));
                return SL_STATUS_INVALID_TYPE;
            }

            std::size_t raw_len = out_pub.size();
            if (EVP_PKEY_get_raw_public_key(pkey.get(), out_pub.data(), &raw_len) != 1 || raw_len != X25519_KEY_BYTES) {
                sl_log_error(LOG_TAG.data(), "Failed to extract raw X25519 public key (len=%zu)", raw_len);
                return SL_STATUS_FAIL;
            }

            return SL_STATUS_OK;
        }

        sl_status_t generate_ephemeral_keypair(PkeyPtr &out_pkey, std::array<uint8_t, X25519_KEY_BYTES> &out_pub)
        {
            PkeyCtxPtr ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr));
            if (!ctx || EVP_PKEY_keygen_init(ctx.get()) != 1) {
                sl_log_error(LOG_TAG.data(), "Ephemeral X25519 keygen init failed");
                return SL_STATUS_FAIL;
            }
            EVP_PKEY *raw = nullptr;
            if (EVP_PKEY_keygen(ctx.get(), &raw) != 1 || (raw == nullptr)) {
                sl_log_error(LOG_TAG.data(), "Ephemeral X25519 keygen failed");
                return SL_STATUS_FAIL;
            }
            out_pkey.reset(raw);

            std::size_t len = out_pub.size();
            if (EVP_PKEY_get_raw_public_key(out_pkey.get(), out_pub.data(), &len) != 1 || len != X25519_KEY_BYTES) {
                sl_log_error(LOG_TAG.data(), "Failed to extract raw ephemeral X25519 public key (len=%zu)", len);
                return SL_STATUS_FAIL;
            }
            return SL_STATUS_OK;
        }

        sl_status_t evp_pkey_from_raw_x25519_pub(const std::array<uint8_t, X25519_KEY_BYTES> &raw_pub, PkeyPtr &out_pkey)
        {
            EVP_PKEY *recipient = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr, raw_pub.data(), raw_pub.size());
            if (recipient == nullptr) {
                sl_log_error(LOG_TAG.data(), "Failed to build EVP_PKEY from raw recipient X25519 public key");
                return SL_STATUS_FAIL;
            }
            out_pkey.reset(recipient);
            return SL_STATUS_OK;
        }

        // Run ECDH(eph_priv, recipient_pub) producing a 32-byte shared secret.
        sl_status_t x25519_ecdh(EVP_PKEY *eph_priv, EVP_PKEY *recipient_pub, std::array<uint8_t, X25519_KEY_BYTES> &out_secret)
        {
            PkeyCtxPtr ctx(EVP_PKEY_CTX_new(eph_priv, nullptr));
            if (!ctx || EVP_PKEY_derive_init(ctx.get()) != 1 || EVP_PKEY_derive_set_peer(ctx.get(), recipient_pub) != 1) {
                sl_log_error(LOG_TAG.data(), "X25519 ECDH derive setup failed");
                return SL_STATUS_FAIL;
            }
            std::size_t len = out_secret.size();
            if (EVP_PKEY_derive(ctx.get(), out_secret.data(), &len) != 1 || len != X25519_KEY_BYTES) {
                sl_log_error(LOG_TAG.data(), "X25519 ECDH derive failed (len=%zu)", len);
                return SL_STATUS_FAIL;
            }
            return SL_STATUS_OK;
        }

        sl_status_t hkdf_sha256(const uint8_t *ikm, std::size_t ikm_len, const uint8_t *salt, std::size_t salt_len, const uint8_t *info, std::size_t info_len, uint8_t *out, std::size_t out_len)
        {
            PkeyCtxPtr ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr));
            if (!ctx || EVP_PKEY_derive_init(ctx.get()) != 1 || EVP_PKEY_CTX_set_hkdf_md(ctx.get(), EVP_sha256()) != 1 || EVP_PKEY_CTX_set1_hkdf_salt(ctx.get(), salt, static_cast<int>(salt_len)) != 1 || EVP_PKEY_CTX_set1_hkdf_key(ctx.get(), ikm, static_cast<int>(ikm_len)) != 1
                || EVP_PKEY_CTX_add1_hkdf_info(ctx.get(), info, static_cast<int>(info_len)) != 1) {
                sl_log_error(LOG_TAG.data(), "HKDF-SHA256 setup failed");
                return SL_STATUS_FAIL;
            }
            std::size_t len = out_len;
            if (EVP_PKEY_derive(ctx.get(), out, &len) != 1 || len != out_len) {
                sl_log_error(LOG_TAG.data(), "HKDF-SHA256 derive failed (len=%zu, expected %zu)", len, out_len);
                return SL_STATUS_FAIL;
            }
            return SL_STATUS_OK;
        }

        sl_status_t aes_256_gcm_seal(const uint8_t *key /* 32 */, const uint8_t *iv /* 12 */, const uint8_t *aad, std::size_t aad_len, const uint8_t *pt, std::size_t pt_len, uint8_t *out_ct, uint8_t out_tag[AES_GCM_TAG_BYTES])
        {
            CipherCtxPtr ctx(EVP_CIPHER_CTX_new());
            if (!ctx) {
                sl_log_error(LOG_TAG.data(), "EVP_CIPHER_CTX_new failed");
                return SL_STATUS_FAIL;
            }
            if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 || EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(AES_GCM_IV_BYTES), nullptr) != 1 || EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, key, iv) != 1) {
                sl_log_error(LOG_TAG.data(), "AES-256-GCM encrypt init failed");
                return SL_STATUS_FAIL;
            }
            int out_len = 0;
            if (aad_len > 0) {
                if (EVP_EncryptUpdate(ctx.get(), nullptr, &out_len, aad, static_cast<int>(aad_len)) != 1) {
                    sl_log_error(LOG_TAG.data(), "AES-256-GCM AAD update failed");
                    return SL_STATUS_FAIL;
                }
            }
            if (pt_len > 0) {
                if (EVP_EncryptUpdate(ctx.get(), out_ct, &out_len, pt, static_cast<int>(pt_len)) != 1) {
                    sl_log_error(LOG_TAG.data(), "AES-256-GCM plaintext update failed");
                    return SL_STATUS_FAIL;
                }
            }
            int final_len = 0;
            if (EVP_EncryptFinal_ex(ctx.get(), out_ct + out_len, &final_len) != 1) {
                sl_log_error(LOG_TAG.data(), "AES-256-GCM encrypt final failed");
                return SL_STATUS_FAIL;
            }
            if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, AES_GCM_TAG_BYTES, out_tag) != 1) {
                sl_log_error(LOG_TAG.data(), "AES-256-GCM get tag failed");
                return SL_STATUS_FAIL;
            }
            return SL_STATUS_OK;
        }
    }  // namespace

    sl_status_t seal_to_x25519_pem(const std::string &recipient_pubkey_pem_path, const uint8_t *plaintext, std::size_t plaintext_len, std::vector<uint8_t> &out_envelope, std::array<uint8_t, SHA256_DIGEST_LENGTH> *out_recipient_fingerprint)
    {
        if (plaintext_len > 0 && plaintext == nullptr) {
            return SL_STATUS_NULL_POINTER;
        }

        // 1. Load recipient public key.
        std::array<uint8_t, X25519_KEY_BYTES> recipient_pub {};
        sl_status_t st = load_recipient_pubkey_raw(recipient_pubkey_pem_path, recipient_pub);
        if (st != SL_STATUS_OK) {
            return st;
        }

        // Optional audit fingerprint of the recipient key.
        if (out_recipient_fingerprint != nullptr) {
            SHA256(recipient_pub.data(), recipient_pub.size(), out_recipient_fingerprint->data());
        }

        // 2. Generate ephemeral keypair.
        PkeyPtr eph_priv;
        std::array<uint8_t, X25519_KEY_BYTES> eph_pub {};
        st = generate_ephemeral_keypair(eph_priv, eph_pub);
        if (st != SL_STATUS_OK) {
            return SL_STATUS_ABORT;
        }

        // 3. ECDH(eph_priv, recipient_pub) -> 32-byte shared secret.
        PkeyPtr recipient_pkey;
        st = evp_pkey_from_raw_x25519_pub(recipient_pub, recipient_pkey);
        if (st != SL_STATUS_OK) {
            return SL_STATUS_ABORT;
        }
        std::array<uint8_t, X25519_KEY_BYTES> shared {};
        st = x25519_ecdh(eph_priv.get(), recipient_pkey.get(), shared);
        if (st != SL_STATUS_OK) {
            OPENSSL_cleanse(shared.data(), shared.size());
            return SL_STATUS_ABORT;
        }

        // 4. HKDF(SHA256) -> 32 byte AES-256 key.
        //    salt = eph_pub || recipient_pub
        //    info = "HKDF_INFO"
        std::array<uint8_t, 2 * X25519_KEY_BYTES> salt {};
        std::memcpy(salt.data(), eph_pub.data(), X25519_KEY_BYTES);
        std::memcpy(salt.data() + X25519_KEY_BYTES, recipient_pub.data(), X25519_KEY_BYTES);

        std::array<uint8_t, AES_GCM_KEY_BYTES> aes_key {};
        st = hkdf_sha256(shared.data(), shared.size(), salt.data(), salt.size(), reinterpret_cast<const uint8_t *>(HKDF_INFO), sizeof(HKDF_INFO) - 1, aes_key.data(), aes_key.size());
        OPENSSL_cleanse(shared.data(), shared.size());  // No longer needed.
        if (st != SL_STATUS_OK) {
            OPENSSL_cleanse(aes_key.data(), aes_key.size());
            return SL_STATUS_ABORT;
        }

        // 5. Random IV.
        std::array<uint8_t, AES_GCM_IV_BYTES> iv {};
        if (RAND_bytes(iv.data(), static_cast<int>(iv.size())) != 1) {
            sl_log_error(LOG_TAG.data(), "RAND_bytes failed for AES-GCM IV");
            OPENSSL_cleanse(aes_key.data(), aes_key.size());
            return SL_STATUS_ABORT;
        }

        // 6. AAD = magic || version || eph_pub || recipient_pub
        const std::size_t aad_len = SEAL_MAGIC.size() + 1 + X25519_KEY_BYTES + X25519_KEY_BYTES;
        std::vector<uint8_t> aad(aad_len);
        std::size_t pos = 0;
        std::memcpy(aad.data() + pos, SEAL_MAGIC.data(), SEAL_MAGIC.size());
        pos += SEAL_MAGIC.size();
        aad[pos++] = SEAL_VERSION;
        std::memcpy(aad.data() + pos, eph_pub.data(), X25519_KEY_BYTES);
        pos += X25519_KEY_BYTES;
        std::memcpy(aad.data() + pos, recipient_pub.data(), X25519_KEY_BYTES);

        // 7. AES-256-GCM encrypt.
        const std::size_t header_len = SEAL_MAGIC.size() + 1 + X25519_KEY_BYTES + AES_GCM_IV_BYTES;
        out_envelope.assign(header_len + plaintext_len + AES_GCM_TAG_BYTES, 0);

        // Compose the envelope in-place.
        pos = 0;
        std::memcpy(out_envelope.data() + pos, SEAL_MAGIC.data(), SEAL_MAGIC.size());
        pos += SEAL_MAGIC.size();
        out_envelope[pos++] = SEAL_VERSION;
        std::memcpy(out_envelope.data() + pos, eph_pub.data(), X25519_KEY_BYTES);
        pos += X25519_KEY_BYTES;
        std::memcpy(out_envelope.data() + pos, iv.data(), AES_GCM_IV_BYTES);
        pos += AES_GCM_IV_BYTES;
        // Ciphertext follows the header; tag at the very end.
        uint8_t *ct_out  = out_envelope.data() + header_len;
        uint8_t *tag_out = out_envelope.data() + header_len + plaintext_len;
        st               = aes_256_gcm_seal(aes_key.data(), iv.data(), aad.data(), aad.size(), plaintext, plaintext_len, ct_out, tag_out);
        OPENSSL_cleanse(aes_key.data(), aes_key.size());
        if (st != SL_STATUS_OK) {
            out_envelope.clear();
            return SL_STATUS_ABORT;
        }
        return SL_STATUS_OK;
    }
}  // namespace security
