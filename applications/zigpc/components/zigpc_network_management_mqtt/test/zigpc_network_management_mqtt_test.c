#include <string.h>

#include "unity.h"

#include "uic_mqtt.h"
#include "zigpc_gateway_notify.h"
#include "zigpc_network_management_mqtt.h"

#include "uic_mqtt_mock.h"
#include "zigpc_gateway_mock.h"
#include "zigpc_gateway_notify_mock.h"

typedef void (*observer_callback_t)(void *event_data);

static observer_callback_t network_init_cb          = NULL;
static mqtt_message_callback_t network_mgmt_mqtt_cb = NULL;
static char subscribed_topic[MQTT_TOPIC_MAX_LENGTH] = {0};

static sl_status_t capture_registered_observers(
  enum zigpc_gateway_notify_event event,
  zigpc_observer_callback_t callback,
  int num_calls)
{
  (void)num_calls;
  if (event == ZIGPC_GATEWAY_NOTIFY_NETWORK_INIT) {
    network_init_cb = callback;
  }
  return SL_STATUS_OK;
}

static void capture_mqtt_subscription(const char *topic,
                                      mqtt_message_callback_t callback,
                                      int num_calls)
{
  (void)num_calls;
  strncpy(subscribed_topic, topic, sizeof(subscribed_topic) - 1);
  subscribed_topic[sizeof(subscribed_topic) - 1] = '\0';
  network_mgmt_mqtt_cb                           = callback;
}

void setUp(void)
{
  network_init_cb       = NULL;
  network_mgmt_mqtt_cb  = NULL;
  subscribed_topic[0]   = '\0';
}

void tearDown(void) {}

void test_setup_registers_network_init_observer(void)
{
  zigpc_gateway_register_observer_AddCallback(capture_registered_observers);

  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_network_management_mqtt_fixt_setup());
  TEST_ASSERT_NOT_NULL(network_init_cb);
}

void test_network_init_subscribes_to_protocol_controller_write_topic(void)
{
  zigpc_gateway_on_network_init_t event_data = {0};

  memcpy(event_data.zigpc_eui64,
         (zigbee_eui64_t){0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x00, 0x11, 0x22},
         sizeof(zigbee_eui64_t));

  zigpc_gateway_register_observer_AddCallback(capture_registered_observers);
  uic_mqtt_subscribe_AddCallback(capture_mqtt_subscription);

  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_network_management_mqtt_fixt_setup());
  TEST_ASSERT_NOT_NULL(network_init_cb);

  network_init_cb(&event_data);

  TEST_ASSERT_EQUAL_STRING(
    "ucl/by-unid/zb-AABBCCDDEE001122/ProtocolController/NetworkManagement/Write",
    subscribed_topic);
  TEST_ASSERT_NOT_NULL(network_mgmt_mqtt_cb);
}

void test_add_node_write_opens_network_for_joins(void)
{
  zigpc_gateway_on_network_init_t event_data = {0};
  const char *payload                        = "{\"State\":\"add node\"}";

  memcpy(event_data.zigpc_eui64,
         (zigbee_eui64_t){0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x00, 0x11, 0x22},
         sizeof(zigbee_eui64_t));

  zigpc_gateway_register_observer_AddCallback(capture_registered_observers);
  uic_mqtt_subscribe_AddCallback(capture_mqtt_subscription);
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_network_management_mqtt_fixt_setup());

  network_init_cb(&event_data);

  zigpc_gateway_network_permit_joins_ExpectAndReturn(true, SL_STATUS_OK);
  TEST_ASSERT_NOT_NULL(network_mgmt_mqtt_cb);
  network_mgmt_mqtt_cb(subscribed_topic, payload, strlen(payload));
}

void test_idle_write_closes_network_for_joins(void)
{
  zigpc_gateway_on_network_init_t event_data = {0};
  const char *payload                        = "{\"State\":\"idle\"}";

  memcpy(event_data.zigpc_eui64,
         (zigbee_eui64_t){0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x00, 0x11, 0x22},
         sizeof(zigbee_eui64_t));

  zigpc_gateway_register_observer_AddCallback(capture_registered_observers);
  uic_mqtt_subscribe_AddCallback(capture_mqtt_subscription);
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_network_management_mqtt_fixt_setup());

  network_init_cb(&event_data);

  zigpc_gateway_network_permit_joins_ExpectAndReturn(false, SL_STATUS_OK);
  TEST_ASSERT_NOT_NULL(network_mgmt_mqtt_cb);
  network_mgmt_mqtt_cb(subscribed_topic, payload, strlen(payload));
}

void test_remove_node_write_removes_requested_unid(void)
{
  zigpc_gateway_on_network_init_t event_data = {0};
  const char *payload
    = "{\"State\":\"remove node\",\"StateParameters\":{\"Unid\":\"zb-0102030405060708\"}}";
  zigbee_eui64_t eui64 = {0x01, 0x02, 0x03, 0x04,
                          0x05, 0x06, 0x07, 0x08};

  memcpy(event_data.zigpc_eui64,
         (zigbee_eui64_t){0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x00, 0x11, 0x22},
         sizeof(zigbee_eui64_t));

  zigpc_gateway_register_observer_AddCallback(capture_registered_observers);
  uic_mqtt_subscribe_AddCallback(capture_mqtt_subscription);
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_network_management_mqtt_fixt_setup());

  network_init_cb(&event_data);

  zigpc_gateway_remove_node_ExpectWithArrayAndReturn(eui64,
                                                     sizeof(zigbee_eui64_t),
                                                     SL_STATUS_OK);
  TEST_ASSERT_NOT_NULL(network_mgmt_mqtt_cb);
  network_mgmt_mqtt_cb(subscribed_topic, payload, strlen(payload));
}

void test_shutdown_unsubscribes_from_protocol_controller_write_topic(void)
{
  zigpc_gateway_on_network_init_t event_data = {0};

  memcpy(event_data.zigpc_eui64,
         (zigbee_eui64_t){0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x00, 0x11, 0x22},
         sizeof(zigbee_eui64_t));

  zigpc_gateway_register_observer_AddCallback(capture_registered_observers);
  uic_mqtt_subscribe_AddCallback(capture_mqtt_subscription);
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_network_management_mqtt_fixt_setup());

  network_init_cb(&event_data);

  uic_mqtt_unsubscribe_Expect(
    "ucl/by-unid/zb-AABBCCDDEE001122/ProtocolController/NetworkManagement/Write",
    NULL);
  uic_mqtt_unsubscribe_IgnoreArg_callback();

  TEST_ASSERT_EQUAL(0, zigpc_network_management_mqtt_fixt_shutdown());
}

void test_invalid_payload_does_not_call_gateway_commands(void)
{
  zigpc_gateway_on_network_init_t event_data = {0};
  const char *payload                        = "{\"Unsupported\":\"value\"}";

  memcpy(event_data.zigpc_eui64,
         (zigbee_eui64_t){0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x00, 0x11, 0x22},
         sizeof(zigbee_eui64_t));

  zigpc_gateway_register_observer_AddCallback(capture_registered_observers);
  uic_mqtt_subscribe_AddCallback(capture_mqtt_subscription);
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_network_management_mqtt_fixt_setup());

  network_init_cb(&event_data);

  TEST_ASSERT_NOT_NULL(network_mgmt_mqtt_cb);
  network_mgmt_mqtt_cb(subscribed_topic, payload, strlen(payload));
}
