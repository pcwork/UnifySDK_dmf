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

#include "zigpc_network_management_mqtt.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "sl_log.h"
#include "uic_mqtt.h"
#include "zigpc_common_zigbee.h"
#include "zigpc_gateway.h"
#include "zigpc_gateway_notify.h"

#define LOG_TAG "zigpc_nwmgmt_mqtt"
#define NETWORK_MANAGEMENT_WRITE_TOPIC_FMT \
  "ucl/by-unid/%s/ProtocolController/NetworkManagement/Write"

static char subscribed_topic[MQTT_TOPIC_MAX_LENGTH] = {0};

static const char *skip_whitespace(const char *cursor)
{
  while ((cursor != NULL) && (*cursor != '\0') && isspace((unsigned char)*cursor)) {
    cursor++;
  }

  return cursor;
}

static sl_status_t extract_json_string_value(const char *payload,
                                             const char *key,
                                             char *dest,
                                             size_t dest_size)
{
  char key_pattern[64] = {0};
  const char *cursor   = NULL;
  const char *end      = NULL;
  size_t length        = 0;

  if ((payload == NULL) || (key == NULL) || (dest == NULL) || (dest_size == 0)) {
    return SL_STATUS_NULL_POINTER;
  }

  if (snprintf(key_pattern, sizeof(key_pattern), "\"%s\"", key)
      >= (int)sizeof(key_pattern)) {
    return SL_STATUS_WOULD_OVERFLOW;
  }

  cursor = strstr(payload, key_pattern);
  if (cursor == NULL) {
    return SL_STATUS_NOT_FOUND;
  }

  cursor = strchr(cursor + strlen(key_pattern), ':');
  if (cursor == NULL) {
    return SL_STATUS_INVALID_CONFIGURATION;
  }

  cursor = skip_whitespace(cursor + 1);
  if ((cursor == NULL) || (*cursor != '"')) {
    return SL_STATUS_INVALID_TYPE;
  }

  cursor++;
  end = strchr(cursor, '"');
  if (end == NULL) {
    return SL_STATUS_INVALID_CONFIGURATION;
  }

  length = (size_t)(end - cursor);
  if (length >= dest_size) {
    return SL_STATUS_WOULD_OVERFLOW;
  }

  memcpy(dest, cursor, length);
  dest[length] = '\0';
  return SL_STATUS_OK;
}

static void on_network_management_write(const char *topic,
                                        const char *message,
                                        const size_t message_length)
{
  char state[32]       = {0};
  char unid[32]        = {0};
  zigbee_eui64_t eui64 = {0};
  sl_status_t status   = SL_STATUS_OK;

  (void)topic;
  (void)message_length;

  status = extract_json_string_value(message, "State", state, sizeof(state));
  if (status != SL_STATUS_OK) {
    sl_log_warning(LOG_TAG, "Ignoring invalid network management payload");
    return;
  }

  if (strcmp(state, "add node") == 0) {
    (void)zigpc_gateway_network_permit_joins(true);
    return;
  }

  if (strcmp(state, "idle") == 0) {
    (void)zigpc_gateway_network_permit_joins(false);
    return;
  }

  if (strcmp(state, "remove node") == 0) {
    status = extract_json_string_value(message, "Unid", unid, sizeof(unid));
    if (status != SL_STATUS_OK) {
      sl_log_warning(LOG_TAG, "Missing Unid for remove node request");
      return;
    }

    status = str_to_zigbee_eui64(unid, strlen(unid), eui64);
    if (status != SL_STATUS_OK) {
      sl_log_warning(LOG_TAG, "Invalid Unid for remove node request");
      return;
    }

    (void)zigpc_gateway_remove_node(eui64);
    return;
  }

  sl_log_warning(LOG_TAG, "Ignoring unsupported network management state: %s",
                 state);
}

static void on_network_initialized(void *event_data)
{
  const zigpc_gateway_on_network_init_t *network_init
    = (const zigpc_gateway_on_network_init_t *)event_data;
  char unid[32] = {0};

  if (network_init == NULL) {
    return;
  }

  if (zigpc_common_eui64_to_unid(network_init->zigpc_eui64,
                                 unid,
                                 sizeof(unid))
      != SL_STATUS_OK) {
    sl_log_warning(LOG_TAG,
                   "Failed to build ProtocolController network management topic");
    return;
  }

  if (snprintf(subscribed_topic,
               sizeof(subscribed_topic),
               NETWORK_MANAGEMENT_WRITE_TOPIC_FMT,
               unid)
      >= (int)sizeof(subscribed_topic)) {
    subscribed_topic[0] = '\0';
    sl_log_warning(LOG_TAG,
                   "ProtocolController network management topic too long");
    return;
  }

  uic_mqtt_subscribe(subscribed_topic, on_network_management_write);
}

sl_status_t zigpc_network_management_mqtt_fixt_setup(void)
{
  return zigpc_gateway_register_observer(ZIGPC_GATEWAY_NOTIFY_NETWORK_INIT,
                                         on_network_initialized);
}

int zigpc_network_management_mqtt_fixt_shutdown(void)
{
  if (subscribed_topic[0] != '\0') {
    uic_mqtt_unsubscribe(subscribed_topic, on_network_management_write);
    subscribed_topic[0] = '\0';
  }

  return 0;
}
