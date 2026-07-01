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

#include <unity.h>

#include "zigbee_host_mock.h"
#include "zigpc_config_mock.h"
#include "zcl_util_mock.h"

#include "zigpc_gateway_process.h"

static const zigpc_config_t hardware_flow_control_config = {
  .serial_port  = "/dev/ttyUSB0",
  .flow_control = ZIGPC_FC_HARDWARE,
  .ota_path     = "/tmp/ota",
};

static const zigpc_config_t software_flow_control_config = {
  .serial_port  = "/dev/ttyUSB0",
  .flow_control = ZIGPC_FC_SOFTWARE,
  .ota_path     = "/tmp/ota",
};

static const zigpc_config_t invalid_flow_control_config = {
  .serial_port  = "/dev/ttyUSB0",
  .flow_control = (zigpc_flow_control_t)99,
  .ota_path     = "/tmp/ota",
};

static const uint16_t test_clusters[] = { 0x0006 };
static bool zigbee_host_init_called = false;
static zigbeeHostFlowControl_t expected_flow_control = ZIGBEE_HOST_FC_HARDWARE;

void suiteSetUp(void) {}

int suiteTearDown(int num_failures)
{
  return num_failures;
}

void setUp(void)
{
  zigbee_host_mock_Init();
  zigpc_config_mock_Init();
  zcl_util_mock_Init();
  zigbee_host_init_called = false;
  expected_flow_control = ZIGBEE_HOST_FC_HARDWARE;
}

void tearDown(void)
{
  zcl_util_mock_Verify();
  zcl_util_mock_Destroy();
  zigpc_config_mock_Verify();
  zigpc_config_mock_Destroy();
  zigbee_host_mock_Verify();
  zigbee_host_mock_Destroy();
}

static int zigbeeHostInit_stub(struct zigbeeHostOpts *opts, int cmock_num_calls)
{
  (void)cmock_num_calls;

  zigbee_host_init_called = true;
  TEST_ASSERT_NOT_NULL(opts);
  TEST_ASSERT_EQUAL(expected_flow_control, opts->flowControl);

  return SL_STATUS_FAIL;
}

void test_zigpc_gateway_process_setup_passes_hardware_flow_control_to_host(void)
{
  expected_flow_control = ZIGBEE_HOST_FC_HARDWARE;
  zigpc_get_config_ExpectAndReturn(&hardware_flow_control_config);
  zigpc_zcl_get_number_supported_clusters_ExpectAndReturn(1);
  zigpc_zcl_get_supported_cluster_list_ExpectAndReturn(test_clusters);
  zigbeeHostInit_StubWithCallback(zigbeeHostInit_stub);

  TEST_ASSERT_EQUAL(SL_STATUS_FAIL, zigpc_gateway_process_setup());
  TEST_ASSERT_TRUE(zigbee_host_init_called);
}

void test_zigpc_gateway_process_setup_passes_software_flow_control_to_host(void)
{
  expected_flow_control = ZIGBEE_HOST_FC_SOFTWARE;
  zigpc_get_config_ExpectAndReturn(&software_flow_control_config);
  zigpc_zcl_get_number_supported_clusters_ExpectAndReturn(1);
  zigpc_zcl_get_supported_cluster_list_ExpectAndReturn(test_clusters);
  zigbeeHostInit_StubWithCallback(zigbeeHostInit_stub);

  TEST_ASSERT_EQUAL(SL_STATUS_FAIL, zigpc_gateway_process_setup());
  TEST_ASSERT_TRUE(zigbee_host_init_called);
}

void test_zigpc_gateway_process_setup_rejects_invalid_flow_control(void)
{
  zigpc_get_config_ExpectAndReturn(&invalid_flow_control_config);

  TEST_ASSERT_EQUAL(SL_STATUS_FAIL, zigpc_gateway_process_setup());
  TEST_ASSERT_FALSE(zigbee_host_init_called);
}
