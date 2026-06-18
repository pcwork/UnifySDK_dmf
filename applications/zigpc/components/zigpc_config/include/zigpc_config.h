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

/**
 * @defgroup zigpc_config ZigPC Configuration Extension
 * @ingroup zigpc_components
 *
 * @brief ZigPC-specific configuration parameters.
 *
 * @{
 */

#ifndef ZIGPC_CONFIG_H
#define ZIGPC_CONFIG_H

#include "uic_version.h"

#include <stdbool.h>
#include <stdint.h>

#define DEFAULT_ZIGPC_DATASTORE_FILE UIC_VAR_DIR "/zigpc.db"
#define DEFAULT_ZIGPC_SERIAL_PORT "/dev/ttyUSB0"
#define DEFAULT_ZIGPC_SUPPORTED_CLUSTERS ""
#define DEFAULT_ZIGPC_OTA_PATH UIC_VAR_DIR "/ota-files/"
#define DEFAULT_ZIGPC_USE_TC_WELL_KNOWN_KEY false
#define DEFAULT_ZIGPC_NETWORK_PAN_ID -1
#define DEFAULT_ZIGPC_NETWORK_RADIO_POWER -1
#define DEFAULT_ZIGPC_NETWORK_CHANNEL -1

#define CONFIG_KEY_ZIGPC_DATASTORE_FILE "zigpc.datastore_file"
#define CONFIG_KEY_ZIGPC_SERIAL_PORT "zigpc.serial"
#define CONFIG_KEY_ZIGPC_SUPPORTED_CLUSTERS "zigpc.supported_clusters"
#define CONFIG_KEY_ZIGPC_OTA_PATH "zigpc.ota_path"
#define CONFIG_KEY_ZIGPC_USE_TC_WELL_KNOWN_KEY "zigpc.tc_use_well_known_key"
#define CONFIG_FLAG_ZIGPC_USE_NETWORK_ARGS "zigpc.use_network_args"
#define CONFIG_KEY_ZIGPC_NETWORK_PAN_ID "zigpc.network_panid"
#define CONFIG_KEY_ZIGPC_NETWORK_RADIO_POWER "zigpc.network_radio_power"
#define CONFIG_KEY_ZIGPC_NETWORK_CHANNEL "zigpc.network_channel"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  /// Hostname of the MQTT broker.
  const char *mqtt_host;
  /// Path to a file containing the PEM encoded trusted CA certificate files.
  const char *mqtt_cafile;
  /// Path to a file containing the PEM encoded certificate file for this client.
  const char *mqtt_certfile;
  /// Path to a file containing the PEM encoded unencrypted private key.
  const char *mqtt_keyfile;
  /// Port of the MQTT broker.
  int mqtt_port;
  /// File name for datastore/persistent storage.
  const char *datastore_file;
  /// Serial port used to communicate with the Zigbee NCP.
  const char *serial_port;
  /// Comma-separated supported cluster names.
  const char *supported_clusters;
  /// OTA firmware image directory used by the Zigbee host stack.
  const char *ota_path;
  /// Allow Trust Center joins using the Zigbee well-known link key.
  bool tc_use_well_known_key;
  /// Use explicit network PAN ID, radio power, and channel values.
  bool use_network_args;
  /// Zigbee network PAN ID when use_network_args is enabled.
  uint16_t network_pan_id;
  /// Zigbee radio power when use_network_args is enabled.
  int8_t network_radio_power;
  /// Zigbee network channel when use_network_args is enabled.
  uint8_t network_channel;
} zigpc_config_t;

/**
 * @brief Get the current ZigPC configuration.
 */
const zigpc_config_t *zigpc_get_config(void);

/**
 * @brief Register ZigPC configuration keys in the Unify config system.
 *
 * @returns 0 on success.
 */
int zigpc_config_init(void);

#ifdef __cplusplus
}
#endif

/** @} end zigpc_config */

#endif  // ZIGPC_CONFIG_H
