#include <array>
#include <type_traits>

#include "crypto.hpp"
#include "psa/psa_resources.hpp"
#include "psa_wrappers_mock.hpp"

using crypto::detail::ScopedCipherOperation;
using crypto::detail::ScopedKey;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;

static_assert(!std::is_copy_constructible_v<ScopedKey>);
static_assert(!std::is_copy_assignable_v<ScopedKey>);
static_assert(!std::is_move_constructible_v<ScopedKey>);
static_assert(!std::is_move_assignable_v<ScopedKey>);
static_assert(!std::is_copy_constructible_v<ScopedCipherOperation>);
static_assert(!std::is_copy_assignable_v<ScopedCipherOperation>);
static_assert(!std::is_move_constructible_v<ScopedCipherOperation>);
static_assert(!std::is_move_assignable_v<ScopedCipherOperation>);
static_assert(std::is_nothrow_destructible_v<ScopedKey>);
static_assert(std::is_nothrow_destructible_v<ScopedCipherOperation>);

namespace
{
    class PsaResources : public ::testing::Test
    {
        protected:
            ::testing::StrictMock<crypto::test::PsaWrappersMock> mock;
            const std::array<uint8_t, crypto::aes_key_size> key {};
    };

    TEST_F(PsaResources, UnusedResourcesDoNotCallCleanup)
    {
        ScopedKey imported_key(mock);
        ScopedCipherOperation operation(mock);
    }

    TEST_F(PsaResources, FailedImportDoesNotAdoptWrittenKeyId)
    {
        EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_OFB, _, _, PSA_KEY_TYPE_AES, crypto::aes_key_bits)).WillOnce(DoAll(SetArgReferee<3>(42), Return(PSA_ERROR_INSUFFICIENT_MEMORY)));
        EXPECT_CALL(mock, destroy_key(_)).Times(0);

        ScopedKey imported_key(mock);
        EXPECT_EQ(PSA_ERROR_INSUFFICIENT_MEMORY, imported_key.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_OFB, key, PSA_KEY_TYPE_AES, crypto::aes_key_bits));
        EXPECT_EQ(PSA_KEY_ID_NULL, imported_key.id());
    }

    TEST_F(PsaResources, OwnedKeyCannotBeReplacedAndIsDestroyedAtScopeExit)
    {
        bool destroyed = false;
        EXPECT_CALL(mock, import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_OFB, _, _, PSA_KEY_TYPE_AES, crypto::aes_key_bits)).WillOnce(DoAll(SetArgReferee<3>(42), Return(PSA_SUCCESS)));
        EXPECT_CALL(mock, destroy_key(42)).WillOnce([&](auto) {
            destroyed = true;
            return PSA_SUCCESS;
        });

        {
            ScopedKey imported_key(mock);
            ASSERT_EQ(PSA_SUCCESS, imported_key.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_OFB, key, PSA_KEY_TYPE_AES, crypto::aes_key_bits));
            EXPECT_EQ(42u, imported_key.id());
            EXPECT_EQ(PSA_ERROR_BAD_STATE, imported_key.import_key(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_OFB, key, PSA_KEY_TYPE_AES, crypto::aes_key_bits));
            EXPECT_FALSE(destroyed);
        }
        EXPECT_TRUE(destroyed);
    }

    TEST_F(PsaResources, FailedCipherSetupDoesNotAbort)
    {
        EXPECT_CALL(mock, cipher_encrypt_setup(_, 42, PSA_ALG_OFB)).WillOnce(Return(PSA_ERROR_BAD_STATE));
        EXPECT_CALL(mock, cipher_abort(_)).Times(0);

        ScopedCipherOperation operation(mock);
        EXPECT_EQ(PSA_ERROR_BAD_STATE, operation.encrypt_setup(42, PSA_ALG_OFB));
    }

    TEST_F(PsaResources, ActiveCipherCannotBeReconfiguredAndIsAbortedOnce)
    {
        psa_cipher_operation_t *active_operation = nullptr;
        EXPECT_CALL(mock, cipher_encrypt_setup(_, 42, PSA_ALG_OFB)).WillOnce([&](auto &operation, auto, auto) {
            active_operation = &operation;
            return PSA_SUCCESS;
        });
        EXPECT_CALL(mock, cipher_abort(_)).WillOnce([&](auto &operation) {
            EXPECT_EQ(active_operation, &operation);
            return PSA_SUCCESS;
        });

        ScopedCipherOperation operation(mock);
        ASSERT_EQ(PSA_SUCCESS, operation.encrypt_setup(42, PSA_ALG_OFB));
        EXPECT_EQ(PSA_ERROR_BAD_STATE, operation.encrypt_setup(43, PSA_ALG_CBC_NO_PADDING));
    }

    TEST_F(PsaResources, SuccessfulFinishDoesNotAbort)
    {
        std::array<uint8_t, crypto::aes_block_size> output {};
        size_t length = 0;
        EXPECT_CALL(mock, cipher_encrypt_setup(_, 42, PSA_ALG_OFB)).WillOnce(Return(PSA_SUCCESS));
        EXPECT_CALL(mock, cipher_finish(_, _, _)).WillOnce(DoAll(SetArgReferee<2>(output.size()), Return(PSA_SUCCESS)));
        EXPECT_CALL(mock, cipher_abort(_)).Times(0);

        ScopedCipherOperation operation(mock);
        ASSERT_EQ(PSA_SUCCESS, operation.encrypt_setup(42, PSA_ALG_OFB));
        EXPECT_EQ(PSA_SUCCESS, operation.finish(output, length));
        EXPECT_EQ(output.size(), length);
    }
}  // namespace
