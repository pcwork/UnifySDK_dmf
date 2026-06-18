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
#include "zigpc_cluster_config.h"
#include "zigpc_config_fixt.h"

#include "config.h"
#include "sl_log.h"

#include <string.h>

#define LOG_TAG "zigpc_config"

static zigpc_config_t config;

int zigpc_config_init(void)
{
  config_status_t status = CONFIG_STATUS_OK;

  status |= config_add_string(CONFIG_KEY_ZIGPC_SERIAL_PORT,
                              "Serial port used to communicate with Zigbee NCP",
                              DEFAULT_ZIGPC_SERIAL_PORT);

  status |= config_add_string(CONFIG_KEY_ZIGPC_DATASTORE_FILE,
                              "ZigPC datastore database file",
                              DEFAULT_ZIGPC_DATASTORE_FILE);

  status |= config_add_string(CONFIG_KEY_ZIGPC_SUPPORTED_CLUSTERS,
                              "Comma-separated ZigPC supported cluster names",
                              DEFAULT_ZIGPC_SUPPORTED_CLUSTERS);

  status |= config_add_string(CONFIG_KEY_ZIGPC_OTA_PATH,
                              "OTA file path for Zigbee firmware images",
                              DEFAULT_ZIGPC_OTA_PATH);

  status |= config_add_bool(CONFIG_KEY_ZIGPC_USE_TC_WELL_KNOWN_KEY,
                            "Allow Trust Center joins using well-known key",
                            DEFAULT_ZIGPC_USE_TC_WELL_KNOWN_KEY);

  status |= config_add_flag(CONFIG_FLAG_ZIGPC_USE_NETWORK_ARGS,
                            "Use explicit Zigbee network arguments");

  status |= config_add_int(CONFIG_KEY_ZIGPC_NETWORK_PAN_ID,
                           "PAN ID of Zigbee network",
                           DEFAULT_ZIGPC_NETWORK_PAN_ID);

  status |= config_add_int(CONFIG_KEY_ZIGPC_NETWORK_RADIO_POWER,
                           "Power of Zigbee radio",
                           DEFAULT_ZIGPC_NETWORK_RADIO_POWER);

  status |= config_add_int(CONFIG_KEY_ZIGPC_NETWORK_CHANNEL,
                           "Zigbee network channel",
                           DEFAULT_ZIGPC_NETWORK_CHANNEL);

  return status != CONFIG_STATUS_OK;
}

static int config_get_int_safe(const char *key)
{
  int val = 0;
  if (CONFIG_STATUS_OK != config_get_as_int(key, &val)) {
    sl_log_error(LOG_TAG, "Failed to get int for key: %s", key);
  }
  return val;
}

sl_status_t zigpc_config_fixt_setup(void)
{
  memset(&config, 0, sizeof(config));

  config_status_t status = CONFIG_STATUS_OK;
  status |= config_get_as_string(CONFIG_KEY_ZIGPC_SERIAL_PORT,
                                 &config.serial_port);
  status |= config_get_as_string(CONFIG_KEY_ZIGPC_DATASTORE_FILE,
                                 &config.datastore_file);
  status |= config_get_as_string(CONFIG_KEY_ZIGPC_SUPPORTED_CLUSTERS,
                                 &config.supported_clusters);
  status |= config_get_as_string(CONFIG_KEY_ZIGPC_OTA_PATH, &config.ota_path);
  status |= config_get_as_string(CONFIG_KEY_MQTT_HOST, &config.mqtt_host);
  status |= config_get_as_string(CONFIG_KEY_MQTT_CAFILE, &config.mqtt_cafile);
  status
    |= config_get_as_string(CONFIG_KEY_MQTT_CERTFILE, &config.mqtt_certfile);
  status |= config_get_as_string(CONFIG_KEY_MQTT_KEYFILE, &config.mqtt_keyfile);
  config.mqtt_port = config_get_int_safe(CONFIG_KEY_MQTT_PORT);

  status |= config_get_as_bool(CONFIG_KEY_ZIGPC_USE_TC_WELL_KNOWN_KEY,
                               &config.tc_use_well_known_key);

  config.use_network_args
    = (config_has_flag(CONFIG_FLAG_ZIGPC_USE_NETWORK_ARGS)
       == CONFIG_STATUS_OK);

  int network_pan_id      = 0;
  int network_radio_power = 0;
  int network_channel     = 0;
  status |= config_get_as_int(CONFIG_KEY_ZIGPC_NETWORK_PAN_ID,
                              &network_pan_id);
  status |= config_get_as_int(CONFIG_KEY_ZIGPC_NETWORK_RADIO_POWER,
                              &network_radio_power);
  status |= config_get_as_int(CONFIG_KEY_ZIGPC_NETWORK_CHANNEL,
                              &network_channel);

  if (status != CONFIG_STATUS_OK) {
    return SL_STATUS_FAIL;
  }

  config.network_pan_id      = (uint16_t)network_pan_id;
  config.network_radio_power = (int8_t)network_radio_power;
  config.network_channel     = (uint8_t)network_channel;

  return zigpc_cluster_configure(config.supported_clusters);
}

const zigpc_config_t *zigpc_get_config(void)
{
  return &config;
}
