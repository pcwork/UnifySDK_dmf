/******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include <string.h>

#include "unity.h"

#include "zigbee_host.h"
#include "zigbee_host_common.h"

static int captured_argc;
static char *captured_argv[8];
static const uint16_t dummy_cluster_list[] = { 0 };

void setUp(void)
{
  captured_argc = 0;
  memset(captured_argv, 0, sizeof(captured_argv));
}

void tearDown(void) {}

bool sl_zigbee_ezsp_process_command_options(int argc, char *argv[])
{
  captured_argc = argc;
  for (int i = 0; i < argc; i++) {
    captured_argv[i] = argv[i];
  }
  return true;
}

void sl_system_init(void) {}

void sl_system_process_action(void) {}

void sli_legacy_serial_cleanup(void) {}

void sl_zigbee_sec_man_init_context(sl_zigbee_sec_man_context_t *context)
{
  memset(context, 0, sizeof(*context));
}

sl_status_t sl_zigbee_sec_man_export_key(sl_zigbee_sec_man_context_t *context,
                                         sl_zigbee_sec_man_key_t *key)
{
  (void)context;
  (void)key;
  return SL_STATUS_OK;
}

sl_status_t sl_zigbee_af_set_ezsp_policy(sl_zigbee_ezsp_policy_id_t policyId,
                                         sl_zigbee_ezsp_decision_id_t decisionId,
                                         const char *policyName,
                                         const char *decisionName)
{
  (void)policyId;
  (void)decisionId;
  (void)policyName;
  (void)decisionName;
  return SL_STATUS_OK;
}

sl_status_t sl_zigbee_subscribe_to_zcl_commands(uint16_t cluster_id,
                                                uint16_t manufacturer_id,
                                                uint8_t direction,
                                                sl_service_function_t service_function)
{
  (void)cluster_id;
  (void)manufacturer_id;
  (void)direction;
  (void)service_function;
  return SL_STATUS_OK;
}

void sl_zigbee_af_core_flush(void) {}

sl_status_t sl_zigbee_ezsp_launch_standalone_bootloader(bool enabled)
{
  (void)enabled;
  return SL_STATUS_OK;
}

void sl_zigbee_ezsp_close(void) {}

sl_zigbee_af_status_t emberAfClusterServiceCallback(sl_service_opcode_t opcode,
                                                    sl_service_function_context_t *context)
{
  (void)opcode;
  (void)context;
  return SL_ZIGBEE_ZCL_STATUS_SUCCESS;
}

static struct zigbeeHostCallbacks callbacks = {0};

void test_zigbeeHostInit_uses_hardware_flow_control_flag(void)
{
  struct zigbeeHostOpts opts = {
    .serialPort = "/dev/ttyUSB0",
    .otaPath = "/tmp/ota",
    .flowControl = ZIGBEE_HOST_FC_HARDWARE,
    .callbacks = &callbacks,
    .supportedClusterList = dummy_cluster_list,
    .supportedClusterListSize = 0,
  };

  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigbeeHostInit(&opts));
  TEST_ASSERT_EQUAL(7, captured_argc);
  TEST_ASSERT_EQUAL_STRING("zigbeeHost", captured_argv[0]);
  TEST_ASSERT_EQUAL_STRING("-p", captured_argv[1]);
  TEST_ASSERT_EQUAL_STRING("/dev/ttyUSB0", captured_argv[2]);
  TEST_ASSERT_EQUAL_STRING("-d", captured_argv[3]);
  TEST_ASSERT_EQUAL_STRING("/tmp/ota", captured_argv[4]);
  TEST_ASSERT_EQUAL_STRING("-f", captured_argv[5]);
  TEST_ASSERT_EQUAL_STRING("r", captured_argv[6]);
}

void test_zigbeeHostInit_uses_software_flow_control_flag(void)
{
  struct zigbeeHostOpts opts = {
    .serialPort = "/dev/ttyUSB0",
    .otaPath = "/tmp/ota",
    .flowControl = ZIGBEE_HOST_FC_SOFTWARE,
    .callbacks = &callbacks,
    .supportedClusterList = dummy_cluster_list,
    .supportedClusterListSize = 0,
  };

  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigbeeHostInit(&opts));
  TEST_ASSERT_EQUAL(7, captured_argc);
  TEST_ASSERT_EQUAL_STRING("zigbeeHost", captured_argv[0]);
  TEST_ASSERT_EQUAL_STRING("-p", captured_argv[1]);
  TEST_ASSERT_EQUAL_STRING("/dev/ttyUSB0", captured_argv[2]);
  TEST_ASSERT_EQUAL_STRING("-d", captured_argv[3]);
  TEST_ASSERT_EQUAL_STRING("/tmp/ota", captured_argv[4]);
  TEST_ASSERT_EQUAL_STRING("-f", captured_argv[5]);
  TEST_ASSERT_EQUAL_STRING("x", captured_argv[6]);
}
