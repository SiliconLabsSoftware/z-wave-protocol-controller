/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include <time.h>
#include <sys/time.h>
#include <string.h>

// Includes from this component
#include "zwave_command_class_time_parameters.h"
#include "zwave_raw_frame_test_helper.h"
#include "platform_date_time.h"

// Includes from other components
#include "zwave_tx_mock.h"
#include "zwave_command_handler_mock.h"
#include "zwave_controller_connection_info.h"

#include "ZW_classcmd.h"

// Test includes
#include "unity.h"

// Date time data that will be returned by the mocked functions
#define TEST_YEAR  1989
#define TEST_MONTH 4
#define TEST_DAY   27
#define TEST_HOUR  10
#define TEST_MIN   20
#define TEST_SEC   30

/* Time tm information struct with a dummy date and time for this test. */
struct tm test_time_info = {.tm_year  = TEST_YEAR - 1900,
                            .tm_mon   = TEST_MONTH - 1,
                            .tm_mday  = TEST_DAY,
                            .tm_hour  = TEST_HOUR,
                            .tm_min   = TEST_MIN,
                            .tm_sec   = TEST_SEC,
                            .tm_isdst = -1};

/// Setup the test suite (called once before all test_xxx functions are called)
void suiteSetUp() {}

/// Teardown the test suite (called once after all test_xxx functions are called)
int suiteTearDown(int num_failures)
{
  return num_failures;
}

time_t time(time_t *t)
{
  return 0;
}

struct tm *localtime_r(const time_t *timep, struct tm *result)
{
  memcpy(result, &test_time_info, sizeof(test_time_info));
  return result;
}

void test_zwave_command_class_time_parameters_not_supported_result()
{
  execute_frame_expect_result(
    zwave_command_class_time_parameters_support_handler,
    NULL,
    0,
    SL_STATUS_NOT_SUPPORTED);
  execute_frame_expect_result(
    zwave_command_class_time_parameters_support_handler,
    NULL,
    1,
    SL_STATUS_NOT_SUPPORTED);
  uint8_t cmd_frame_without_command[] = {
    COMMAND_CLASS_TIME_PARAMETERS,
  };
  execute_frame_expect_result(
    zwave_command_class_time_parameters_support_handler,
    cmd_frame_without_command,
    sizeof(cmd_frame_without_command),
    SL_STATUS_NOT_SUPPORTED);
}

void test_zwave_command_class_time_parameters_ignore_non_relevant_cc()
{
  uint8_t cmd_frame_unknown[] = {
    0xFF,
    0xFF  // arbitrary Command
  };

  execute_frame_expect_result(
    zwave_command_class_time_parameters_support_handler,
    cmd_frame_unknown,
    sizeof(cmd_frame_unknown),
    SL_STATUS_NOT_SUPPORTED);
}

void test_zwave_command_class_time_parameters_ignore_non_relevant_cmd()
{
  uint8_t cmd_frame_unknown[] = {
    COMMAND_CLASS_TIME_PARAMETERS,
    0xFF  // arbitrary Command
  };

  execute_frame_expect_result(
    zwave_command_class_time_parameters_support_handler,
    cmd_frame_unknown,
    sizeof(cmd_frame_unknown),
    SL_STATUS_NOT_SUPPORTED);
}

void test_zwave_command_class_time_parameters_multicast_not_allowed()
{
  uint8_t cmd_frame_time_parameters_command[]
    = {COMMAND_CLASS_TIME_PARAMETERS, TIME_PARAMETERS_GET};
  assert_dissallow_multicast(
    zwave_command_class_time_parameters_support_handler,
    cmd_frame_time_parameters_command);
}

