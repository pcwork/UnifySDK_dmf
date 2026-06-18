/*******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "zigpc_config.h"
#include "zigpc_config_fixt.h"
#include "zigpc_cluster_config.h"

#include "config.h"
#include "unity.h"

#include <stdio.h>
#include <unistd.h>

#define TEST_CONFIG_FILE "_zigpc_test_config.ini"

static void remove_test_config_file()
{
  if (access(TEST_CONFIG_FILE, F_OK) != -1) {
    remove(TEST_CONFIG_FILE);
  }
}

void setUp()
{
  remove_test_config_file();
}

void tearDown()
{
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

  return result > 0;
}

void test_zigpc_config_reads_runtime_settings()
{
  char *argv_inject[3] = {"test_config", "--conf", TEST_CONFIG_FILE};
  const char *ini_content
    = "zigpc:\n"
      "    serial: /dev/ttyACM0\n"
      "    datastore_file: custom-zigpc.db\n"
      "    supported_clusters: OnOff,Level\n"
    "    ota_path: /tmp/ota\n"
    "    tc_use_well_known_key: true\n"
    "mqtt:\n"
    "    host: localhost\n"
      "    port: 1884\n";

  TEST_ASSERT_TRUE(create_file_with_content(TEST_CONFIG_FILE, ini_content));
  TEST_ASSERT_EQUAL(0, zigpc_config_init());

  config_parse(sizeof(argv_inject) / sizeof(char *), argv_inject, "test");
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_config_fixt_setup());

  TEST_ASSERT_EQUAL_STRING("/dev/ttyACM0", zigpc_get_config()->serial_port);
  TEST_ASSERT_EQUAL_STRING("custom-zigpc.db",
                           zigpc_get_config()->datastore_file);
  TEST_ASSERT_EQUAL_STRING("localhost", zigpc_get_config()->mqtt_host);
  TEST_ASSERT_EQUAL(1884, zigpc_get_config()->mqtt_port);
  TEST_ASSERT_EQUAL_STRING("OnOff,Level",
                           zigpc_get_config()->supported_clusters);
  TEST_ASSERT_EQUAL_STRING("/tmp/ota", zigpc_get_config()->ota_path);
  TEST_ASSERT_TRUE(zigpc_get_config()->tc_use_well_known_key);
}

void test_zigpc_cluster_config_defaults_to_small_cluster_set()
{
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_cluster_configure(""));

  TEST_ASSERT_TRUE(zigpc_cluster_is_supported("Basic"));
  TEST_ASSERT_TRUE(zigpc_cluster_is_supported("OnOff"));
  TEST_ASSERT_FALSE(zigpc_cluster_is_supported("Level"));
  TEST_ASSERT_FALSE(zigpc_cluster_is_supported("Thermostat"));
  TEST_ASSERT_EQUAL(2, zigpc_cluster_count());
  TEST_ASSERT_EQUAL_HEX16(0x0000, zigpc_cluster_get_active_ids()[0]);
  TEST_ASSERT_EQUAL_HEX16(0x0006, zigpc_cluster_get_active_ids()[1]);
}

void test_zigpc_cluster_config_accepts_explicit_cluster_list()
{
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_cluster_configure("OnOff,Level"));

  TEST_ASSERT_TRUE(zigpc_cluster_is_supported("OnOff"));
  TEST_ASSERT_TRUE(zigpc_cluster_is_supported("Level"));
  TEST_ASSERT_FALSE(zigpc_cluster_is_supported("Basic"));
  TEST_ASSERT_EQUAL(2, zigpc_cluster_count());
  TEST_ASSERT_EQUAL_HEX16(0x0006, zigpc_cluster_get_active_ids()[0]);
  TEST_ASSERT_EQUAL_HEX16(0x0008, zigpc_cluster_get_active_ids()[1]);
}

void test_zigpc_cluster_config_rejects_unknown_cluster()
{
  TEST_ASSERT_EQUAL(SL_STATUS_FAIL,
                    zigpc_cluster_configure("OnOff,Thermostat"));

  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_cluster_configure("OnOff"));
  TEST_ASSERT_TRUE(zigpc_cluster_is_supported("OnOff"));
  TEST_ASSERT_FALSE(zigpc_cluster_is_supported("Thermostat"));
}
