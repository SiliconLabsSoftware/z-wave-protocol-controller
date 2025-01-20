/*******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#include "config.h"
#include "zigpc_config.h"
#include "zigpc_config_fixt.h"
#include "unity.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define TEST_CONFIG_FILE "_test_config.ini"

static void remove_test_config_file()
{
  if (access(TEST_CONFIG_FILE, F_OK) != -1) {
    remove(TEST_CONFIG_FILE);
  }
}

void setUp()
{
  // Ensure test config file doesn't exist
  remove_test_config_file();
}

void tearDown()
{
  // Remove test config file on teardown
  remove_test_config_file();
}

static bool create_file_with_content(const char *filename, const char *content)
{
  FILE *fpth = fopen(filename, "w");
  if (fpth == NULL) {
    return false;
  }
  int result = fputs(content, fpth);
  fclose(fpth);

  return (result > 0);
}

void test_init()
{
  char *argv_inject[3]    = {"test_config", "--conf", TEST_CONFIG_FILE};
  const char *ini_content = "zigpc:\n"
                            "    - serial: /dev/ttyUSB0\n"
                            "    - tc_use_well_known_key: true\n"
                            "    - datastore_file: /tmp/database_file.db\n"
                            "    - ota_path: /test-ota-mock/\n"
                            "    - attr_polling_rate_ms: 500\n"
                            "mqtt:\n"
                            "    - host: localhost\n"
                            "    - port: 2000\n";
  TEST_ASSERT_TRUE_MESSAGE(
    create_file_with_content(TEST_CONFIG_FILE, ini_content),
    "Failed to create config file");
  TEST_ASSERT_EQUAL_MESSAGE(0, zigpc_config_init(), "zigpc_config_init failed");
  config_parse(sizeof(argv_inject) / sizeof(char *),
               argv_inject,
               "test version");
  zigpc_config_fixt_setup();
  TEST_ASSERT_EQUAL_STRING("localhost", zigpc_get_config()->mqtt_host);
  TEST_ASSERT_EQUAL(2000, zigpc_get_config()->mqtt_port);
  TEST_ASSERT_EQUAL_STRING("/dev/ttyUSB0", zigpc_get_config()->serial_port);
  TEST_ASSERT_EQUAL_STRING("/tmp/database_file.db",
                           zigpc_get_config()->datastore_file);
  TEST_ASSERT_EQUAL_STRING("/test-ota-mock/",
                           zigpc_get_config()->ota_path);
  TEST_ASSERT_EQUAL_INT(500,zigpc_get_config()->attr_polling_rate_ms);
  TEST_ASSERT_TRUE(zigpc_get_config()->tc_use_well_known_key);
}
