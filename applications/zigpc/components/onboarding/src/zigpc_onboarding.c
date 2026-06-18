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

#include <string.h>

#include "sl_log.h"

#include "zigpc_datastore.h"
#include "zigpc_discovery.h"
#include "zigpc_gateway_notify.h"
#include "zigpc_onboarding.h"

static const char LOG_TAG[] = "zigpc_onboarding";

static void zigpc_onboarding_log_discovery_status(
  zigbee_eui64_uint_t eui64_uint, zigpc_discovery_status_t status)
{
  sl_log_info(LOG_TAG,
              "Discovery status %u for EUI64 %016llX",
              status,
              (unsigned long long)eui64_uint);
}

static void zigpc_onboarding_on_network_initialized(void *event_data)
{
  if (event_data == NULL) {
    return;
  }

  const zigpc_gateway_on_network_init_t *net_init
    = (const zigpc_gateway_on_network_init_t *)event_data;

  zigpc_network_data_t network_data = {
    .panid         = net_init->zigpc_panid,
    .radio_channel = net_init->zigpc_radio_channel,
  };
  zigpc_device_data_t gateway_data = {
    .network_status            = ZIGBEE_NODE_STATUS_INCLUDED,
    .max_cmd_delay             = 0,
    .endpoint_total_count      = 1,
    .endpoint_discovered_count = 1,
  };

  memcpy(network_data.gateway_eui64,
         net_init->zigpc_eui64,
         sizeof(zigbee_eui64_t));
  memcpy(network_data.ext_panid,
         net_init->zigpc_ext_panid,
         sizeof(zigbee_ext_panid_t));

  zigpc_datastore_create_network();

  sl_status_t status = zigpc_datastore_write_network(&network_data);
  if (status != SL_STATUS_OK) {
    sl_log_warning(LOG_TAG, "Failed to store network information: 0x%X", status);
    return;
  }

  zigpc_datastore_remove_device(net_init->zigpc_eui64);
  status = zigpc_datastore_create_device(net_init->zigpc_eui64);
  if (status != SL_STATUS_OK) {
    sl_log_warning(LOG_TAG, "Failed to create gateway device: 0x%X", status);
    return;
  }

  status = zigpc_datastore_write_device(net_init->zigpc_eui64, &gateway_data);
  if (status != SL_STATUS_OK) {
    sl_log_warning(LOG_TAG, "Failed to persist gateway device: 0x%X", status);
    return;
  }

  status = zigpc_datastore_create_endpoint(net_init->zigpc_eui64,
                                           net_init->zigpc_endpoint_id);
  if ((status != SL_STATUS_OK) && (status != SL_STATUS_ALREADY_EXISTS)) {
    sl_log_warning(LOG_TAG, "Failed to create gateway endpoint: 0x%X", status);
  }
}

static void zigpc_onboarding_on_node_add_complete(void *event_data)
{
  if (event_data == NULL) {
    return;
  }

  const zigpc_gateway_on_node_add_t *node_add
    = (const zigpc_gateway_on_node_add_t *)event_data;

  zigpc_device_data_t device_data = {
    .network_status            = ZIGBEE_NODE_STATUS_NODEID_ASSIGNED,
    .max_cmd_delay             = 1,
    .endpoint_total_count      = 0,
    .endpoint_discovered_count = 0,
  };

  zigpc_datastore_remove_device(node_add->eui64);

  sl_status_t status = zigpc_datastore_create_device(node_add->eui64);
  if (status != SL_STATUS_OK) {
    sl_log_warning(LOG_TAG, "Failed to create joined device: 0x%X", status);
    return;
  }

  status = zigpc_datastore_write_device(node_add->eui64, &device_data);
  if (status != SL_STATUS_OK) {
    sl_log_warning(LOG_TAG, "Failed to persist joined device: 0x%X", status);
    return;
  }

  status = zigpc_discovery_interview_device(
    zigbee_eui64_to_uint(node_add->eui64),
    zigpc_onboarding_log_discovery_status);
  if (status != SL_STATUS_OK) {
    sl_log_warning(LOG_TAG, "Failed to start node interview: 0x%X", status);
  }
}

static void zigpc_onboarding_on_node_removed(void *event_data)
{
  if (event_data == NULL) {
    return;
  }

  const zigpc_gateway_on_node_removed_t *node_removed
    = (const zigpc_gateway_on_node_removed_t *)event_data;

  sl_status_t status = zigpc_datastore_remove_device(node_removed->eui64);
  if (status != SL_STATUS_OK) {
    sl_log_warning(LOG_TAG, "Failed to remove departed device: 0x%X", status);
  }
}

sl_status_t zigpc_onboarding_fixt_setup(void)
{
  sl_status_t status = zigpc_gateway_register_observer(
    ZIGPC_GATEWAY_NOTIFY_NETWORK_INIT,
    zigpc_onboarding_on_network_initialized);

  if (status == SL_STATUS_OK) {
    status = zigpc_gateway_register_observer(
      ZIGPC_GATEWAY_NOTIFY_NODE_ADD_COMPLETE,
      zigpc_onboarding_on_node_add_complete);
  }

  if (status == SL_STATUS_OK) {
    status = zigpc_gateway_register_observer(
      ZIGPC_GATEWAY_NOTIFY_NODE_REMOVED,
      zigpc_onboarding_on_node_removed);
  }

  return status;
}

int zigpc_onboarding_fixt_shutdown(void)
{
  return 0;
}
