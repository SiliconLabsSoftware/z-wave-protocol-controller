#include <algorithm>
#include <array>

#include "c_api_test_support.hpp"
#include "crypto_mock.hpp"

extern "C" {
#include "zwave_s2_key_derivation.h"
#include "zwave_s2_next_nonce.h"
}

namespace
{
    class ZwaveS2KeyDerivationFailures : public ::testing::Test
    {
        protected:
            ::testing::StrictMock<crypto::test::CryptoMock> backend;
            unsigned cmac_call_count = 0;

            void SetUp() override
            {
                crypto_c::test::set_backend(backend);
            }

            void TearDown() override
            {
                crypto_c::test::reset_backend();
            }

            void fail_cmac_on_call(unsigned call_number)
            {
                cmac_call_count = 0;
                EXPECT_CALL(backend, aes_cmac_compute(::testing::_, ::testing::_, ::testing::_)).Times(call_number).WillRepeatedly([this, call_number](auto, auto, auto mac) {
                    if (++cmac_call_count == call_number) {
                        return SL_STATUS_FAIL;
                    }
                    std::fill(mac.begin(), mac.end(), static_cast<uint8_t>(cmac_call_count));
                    return SL_STATUS_OK;
                });
            }
    };

    TEST_F(ZwaveS2KeyDerivationFailures, NonceInstantiationLeavesContextUnchangedOnEachCryptoFailure)
    {
        const std::array<uint8_t, 16> sender_entropy {};
        const std::array<uint8_t, 16> receiver_entropy {};
        const std::array<uint8_t, 32> nonce_key {};

        for (unsigned fail_call = 1; fail_call <= 4; ++fail_call) {
            SCOPED_TRACE(fail_call);
            crypto_aes_ctr_drbg_ctx_t ctx;
            std::fill_n(ctx.counter, sizeof(ctx.counter), 0xa5);
            std::fill_n(ctx.key, sizeof(ctx.key), 0xb6);
            const auto original = ctx;
            if (fail_call <= 3) {
                fail_cmac_on_call(fail_call);
            } else {
                EXPECT_CALL(backend, aes_cmac_compute(::testing::_, ::testing::_, ::testing::_)).Times(3).WillRepeatedly([](auto, auto, auto mac) {
                    std::fill(mac.begin(), mac.end(), 0x42);
                    return SL_STATUS_OK;
                });
                EXPECT_CALL(backend, aes_ctr_drbg_init(::testing::_, ::testing::_, ::testing::_)).WillOnce(::testing::Return(SL_STATUS_FAIL));
            }

            EXPECT_EQ(SL_STATUS_FAIL, zwave_s2_next_nonce_instantiate(&ctx, sender_entropy.data(), receiver_entropy.data(), nonce_key.data()));
            EXPECT_THAT(ctx.counter, ::testing::ElementsAreArray(original.counter));
            EXPECT_THAT(ctx.key, ::testing::ElementsAreArray(original.key));
        }
    }

    TEST_F(ZwaveS2KeyDerivationFailures, NetworkKeyExpandLeavesOutputsUnchangedOnEachCmacFailure)
    {
        const std::array<uint8_t, 16> key {};

        for (unsigned fail_call = 1; fail_call <= 4; ++fail_call) {
            std::array<uint8_t, 16> ccm_key;
            std::array<uint8_t, 32> nonce_pers_string;
            std::array<uint8_t, 16> mpan_key;
            ccm_key.fill(0xa1);
            nonce_pers_string.fill(0xb2);
            mpan_key.fill(0xc3);
            const auto expected_ccm_key           = ccm_key;
            const auto expected_nonce_pers_string = nonce_pers_string;
            const auto expected_mpan_key          = mpan_key;
            fail_cmac_on_call(fail_call);

            EXPECT_EQ(SL_STATUS_FAIL, zwave_s2_derive_network_key(key.data(), ccm_key.data(), nonce_pers_string.data(), mpan_key.data()));
            EXPECT_EQ(expected_ccm_key, ccm_key);
            EXPECT_EQ(expected_nonce_pers_string, nonce_pers_string);
            EXPECT_EQ(expected_mpan_key, mpan_key);
        }
    }

    TEST_F(ZwaveS2KeyDerivationFailures, TempKeyExpandLeavesOutputsUnchangedOnCmacFailure)
    {
        const std::array<uint8_t, 16> key {};
        std::array<uint8_t, 16> ccm_key;
        std::array<uint8_t, 32> nonce_pers_string;
        std::array<uint8_t, 16> mpan_key;
        ccm_key.fill(0xa1);
        nonce_pers_string.fill(0xb2);
        mpan_key.fill(0xc3);
        const auto expected_ccm_key           = ccm_key;
        const auto expected_nonce_pers_string = nonce_pers_string;
        const auto expected_mpan_key          = mpan_key;
        fail_cmac_on_call(2);

        EXPECT_EQ(SL_STATUS_FAIL, zwave_s2_derive_temp_key(key.data(), ccm_key.data(), nonce_pers_string.data(), mpan_key.data()));
        EXPECT_EQ(expected_ccm_key, ccm_key);
        EXPECT_EQ(expected_nonce_pers_string, nonce_pers_string);
        EXPECT_EQ(expected_mpan_key, mpan_key);
    }

    TEST_F(ZwaveS2KeyDerivationFailures, ExtractPseudoRandomKeyLeavesOutputUnchangedOnCmacFailure)
    {
        const std::array<uint8_t, 32> shared_secret {};
        const std::array<uint8_t, 64> public_keys {};
        std::array<uint8_t, 16> keymat;
        keymat.fill(0xd4);
        const auto expected_keymat = keymat;
        fail_cmac_on_call(1);

        EXPECT_EQ(SL_STATUS_FAIL, zwave_s2_extract_pseudo_random_key(shared_secret.data(), public_keys.data(), keymat.data()));
        EXPECT_EQ(expected_keymat, keymat);
    }
}  // namespace
