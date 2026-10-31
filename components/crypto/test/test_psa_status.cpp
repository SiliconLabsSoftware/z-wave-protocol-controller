#include <array>

#include <gtest/gtest.h>

#include "psa_status.hpp"

TEST(PsaStatus, MapsKnownStatuses)
{
    struct StatusMapping {
            psa_status_t psa_status;
            sl_status_t sl_status;
    };
    const std::array<StatusMapping, 6> mappings {{
      {PSA_SUCCESS, SL_STATUS_OK},
      {PSA_ERROR_INVALID_SIGNATURE, SL_STATUS_INVALID_SIGNATURE},
      {PSA_ERROR_INVALID_ARGUMENT, SL_STATUS_INVALID_PARAMETER},
      {PSA_ERROR_NOT_SUPPORTED, SL_STATUS_NOT_SUPPORTED},
      {PSA_ERROR_INSUFFICIENT_MEMORY, SL_STATUS_ALLOCATION_FAILED},
      {PSA_ERROR_BAD_STATE, SL_STATUS_INVALID_STATE},
    }};

    for (const auto &mapping: mappings) {
        SCOPED_TRACE(mapping.psa_status);
        EXPECT_EQ(mapping.sl_status, crypto::psa_status_to_sl_status(mapping.psa_status));
    }
}

TEST(PsaStatus, FallsBackForUnmappedAndUnknownStatuses)
{
    EXPECT_EQ(SL_STATUS_FAIL, crypto::psa_status_to_sl_status(PSA_ERROR_GENERIC_ERROR));
    EXPECT_EQ(SL_STATUS_FAIL, crypto::psa_status_to_sl_status(PSA_ERROR_INSUFFICIENT_ENTROPY));
    EXPECT_EQ(SL_STATUS_FAIL, crypto::psa_status_to_sl_status(static_cast<psa_status_t>(-12345)));
}
