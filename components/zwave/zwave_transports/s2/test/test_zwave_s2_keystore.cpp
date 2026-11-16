#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

#include "c_api_test_support.hpp"
#include "crypto_mock.hpp"
#include "zwave_s2_keystore.h"
#include "zwave_s2_keystore_int.h"
#include "zwave_controller_endian.h"

extern "C" {
#include "s2_keystore.h"
#include "s2_inclusion.h"
#include "zwapi_init.h"
#include "zwave_s0_transport.h"
}

namespace
{
    nvm_config_t nvm;
    std::vector<uint16_t> writes;
    sl_status_t read_status;

    class ZwaveS2Keystore : public ::testing::Test
    {
        protected:
            ::testing::StrictMock<crypto::test::CryptoMock> backend;
            unsigned random_call_count = 0;

            void SetUp() override
            {
                std::memset(&nvm, 0xa5, sizeof(nvm));
                writes.clear();
                read_status = SL_STATUS_OK;
                crypto_c::test::set_backend(backend);
            }

            void TearDown() override
            {
                crypto_c::test::reset_backend();
            }

            void generate_keys(unsigned calls, unsigned fail_on_call = 0)
            {
                random_call_count = 0;
                EXPECT_CALL(backend, generate_random(::testing::_)).Times(calls).WillRepeatedly([this, fail_on_call](auto output) {
                    ++random_call_count;
                    std::fill(output.begin(), output.end(), static_cast<uint8_t>(random_call_count));
                    return random_call_count == fail_on_call ? SL_STATUS_NOT_INITIALIZED : SL_STATUS_OK;
                });
            }
    };

    TEST_F(ZwaveS2Keystore, FailedFirstEcdhGenerationDoesNotPersistKeyOrMarkNvmValid)
    {
        const auto original = nvm;
        EXPECT_CALL(backend, generate_random(::testing::_)).WillOnce([](auto output) {
            // A failed backend may have already written part of its output.
            std::fill(output.begin(), output.end(), 0);
            return SL_STATUS_NOT_INITIALIZED;
        });

        EXPECT_EQ(SL_STATUS_NOT_INITIALIZED, zwave_s2_keystore_init());

        EXPECT_TRUE(writes.empty());
        EXPECT_EQ(0, std::memcmp(&nvm, &original, sizeof(nvm)));
    }

    TEST_F(ZwaveS2Keystore, FailedSecondEcdhGenerationLeavesNvmUninitialized)
    {
        const auto original_magic = nvm.magic;
        generate_keys(2, 2);

        EXPECT_EQ(SL_STATUS_NOT_INITIALIZED, zwave_s2_keystore_init());

        EXPECT_EQ(original_magic, nvm.magic);
        ASSERT_EQ(1U, writes.size());
        EXPECT_EQ(offsetof(nvm_config_t, ecdh_priv_key), writes.front());
        EXPECT_TRUE(std::all_of(std::begin(nvm.ecdh_priv_key), std::end(nvm.ecdh_priv_key), [](auto byte) { return byte == 1; }));
    }

    TEST_F(ZwaveS2Keystore, FailedFinalEcdhGenerationLeavesNvmUninitialized)
    {
        const auto original_magic = nvm.magic;
        generate_keys(9, 9);

        EXPECT_EQ(SL_STATUS_NOT_INITIALIZED, zwave_s2_keystore_init());

        EXPECT_EQ(original_magic, nvm.magic);
        EXPECT_EQ(writes.end(), std::find(writes.begin(), writes.end(), offsetof(nvm_config_t, magic)));
    }

    TEST_F(ZwaveS2Keystore, ExistingNvmStillPropagatesEcdhGenerationFailure)
    {
        nvm.magic           = zwave_controller_ntohl(NVM_MAGIC);
        const auto original = nvm;
        generate_keys(1, 1);

        EXPECT_EQ(SL_STATUS_NOT_INITIALIZED, zwave_s2_keystore_init());

        EXPECT_TRUE(writes.empty());
        EXPECT_EQ(0, std::memcmp(&nvm, &original, sizeof(nvm)));
    }

    TEST_F(ZwaveS2Keystore, SuccessfulInitializationPersistsGeneratedKeyAndMarksNvmValidLast)
    {
        generate_keys(9);

        EXPECT_EQ(SL_STATUS_OK, zwave_s2_keystore_init());

        EXPECT_EQ(NVM_MAGIC, zwave_controller_ntohl(nvm.magic));
        ASSERT_FALSE(writes.empty());
        EXPECT_EQ(offsetof(nvm_config_t, magic), writes.back());
        EXPECT_TRUE(std::all_of(std::begin(nvm.ecdh_priv_key), std::end(nvm.ecdh_priv_key), [](auto byte) { return byte == 1; }));
        std::array<uint8_t, 32> dynamic_key;
        zwave_s2_keystore_set_ecdh_key_mode(ZWAVE_S2_KEYSTORE_DYNAMIC_ECDH_KEY);
        keystore_private_key_read(dynamic_key.data());
        EXPECT_TRUE(std::all_of(dynamic_key.begin(), dynamic_key.end(), [](auto byte) { return byte == 9; }));
    }

    TEST_F(ZwaveS2Keystore, InitializationCanRetryAfterEcdhGenerationFailure)
    {
        generate_keys(1, 1);
        EXPECT_EQ(SL_STATUS_NOT_INITIALIZED, zwave_s2_keystore_init());
        ASSERT_TRUE(::testing::Mock::VerifyAndClearExpectations(&backend));

        generate_keys(9);
        EXPECT_EQ(SL_STATUS_OK, zwave_s2_keystore_init());
        EXPECT_EQ(NVM_MAGIC, zwave_controller_ntohl(nvm.magic));
    }

    TEST_F(ZwaveS2Keystore, FailedNvmReadStopsInitialization)
    {
        read_status = SL_STATUS_FAIL;
        EXPECT_CALL(backend, generate_random(::testing::_)).Times(0);

        EXPECT_EQ(SL_STATUS_FAIL, zwave_s2_keystore_init());

        EXPECT_TRUE(writes.empty());
    }
}  // namespace

extern "C" {
sl_status_t zwapi_memory_get_buffer(uint16_t offset, uint8_t *buffer, uint8_t length)
{
    std::memcpy(buffer, reinterpret_cast<const uint8_t *>(&nvm) + offset, length);
    return read_status;
}

uint8_t zwapi_memory_put_buffer(uint16_t offset, const uint8_t *buffer, uint16_t length, void (*)(void))
{
    writes.push_back(offset);
    std::memcpy(reinterpret_cast<uint8_t *>(&nvm) + offset, buffer, length);
    return 1;
}

void zwapi_get_chip_type_version(uint8_t *type, uint8_t *version)
{
    *type    = 0;
    *version = 0;
}

sl_status_t zwapi_nvr_get_value(uint8_t, uint8_t length, uint8_t *buffer)
{
    std::memset(buffer, 0, length);
    return SL_STATUS_OK;
}

uint8_t s2_get_key_count(void)
{
    return 6;
}

void s0_set_key(const uint8_t *) {}

void sl_log(const char *const, sl_log_level_t, const char *, ...) {}
}
