#ifndef TEST_CRYPTO_INJECTION_FIXTURE_HPP_
#define TEST_CRYPTO_INJECTION_FIXTURE_HPP_

#include <array>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algorithms/crypto_impl.hpp"
#include "psa/psa_wrappers.hpp"
#include "psa_wrappers_mock.hpp"
#include "test_fixture.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

namespace
{
    MATCHER_P(SameBuffer, expected, "")
    {
        return arg.data() == expected.data() && arg.size() == expected.size();
    }
}  // namespace

class CryptoInjection : public ::testing::Test
{
    protected:
        crypto::PsaWrappers real_psa_wrappers;
        StrictMock<crypto::test::PsaWrappersMock> mock;
        crypto::Crypto implementation {mock};
        crypto::CryptoInterface &algorithms {implementation};
        const std::array<uint8_t, crypto::aes_key_size> key {};
        const std::array<uint8_t, crypto::aes_block_size> input {};
        std::array<uint8_t, crypto::aes_block_size> output {};

        void SetUp() override
        {
            EXPECT_CALL(mock, crypto_init()).Times(::testing::AnyNumber()).WillRepeatedly(Return(PSA_SUCCESS));
        }

        void expect_import_failure(psa_key_usage_t usage, psa_algorithm_t algorithm)
        {
            EXPECT_CALL(mock, import_key(usage, algorithm, SameBuffer(std::span(key)), _, PSA_KEY_TYPE_AES, crypto::aes_key_bits)).WillOnce(Return(PSA_ERROR_INSUFFICIENT_MEMORY));
        }
};

class CryptoRealInjection : public CryptoInjection
{
    protected:
        void SetUp() override
        {
            CryptoInjection::SetUp();
            ASSERT_EQ(PSA_SUCCESS, real_psa_wrappers.crypto_init());
        }

        void TearDown() override
        {
            mbedtls_psa_crypto_free();
        }
};

class CryptoInjectionMisc : public CryptoInjection
{};
class CryptoRealInjectionMisc : public CryptoRealInjection
{};

class AesEcbInjection : public CryptoInjection
{};
class AesOfbInjection : public CryptoInjection
{};
class AesCbcMacInjection : public CryptoInjection
{};
class AesCmacInjection : public CryptoInjection
{};
class AesCcmInjection : public CryptoInjection
{};
class AesCtrDrbgInjection : public CryptoInjection
{};
class AesCtrDrbgRealInjection : public CryptoRealInjection
{};
class X25519Injection : public CryptoInjection
{};
class X25519RealInjection : public CryptoRealInjection
{};
class RandInjection : public CryptoInjection
{};

enum class CipherFailureStage { Setup, SetIv, Update, Finish };

inline const char *CipherFailureStageName(CipherFailureStage stage)
{
    switch (stage) {
        case CipherFailureStage::Setup:
            return "Setup";
        case CipherFailureStage::SetIv:
            return "SetIv";
        case CipherFailureStage::Update:
            return "Update";
        case CipherFailureStage::Finish:
            return "Finish";
    }
    return "Unknown";
}

class CryptoMultipartFailure : public CryptoInjection, public ::testing::WithParamInterface<CipherFailureStage>
{
    protected:
        psa_cipher_operation_t *operation = nullptr;

        void expect_multipart_failure(psa_algorithm_t algorithm, size_t update_length)
        {
            const auto stage          = GetParam();
            const auto same_operation = ::testing::Truly([this](const psa_cipher_operation_t &candidate) { return &candidate == operation; });
            ::testing::InSequence sequence;

            EXPECT_CALL(mock, crypto_init()).WillOnce(Return(PSA_SUCCESS));
            EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, algorithm, SameBuffer(std::span(key)), _, PSA_KEY_TYPE_AES, crypto::aes_key_bits)).WillOnce(::testing::DoAll(::testing::SetArgReferee<3>(42), Return(PSA_SUCCESS)));
            EXPECT_CALL(mock, cipher_encrypt_setup(_, 42, algorithm)).WillOnce([this, stage](psa_cipher_operation_t &candidate, psa_key_id_t, psa_algorithm_t) {
                operation = &candidate;
                return stage == CipherFailureStage::Setup ? PSA_ERROR_BAD_STATE : PSA_SUCCESS;
            });

            if (stage != CipherFailureStage::Setup) {
                EXPECT_CALL(mock, cipher_set_iv(same_operation, ::testing::SizeIs(crypto::aes_block_size))).WillOnce(Return(stage == CipherFailureStage::SetIv ? PSA_ERROR_BAD_STATE : PSA_SUCCESS));

                if (stage == CipherFailureStage::Update || stage == CipherFailureStage::Finish) {
                    EXPECT_CALL(mock, cipher_update(same_operation, _, _, _)).WillOnce(::testing::DoAll(::testing::SetArgReferee<3>(update_length), Return(stage == CipherFailureStage::Update ? PSA_ERROR_BAD_STATE : PSA_SUCCESS)));
                }

                if (stage == CipherFailureStage::Finish) {
                    EXPECT_CALL(mock, cipher_finish(same_operation, _, _)).WillOnce(Return(PSA_ERROR_BAD_STATE));
                }

                EXPECT_CALL(mock, cipher_abort(same_operation)).WillOnce(Return(PSA_SUCCESS));
            } else {
                EXPECT_CALL(mock, cipher_abort(_)).Times(0);
            }

            EXPECT_CALL(mock, destroy_key(42)).WillOnce(Return(PSA_SUCCESS));
        }
};

class AesOfbMultipartFailure : public CryptoMultipartFailure
{};
class AesCbcMacMultipartFailure : public CryptoMultipartFailure
{};

#endif
