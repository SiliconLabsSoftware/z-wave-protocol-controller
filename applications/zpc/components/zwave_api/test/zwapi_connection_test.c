/******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
#include "unity.h"
#include <stdio.h>

#include <unity.h>
#include "zwapi_connection.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Woverflow"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Woverflow"
#endif

/// Setup the test suite (called once before all test_xxx functions are called)
void suiteSetUp() {}

/// Teardown the test suite (called once after all test_xxx functions are called)
int suiteTearDown(int num_failures)
{
  return num_failures;
}

/// Called before each and every test
void setUp() {}

void test_zwapi_connection_tx_invalid_payload_full()
{
  uint8_t cmd          = 0x01;
  uint8_t type         = 0x02;
  uint8_t buffer[0xFF] = {0};  ///< Maximum of len type
  uint8_t len          = sizeof(buffer);
  bool ack_needed      = true;

  // Expect the function to detect overflow
  zwapi_connection_tx(cmd, type, buffer, len, ack_needed);
}

void test_zwapi_connection_tx_valid_inputs()
{
  uint8_t cmd  = 0x01;
  uint8_t type = 0x02;
  uint8_t buffer[10]
    = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0};
  uint8_t len     = sizeof(buffer);
  bool ack_needed = true;

  // Expect the function to execute without errors
  zwapi_connection_tx(cmd, type, buffer, len, ack_needed);
}

void test_zwapi_connection_tx_null_buffer()
{
  uint8_t cmd     = 0x01;
  uint8_t type    = 0x02;
  uint8_t *buffer = NULL;
  uint8_t len     = 0;
  bool ack_needed = true;

  // Expect the function to not crash
  zwapi_connection_tx(cmd, type, buffer, len, ack_needed);
}