void test_zwave_command_class_time_parameters_get()
{
    // Prepare the expected date_time_t structure
    date_time_t expected_time = {
        .year = TEST_YEAR - 1900, // Adjust for tm_year
        .mon  = TEST_MONTH - 1,    // Adjust for tm_mon
        .day  = TEST_DAY,
        .hour = TEST_HOUR & 0xF,
        .min  = TEST_MIN,
        .sec  = TEST_SEC
    };

    // Simulate the set time in the system
    platform_set_date_time(&expected_time);

    // Prepare command frame for TIME_PARAMETERS_GET command
    const uint8_t cmd_frame_time_parameters_command[] = {
        COMMAND_CLASS_TIME_PARAMETERS,
        TIME_PARAMETERS_GET
    };

    // Prepare expected response frame
    const uint8_t exp_frame_time_parameters_command[] = {
        COMMAND_CLASS_TIME_PARAMETERS,
        TIME_PARAMETERS_REPORT,
        (TEST_YEAR >> 8),      // MSB of the year
        (TEST_YEAR & 0xFF),    // LSB of the year
        TEST_MONTH,            // Month (1-12)
        TEST_DAY,              // Day
        TEST_HOUR & 0xF,       // Hour
        TEST_MIN,              // Minute
        TEST_SEC               // Second
    };

    // Execute the test
    execute_frame_expect_frame(
        zwave_command_class_time_parameters_support_handler,
        cmd_frame_time_parameters_command,
        sizeof(cmd_frame_time_parameters_command),
        exp_frame_time_parameters_command,
        sizeof(exp_frame_time_parameters_command),
        SL_STATUS_OK
    );
}

void test_zwave_command_class_time_parameters_set(void) {
    // Define test frame data for a known date and time
    uint8_t valid_frame_data[] = {0x00, 0x00, 0x07, 0xE5, 0x04, 0x15, 0x10, 0x20, 0x30}; // 2021-04-21 16:32:48
    uint16_t valid_frame_length = sizeof(valid_frame_data);

    // Call the function to set time parameters
    sl_status_t result = zwave_command_class_time_parameters_set(valid_frame_data, valid_frame_length);
    TEST_ASSERT_EQUAL(SL_STATUS_OK, result);

    // Validate that the simulated time has been updated correctly
    date_time_t simulated_time = platform_get_date_time();

    // Adjust expected values for tm_year and tm_mon
    TEST_ASSERT_EQUAL(121 , simulated_time.year); // Adjusting years(2021 -1900)
    TEST_ASSERT_EQUAL(3, simulated_time.mon);     // Adjusting months(4-1)
    TEST_ASSERT_EQUAL(21, simulated_time.day);
    TEST_ASSERT_EQUAL(16, simulated_time.hour);
    TEST_ASSERT_EQUAL(32, simulated_time.min);
    TEST_ASSERT_EQUAL(48, simulated_time.sec);
}

static sl_status_t zwave_command_handler_register_handler_CALLBACK(
  zwave_command_handler_t actual, int cmock_num_calls)
{
  zwave_command_handler_t expected = {0};
  expected.minimal_scheme          = ZWAVE_CONTROLLER_ENCAPSULATION_NONE;
  expected.command_class           = COMMAND_CLASS_TIME_PARAMETERS;
  expected.command_class_name      = "Time Parameters";
  expected.version                 = TIME_PARAMETERS_VERSION;
  expected.support_handler
    = &zwave_command_class_time_parameters_support_handler;
  expected.control_handler = NULL;

  TEST_ASSERT_EQUAL_PTR(expected.support_handler, actual.support_handler);
  TEST_ASSERT_EQUAL_PTR(expected.control_handler, actual.control_handler);
  TEST_ASSERT_EQUAL(expected.minimal_scheme, actual.minimal_scheme);
  TEST_ASSERT_EQUAL(expected.command_class, actual.command_class);
  TEST_ASSERT_EQUAL_STRING(expected.command_class_name,
                           actual.command_class_name);
  TEST_ASSERT_EQUAL(expected.version, actual.version);
  TEST_ASSERT_FALSE(actual.manual_security_validation);
  return SL_STATUS_OK;
}

void test_zwave_command_class_time_parameters_init_registers_handler()
{
  zwave_command_handler_register_handler_AddCallback(
    zwave_command_handler_register_handler_CALLBACK);
  zwave_command_handler_register_handler_IgnoreAndReturn(SL_STATUS_OK);
  zwave_command_class_time_parameters_init();
}
