#include <algorithm>
#include <array>
#include <string>
#include <tuple>

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
            unsigned cmac_call_count                    = 0;
            static constexpr sl_status_t failure_status = SL_STATUS_NOT_INITIALIZED;

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
                    ++cmac_call_count;
                    std::fill(mac.begin(), mac.end(), static_cast<uint8_t>(cmac_call_count));
                    return cmac_call_count == call_number ? failure_status : SL_STATUS_OK;
                });
            }
    };

    class ZwaveS2NonceFailure : public ZwaveS2KeyDerivationFailures, public ::testing::WithParamInterface<unsigned>
    {};

    TEST_P(ZwaveS2NonceFailure, PropagatesFailureAndLeavesContextUnchanged)
    {
        const std::array<uint8_t, 16> sender_entropy {};
        const std::array<uint8_t, 16> receiver_entropy {};
        const std::array<uint8_t, 32> nonce_key {};

        const unsigned fail_call = GetParam();
        crypto_aes_ctr_drbg_ctx_t ctx;
        std::fill_n(ctx.counter, sizeof(ctx.counter), 0xa5);
        std::fill_n(ctx.key, sizeof(ctx.key), 0xb6);
        const auto original = ctx;
        const ::testing::InSequence sequence;
        if (fail_call <= 3) {
            fail_cmac_on_call(fail_call);
        } else {
            EXPECT_CALL(backend, aes_cmac_compute(::testing::_, ::testing::_, ::testing::_)).Times(3).WillRepeatedly([](auto, auto, auto mac) {
                std::fill(mac.begin(), mac.end(), 0x42);
                return SL_STATUS_OK;
            });
            EXPECT_CALL(backend, aes_ctr_drbg_init(::testing::_, ::testing::_, ::testing::_)).WillOnce([](auto &next_ctx, auto, auto) {
                next_ctx.key.fill(0x11);
                next_ctx.counter.fill(0x22);
                return failure_status;
            });
        }

        EXPECT_EQ(failure_status, zwave_s2_next_nonce_instantiate(&ctx, sender_entropy.data(), receiver_entropy.data(), nonce_key.data()));
        EXPECT_THAT(ctx.counter, ::testing::ElementsAreArray(original.counter));
        EXPECT_THAT(ctx.key, ::testing::ElementsAreArray(original.key));
    }

    INSTANTIATE_TEST_SUITE_P(CryptoSteps, ZwaveS2NonceFailure, ::testing::Values(1U, 2U, 3U, 4U), [](const auto &info) { return info.param == 4 ? std::string("DrbgInit") : "Cmac" + std::to_string(info.param); });

    struct KeyExpansion {
            const char *name;
            decltype(&zwave_s2_derive_network_key) derive;
    };

    const auto key_expansions = ::testing::Values(KeyExpansion {"NetworkKey", zwave_s2_derive_network_key}, KeyExpansion {"TempKey", zwave_s2_derive_temp_key});

    class ZwaveS2KeyExpansion : public ZwaveS2KeyDerivationFailures, public ::testing::WithParamInterface<std::tuple<KeyExpansion, unsigned>>
    {};

    TEST_P(ZwaveS2KeyExpansion, PropagatesCmacFailureAndLeavesOutputsUnchanged)
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
        const auto &[expansion, fail_call]    = GetParam();
        fail_cmac_on_call(fail_call);

        EXPECT_EQ(failure_status, expansion.derive(key.data(), ccm_key.data(), nonce_pers_string.data(), mpan_key.data()));
        EXPECT_EQ(expected_ccm_key, ccm_key);
        EXPECT_EQ(expected_nonce_pers_string, nonce_pers_string);
        EXPECT_EQ(expected_mpan_key, mpan_key);
    }

    INSTANTIATE_TEST_SUITE_P(CmacSteps, ZwaveS2KeyExpansion, ::testing::Combine(key_expansions, ::testing::Values(1U, 2U, 3U, 4U)), [](const auto &info) { return std::string(std::get<0>(info.param).name) + "_Cmac" + std::to_string(std::get<1>(info.param)); });

    TEST_F(ZwaveS2KeyDerivationFailures, ExtractPseudoRandomKeyLeavesOutputUnchangedOnCmacFailure)
    {
        const std::array<uint8_t, 32> shared_secret {};
        const std::array<uint8_t, 64> public_keys {};
        std::array<uint8_t, 16> keymat;
        keymat.fill(0xd4);
        const auto expected_keymat = keymat;
        fail_cmac_on_call(1);

        EXPECT_EQ(failure_status, zwave_s2_extract_pseudo_random_key(shared_secret.data(), public_keys.data(), keymat.data()));
        EXPECT_EQ(expected_keymat, keymat);
    }

    class ZwaveS2KeyExpansionNull : public ZwaveS2KeyDerivationFailures, public ::testing::WithParamInterface<std::tuple<KeyExpansion, unsigned>>
    {};

    TEST_P(ZwaveS2KeyExpansionNull, RejectsNullBeforeCryptoAndLeavesOutputsUnchanged)
    {
        const std::array<uint8_t, 16> key {};
        std::array<uint8_t, 16> ccm_key;
        std::array<uint8_t, 32> nonce_pers_string;
        std::array<uint8_t, 16> mpan_key;
        ccm_key.fill(0xa1);
        nonce_pers_string.fill(0xb2);
        mpan_key.fill(0xc3);
        const auto &[expansion, null_arg] = GetParam();

        EXPECT_EQ(SL_STATUS_NULL_POINTER, expansion.derive(null_arg == 0 ? nullptr : key.data(), null_arg == 1 ? nullptr : ccm_key.data(), null_arg == 2 ? nullptr : nonce_pers_string.data(), null_arg == 3 ? nullptr : mpan_key.data()));
        EXPECT_THAT(ccm_key, ::testing::Each(0xa1));
        EXPECT_THAT(nonce_pers_string, ::testing::Each(0xb2));
        EXPECT_THAT(mpan_key, ::testing::Each(0xc3));
    }

    INSTANTIATE_TEST_SUITE_P(RequiredArguments, ZwaveS2KeyExpansionNull, ::testing::Combine(key_expansions, ::testing::Values(0U, 1U, 2U, 3U)), ([](const auto &info) {
                                 const char *arguments[] = {"Key", "CcmKey", "NoncePersonalization", "MpanKey"};
                                 return std::string(std::get<0>(info.param).name) + "_" + arguments[std::get<1>(info.param)];
                             }));

    class ZwaveS2ExtractNull : public ZwaveS2KeyDerivationFailures, public ::testing::WithParamInterface<unsigned>
    {};

    TEST_P(ZwaveS2ExtractNull, RejectsNullBeforeCryptoAndLeavesOutputUnchanged)
    {
        const std::array<uint8_t, 32> shared_secret {};
        const std::array<uint8_t, 64> public_keys {};
        std::array<uint8_t, 16> keymat;
        keymat.fill(0xd4);
        const auto null_arg = GetParam();

        EXPECT_EQ(SL_STATUS_NULL_POINTER, zwave_s2_extract_pseudo_random_key(null_arg == 0 ? nullptr : shared_secret.data(), null_arg == 1 ? nullptr : public_keys.data(), null_arg == 2 ? nullptr : keymat.data()));
        EXPECT_THAT(keymat, ::testing::Each(0xd4));
    }

    INSTANTIATE_TEST_SUITE_P(RequiredArguments, ZwaveS2ExtractNull, ::testing::Values(0U, 1U, 2U), ([](const auto &info) {
                                 const char *arguments[] = {"SharedSecret", "PublicKeys", "PseudoRandomKey"};
                                 return arguments[info.param];
                             }));

    class ZwaveS2NonceNull : public ZwaveS2KeyDerivationFailures, public ::testing::WithParamInterface<unsigned>
    {};

    TEST_P(ZwaveS2NonceNull, RejectsNullBeforeCryptoAndLeavesContextUnchanged)
    {
        const std::array<uint8_t, 16> sender_entropy {};
        const std::array<uint8_t, 16> receiver_entropy {};
        const std::array<uint8_t, 32> nonce_key {};
        crypto_aes_ctr_drbg_ctx_t ctx;
        std::fill_n(ctx.counter, sizeof(ctx.counter), 0xa5);
        std::fill_n(ctx.key, sizeof(ctx.key), 0xb6);
        const auto null_arg = GetParam();

        EXPECT_EQ(SL_STATUS_NULL_POINTER, zwave_s2_next_nonce_instantiate(null_arg == 0 ? nullptr : &ctx, null_arg == 1 ? nullptr : sender_entropy.data(), null_arg == 2 ? nullptr : receiver_entropy.data(), null_arg == 3 ? nullptr : nonce_key.data()));
        EXPECT_THAT(ctx.counter, ::testing::Each(0xa5));
        EXPECT_THAT(ctx.key, ::testing::Each(0xb6));
    }

    INSTANTIATE_TEST_SUITE_P(RequiredArguments, ZwaveS2NonceNull, ::testing::Values(0U, 1U, 2U, 3U), ([](const auto &info) {
                                 const char *arguments[] = {"Context", "SenderEntropy", "ReceiverEntropy", "NonceKey"};
                                 return arguments[info.param];
                             }));
}  // namespace
