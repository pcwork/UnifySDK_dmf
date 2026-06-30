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

static const zigpc_config_t test_config = {
  .serial_port  = "/dev/ttyUSB0",
  .flow_control = ZIGPC_FC_SOFTWARE,
  .ota_path     = "/tmp/ota",
};

static const uint16_t test_clusters[] = { 0x0006 };

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

  TEST_ASSERT_NOT_NULL(opts);
  TEST_ASSERT_EQUAL(ZIGBEE_HOST_FC_HARDWARE, opts->flowControl);

  return SL_STATUS_FAIL;
}

void test_zigpc_gateway_process_setup_defaults_host_flow_control_to_hardware(void)
{
  zigpc_get_config_ExpectAndReturn(&test_config);
  zigpc_zcl_get_number_supported_clusters_ExpectAndReturn(1);
  zigpc_zcl_get_supported_cluster_list_ExpectAndReturn(test_clusters);
  zigbeeHostInit_StubWithCallback(zigbeeHostInit_stub);

  TEST_ASSERT_EQUAL(SL_STATUS_FAIL, zigpc_gateway_process_setup());
}
