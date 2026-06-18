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

#define DEFAULT_ZIGPC_DATASTORE_FILE UIC_VAR_DIR "/zigpc.db"
#define DEFAULT_ZIGPC_SERIAL_PORT "/dev/ttyUSB0"
#define DEFAULT_ZIGPC_SUPPORTED_CLUSTERS ""

#define CONFIG_KEY_ZIGPC_DATASTORE_FILE "zigpc.datastore_file"
#define CONFIG_KEY_ZIGPC_SERIAL_PORT "zigpc.serial"
#define CONFIG_KEY_ZIGPC_SUPPORTED_CLUSTERS "zigpc.supported_clusters"

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

