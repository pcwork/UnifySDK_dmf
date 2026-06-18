#include "zigpc_gateway_notify.h"
#include "zigpc_onboarding.h"

#include <string.h>

#include "unity.h"

#include "zigpc_datastore_mock.h"
#include "zigpc_discovery_mock.h"
#include "zigpc_gateway_mock.h"

typedef void (*observer_callback_t)(void *event_data);

static observer_callback_t network_init_cb = NULL;
static observer_callback_t node_add_cb     = NULL;
static observer_callback_t node_remove_cb  = NULL;

static sl_status_t capture_registered_observers(
  enum zigpc_gateway_notify_event event, zigpc_observer_function_t callback, int num_calls)
{
  (void)num_calls;
  if (event == ZIGPC_GATEWAY_NOTIFY_NETWORK_INIT) {
    network_init_cb = callback;
  } else if (event == ZIGPC_GATEWAY_NOTIFY_NODE_ADD_COMPLETE) {
    node_add_cb = callback;
  } else if (event == ZIGPC_GATEWAY_NOTIFY_NODE_REMOVED) {
    node_remove_cb = callback;
  }
  return SL_STATUS_OK;
}

void suiteSetUp(void) {}

int suiteTearDown(int num_failures)
{
  return num_failures;
}

void setUp(void)
{
  network_init_cb = NULL;
  node_add_cb     = NULL;
  node_remove_cb  = NULL;
}

void tearDown(void) {}

void test_setup_registers_required_gateway_observers(void)
{
  zigpc_gateway_register_observer_Stub(capture_registered_observers);

  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_onboarding_fixt_setup());
  TEST_ASSERT_NOT_NULL(network_init_cb);
  TEST_ASSERT_NOT_NULL(node_add_cb);
  TEST_ASSERT_NOT_NULL(node_remove_cb);
}

void test_node_add_complete_creates_device_and_starts_interview(void)
{
  zigbee_eui64_t eui64 = {0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE};
  zigpc_gateway_on_node_add_t event_data = {0};
  zigpc_device_data_t device_data        = {
    .network_status            = ZIGBEE_NODE_STATUS_NODEID_ASSIGNED,
    .max_cmd_delay             = 1,
    .endpoint_total_count      = 0,
    .endpoint_discovered_count = 0,
  };

  memcpy(event_data.eui64, eui64, sizeof(zigbee_eui64_t));

  zigpc_gateway_register_observer_Stub(capture_registered_observers);
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_onboarding_fixt_setup());

  zigpc_datastore_remove_device_ExpectWithArrayAndReturn(
    eui64,
    sizeof(zigbee_eui64_t),
    SL_STATUS_OK);
  zigpc_datastore_create_device_ExpectWithArrayAndReturn(
    eui64,
    sizeof(zigbee_eui64_t),
    SL_STATUS_OK);
  zigpc_datastore_write_device_ExpectWithArrayAndReturn(
    eui64,
    sizeof(zigbee_eui64_t),
    &device_data,
    1,
    SL_STATUS_OK);
  zigpc_discovery_interview_device_ExpectAndReturn(zigbee_eui64_to_uint(eui64),
                                                   NULL,
                                                   SL_STATUS_OK);
  zigpc_discovery_interview_device_IgnoreArg_callback();

  TEST_ASSERT_NOT_NULL(node_add_cb);
  node_add_cb(&event_data);
}

void test_network_init_persists_gateway_network_and_endpoint(void)
{
  zigbee_eui64_t gateway_eui64 = {0xAA, 0xBB, 0xCC, 0xDD, 0x10, 0x20, 0x30, 0x40};
  zigbee_ext_panid_t ext_panid = {0x11, 0x22, 0x33, 0x44,
                                  0x55, 0x66, 0x77, 0x88};
  zigpc_gateway_on_network_init_t event_data = {
    .zigpc_endpoint_id   = 1,
    .zigpc_panid         = 0x1234,
    .zigpc_radio_channel = 20,
  };
  zigpc_network_data_t network_data = {
    .panid         = 0x1234,
    .radio_channel = 20,
  };
  zigpc_device_data_t gateway_data = {
    .network_status            = ZIGBEE_NODE_STATUS_INCLUDED,
    .max_cmd_delay             = 0,
    .endpoint_total_count      = 1,
    .endpoint_discovered_count = 1,
  };

  memcpy(event_data.zigpc_eui64, gateway_eui64, sizeof(zigbee_eui64_t));
  memcpy(event_data.zigpc_ext_panid, ext_panid, sizeof(zigbee_ext_panid_t));
  memcpy(network_data.gateway_eui64, gateway_eui64, sizeof(zigbee_eui64_t));
  memcpy(network_data.ext_panid, ext_panid, sizeof(zigbee_ext_panid_t));

  zigpc_gateway_register_observer_Stub(capture_registered_observers);
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_onboarding_fixt_setup());

  zigpc_datastore_create_network_ExpectAndReturn(SL_STATUS_OK);
  zigpc_datastore_write_network_ExpectAndReturn(&network_data, SL_STATUS_OK);
  zigpc_datastore_remove_device_ExpectWithArrayAndReturn(
    gateway_eui64,
    sizeof(zigbee_eui64_t),
    SL_STATUS_OK);
  zigpc_datastore_create_device_ExpectWithArrayAndReturn(
    gateway_eui64,
    sizeof(zigbee_eui64_t),
    SL_STATUS_OK);
  zigpc_datastore_write_device_ExpectWithArrayAndReturn(
    gateway_eui64,
    sizeof(zigbee_eui64_t),
    &gateway_data,
    1,
    SL_STATUS_OK);
  zigpc_datastore_create_endpoint_ExpectWithArrayAndReturn(
    gateway_eui64,
    sizeof(zigbee_eui64_t),
    1,
    SL_STATUS_OK);

  TEST_ASSERT_NOT_NULL(network_init_cb);
  network_init_cb(&event_data);
}

void test_node_removed_deletes_device_from_datastore(void)
{
  zigbee_eui64_t eui64 = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  zigpc_gateway_on_node_removed_t event_data = {0};

  memcpy(event_data.eui64, eui64, sizeof(zigbee_eui64_t));

  zigpc_gateway_register_observer_Stub(capture_registered_observers);
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_onboarding_fixt_setup());

  zigpc_datastore_remove_device_ExpectWithArrayAndReturn(
    eui64,
    sizeof(zigbee_eui64_t),
    SL_STATUS_OK);

  TEST_ASSERT_NOT_NULL(node_remove_cb);
  node_remove_cb(&event_data);
}
