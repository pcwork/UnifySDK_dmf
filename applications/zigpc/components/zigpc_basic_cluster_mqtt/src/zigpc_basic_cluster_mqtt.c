/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#include "zigpc_basic_cluster_mqtt.h"

#include "sl_log.h"
#include "uic_mqtt.h"
#include "zcl_util.h"
#include "zigpc_common_zigbee.h"
#include "zigpc_gateway.h"
#include "zigpc_gateway_notify.h"

#include <stdio.h>
#include <string.h>

#define LOG_TAG "zigpc_basic_cluster_mqtt"

#define BASIC_CLUSTER_ID 0x0000
#define BASIC_RESET_TO_FACTORY_DEFAULTS_COMMAND_ID 0x00
#define BASIC_RESET_TO_FACTORY_DEFAULTS_SUBSCRIPTION \
  "ucl/by-unid/+/ep+/Basic/Commands/ResetToFactoryDefaults"

static void on_mqtt_reset_to_factory_defaults(const char *topic,
                                              const char *message,
                                              const size_t message_length)
{
  (void)message;
  (void)message_length;

  char unid[32]        = {0};
  unsigned int endpoint = 0;
  zcl_frame_t frame    = {0};
  zigbee_eui64_t eui64 = {0};

  if (sscanf(topic,
             "ucl/by-unid/%31[^/]/ep%u/Basic/Commands/ResetToFactoryDefaults",
             unid,
             &endpoint)
      != 2) {
    sl_log_warning(LOG_TAG, "Ignoring malformed Basic command topic: %s", topic);
    return;
  }

  if (str_to_zigbee_eui64(unid, strlen(unid), eui64) != SL_STATUS_OK) {
    sl_log_warning(LOG_TAG, "Ignoring invalid UNID in topic: %s", topic);
    return;
  }

  if (zigpc_zcl_build_command_frame(&frame,
                                    ZIGPC_ZCL_FRAME_TYPE_CMD_TO_SERVER,
                                    BASIC_CLUSTER_ID,
                                    BASIC_RESET_TO_FACTORY_DEFAULTS_COMMAND_ID,
                                    0,
                                    NULL)
      != SL_STATUS_OK) {
    sl_log_warning(LOG_TAG, "Failed to build Basic reset command frame");
    return;
  }

  if (zigpc_gateway_send_zcl_command_frame(eui64,
                                           (zigbee_endpoint_id_t)endpoint,
                                           BASIC_CLUSTER_ID,
                                           &frame)
      != SL_STATUS_OK) {
    sl_log_warning(LOG_TAG, "Failed to send Basic reset command");
  }
}

static void publish_basic_command_received(const zigpc_gateway_on_command_received_t *event)
{
  char unid[32]  = {0};
  char topic[128] = {0};

  if (zigpc_common_eui64_to_unid(event->eui64, unid, sizeof(unid))
      != SL_STATUS_OK) {
    sl_log_warning(LOG_TAG, "Failed to convert EUI64 to UNID for Basic command");
    return;
  }

  snprintf(topic,
           sizeof(topic),
           "ucl/by-unid/%s/ep%u/Basic/Commands/Received/ResetToFactoryDefaults",
           unid,
           event->endpoint_id);
  uic_mqtt_publish(topic, "{}", 2, false);
}

static void on_gateway_command_received(void *event_data)
{
  if (event_data == NULL) {
    return;
  }

  const zigpc_gateway_on_command_received_t *event
    = (const zigpc_gateway_on_command_received_t *)event_data;

  if ((event->cluster_id == BASIC_CLUSTER_ID)
      && (event->command_id == BASIC_RESET_TO_FACTORY_DEFAULTS_COMMAND_ID)) {
    publish_basic_command_received(event);
  }
}

sl_status_t zigpc_basic_cluster_mqtt_fixt_setup(void)
{
  sl_status_t status = zigpc_gateway_register_observer(
    ZIGPC_GATEWAY_NOTIFY_ZCL_COMMAND_RECEIVED,
    on_gateway_command_received);

  if (status == SL_STATUS_OK) {
    uic_mqtt_subscribe(BASIC_RESET_TO_FACTORY_DEFAULTS_SUBSCRIPTION,
                       on_mqtt_reset_to_factory_defaults);
  }

  return status;
}

int zigpc_basic_cluster_mqtt_fixt_shutdown(void)
{
  uic_mqtt_unsubscribe(BASIC_RESET_TO_FACTORY_DEFAULTS_SUBSCRIPTION,
                       on_mqtt_reset_to_factory_defaults);
  return 0;
}
