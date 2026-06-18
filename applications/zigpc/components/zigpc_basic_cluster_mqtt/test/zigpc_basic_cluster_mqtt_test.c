#include <string.h>

#include "unity.h"

#include "uic_mqtt.h"
#include "zigpc_gateway_notify.h"
#include "zigpc_common_zigbee.h"

#include "uic_mqtt_mock.h"
#include "zigpc_gateway_mock.h"
#include "zigpc_zcl_util_mock.h"

sl_status_t zigpc_basic_cluster_mqtt_fixt_setup(void);
int zigpc_basic_cluster_mqtt_fixt_shutdown(void);

typedef void (*mqtt_cb_t)(const char *topic, const char *message, const size_t message_length);
typedef void (*observer_cb_t)(void *event_data);

static mqtt_cb_t mqtt_command_callback = NULL;
static observer_cb_t command_observer_callback = NULL;

static void capture_mqtt_subscription(const char *topic, mqtt_message_callback_t callback, int num_calls)
{
  (void)num_calls;
  TEST_ASSERT_EQUAL_STRING("ucl/by-unid/+/ep+/Basic/Commands/ResetToFactoryDefaults", topic);
  mqtt_command_callback = callback;
}

static sl_status_t capture_gateway_observer(
  enum zigpc_gateway_notify_event event, zigpc_observer_function_t callback, int num_calls)
{
  (void)num_calls;
  if (event == ZIGPC_GATEWAY_NOTIFY_ZCL_COMMAND_RECEIVED) {
    command_observer_callback = callback;
  }
  return SL_STATUS_OK;
}

void setUp(void)
{
  mqtt_command_callback = NULL;
  command_observer_callback = NULL;
}

void tearDown(void) {}

void test_setup_subscribes_and_registers_basic_command_observer(void)
{
  uic_mqtt_subscribe_Stub(capture_mqtt_subscription);
  zigpc_gateway_register_observer_Stub(capture_gateway_observer);

  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_basic_cluster_mqtt_fixt_setup());
  TEST_ASSERT_NOT_NULL(mqtt_command_callback);
  TEST_ASSERT_NOT_NULL(command_observer_callback);
}

void test_mqtt_reset_to_factory_defaults_sends_basic_cluster_command(void)
{
  const char *topic = "ucl/by-unid/zb-0102030405060708/ep1/Basic/Commands/ResetToFactoryDefaults";
  zcl_frame_t frame = {0};
  zigbee_eui64_t eui64 = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

  uic_mqtt_subscribe_Stub(capture_mqtt_subscription);
  zigpc_gateway_register_observer_Stub(capture_gateway_observer);
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_basic_cluster_mqtt_fixt_setup());

  zigpc_zcl_build_command_frame_ExpectAndReturn(NULL,
                                                ZIGPC_ZCL_FRAME_TYPE_CMD_TO_SERVER,
                                                0x0000,
                                                0x00,
                                                0,
                                                NULL,
                                                SL_STATUS_OK);
  zigpc_zcl_build_command_frame_IgnoreArg_frame();
  zigpc_gateway_send_zcl_command_frame_ExpectWithArrayAndReturn(eui64,
                                                                sizeof(zigbee_eui64_t),
                                                                1,
                                                                0x0000,
                                                                NULL,
                                                                1,
                                                                SL_STATUS_OK);
  zigpc_gateway_send_zcl_command_frame_IgnoreArg_frame();

  TEST_ASSERT_NOT_NULL(mqtt_command_callback);
  mqtt_command_callback(topic, "", 0);
}

void test_received_basic_reset_to_factory_defaults_publishes_custom_topic(void)
{
  zigpc_gateway_on_command_received_t event_data = {0};
  const char *expected_topic =
    "ucl/by-unid/zb-0102030405060708/ep1/Basic/Commands/Received/ResetToFactoryDefaults";

  event_data.cluster_id = 0x0000;
  event_data.command_id = 0x00;
  event_data.endpoint_id = 1;
  memcpy(event_data.eui64,
         (zigbee_eui64_t){0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
         sizeof(zigbee_eui64_t));

  uic_mqtt_subscribe_Stub(capture_mqtt_subscription);
  zigpc_gateway_register_observer_Stub(capture_gateway_observer);
  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_basic_cluster_mqtt_fixt_setup());

  uic_mqtt_publish_Expect(expected_topic, "{}", 2, false);

  TEST_ASSERT_NOT_NULL(command_observer_callback);
  command_observer_callback(&event_data);
}
