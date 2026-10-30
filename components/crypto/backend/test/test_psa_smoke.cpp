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

#include <cstdint>

#include <gtest/gtest.h>
#include "psa/crypto.h"

#include "test_fixture.hpp"

// PSA random generation succeeds after crypto init.
TEST_F(CryptoBackend, PsaGenerateRandom)
{
    uint8_t buf[16]     = {0};
    psa_status_t status = psa_wrappers.generate_random(buf);
    EXPECT_EQ(PSA_SUCCESS, status);
}
