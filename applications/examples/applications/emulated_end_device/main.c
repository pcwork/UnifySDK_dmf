/******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
// EED Components
#include "eed_datastore_fixt.h"
#include "eed_config_fixt.h"
#include "eed_application_monitoring.h"
#include "eed_config.h"
#include "eed_attribute_store.h"
#include "eed_stdin_fixt.h"
#include "eed_node_state.h"
#include "eed_attribute_mapper.h"
#include "eed_group_dispatch.h"
#include "eed_attribute_store_attribute_types.h"
#include "eed_attribute_store_helper.h"
#include "eed_matter_device_type.h"

// Unify components
#include "uic_main.h"
#include "datastore_fixt.h"
#include "attribute_store_fixt.h"
#include "attribute_mapper.h"
#include "dotdot_mqtt.h"
#include "unify_dotdot_attribute_store.h"
#include "attribute_store.h"
#include "attribute_store_helper.h"
#include "sl_log.h"

// Generic incldues
#include <stdlib.h>
#include <string.h>

// Log tag
#define LOG_TAG "main"

#define UNID_FORMAT "eed-%08X"

#define DEFAULT_UNID_VALUE 1

#define DEFAULT_ENDPOINT_ID 0

#define MAXIMUM_CLUSTER_LIST_SIZE 768 

// Helper: configure a single endpoint with given cluster_or_devtype string.
// If the cluster_or_devtype is a known device type, it expands to clusters;
// otherwise it is treated as semicolon-separated cluster names directly.
static sl_status_t eed_configure_single_endpoint(const char *unid, 
                                                  dotdot_endpoint_id_t endpoint_id,
                                                  const char *cluster_or_devtype)
{
  char cluster_list[MAXIMUM_CLUSTER_LIST_SIZE] = {0};

  // Try to interpret as device type first
  if (SL_STATUS_OK == getClustersForDeviceType(cluster_or_devtype, cluster_list, sizeof(cluster_list))) {
    sl_log_info(LOG_TAG, "Configured endpoint %d on %s via device type '%s'", 
                endpoint_id, unid, cluster_or_devtype);
    cluster_config_configure(unid, endpoint_id, cluster_list);
  } else {
    // Treat as direct cluster list (semicolon-separated)
    cluster_config_configure(unid, endpoint_id, cluster_or_devtype);
    sl_log_info(LOG_TAG, "Configured endpoint %d on %s with clusters: %s", 
                endpoint_id, unid, cluster_or_devtype);
  }

  // Check if the UNID node already exists
  if (ATTRIBUTE_STORE_INVALID_NODE == eed_attribute_store_get_unid_node(unid)) {
    attribute_store_node_t unid_node = attribute_store_add_node(ATTRIBUTE_UNID, attribute_store_get_root());
    attribute_store_set_reported_string(unid_node, unid);
  }

  // Check if the endpoint node already exists
  if (ATTRIBUTE_STORE_INVALID_NODE == eed_attribute_store_get_endpoint_node(unid, endpoint_id)) {
    attribute_store_node_t endpoint_node = attribute_store_add_node(ATTRIBUTE_ENDPOINT_ID, eed_attribute_store_get_unid_node(unid));
    attribute_store_set_reported(endpoint_node, &endpoint_id, sizeof(endpoint_id));
  }

  return SL_STATUS_OK;
}

// Parse the endpoint_configs string and configure all endpoints.
// Format: "unid,endpoint,cluster_or_devtype|unid,endpoint,cluster_or_devtype|..."
static sl_status_t eed_endpoint_configs_init(void) {
  const char *endpoint_configs = eed_get_config()->endpoint_configs;
  if (endpoint_configs == NULL || strcmp(endpoint_configs, "") == 0) {
    return SL_STATUS_OK;
  }

  // Make a mutable copy for strtok
  char config_copy[MAXIMUM_CLUSTER_LIST_SIZE] = {0};
  strncpy(config_copy, endpoint_configs, sizeof(config_copy) - 1);

  char *saveptr_entry;
  char *entry = strtok_r(config_copy, "|", &saveptr_entry);
  while (entry != NULL) {
    // Each entry format: "unid_value,endpoint_id,cluster_or_devtype"
    char *saveptr_field;
    char *unid_str = strtok_r(entry, ",", &saveptr_field);
    char *ep_str   = strtok_r(NULL, ",", &saveptr_field);
    char *cluster_str = strtok_r(NULL, ",", &saveptr_field);

    if (unid_str == NULL || ep_str == NULL || cluster_str == NULL) {
      sl_log_warning(LOG_TAG, "Invalid endpoint config entry: '%s', skipping", entry);
      entry = strtok_r(NULL, "|", &saveptr_entry);
      continue;
    }

    int unid_value = atoi(unid_str);
    dotdot_endpoint_id_t endpoint_id = (dotdot_endpoint_id_t)atoi(ep_str);

    char unid[MAXIMUM_UNID_SIZE] = {};
    snprintf(unid, MAXIMUM_UNID_SIZE, UNID_FORMAT, unid_value);

    eed_configure_single_endpoint(unid, endpoint_id, cluster_str);

    entry = strtok_r(NULL, "|", &saveptr_entry);
  }

  return SL_STATUS_OK;
}

sl_status_t eed_supported_cluster_list_init() {

  // First, process the legacy single-endpoint config (cluster_list or device_type)
  if (strcmp(eed_get_config()->cluster_list, "") != 0 || strcmp(eed_get_config()->device_type, "") != 0) {
    int unid_value = DEFAULT_UNID_VALUE;
    dotdot_endpoint_id_t endpoint_id = DEFAULT_ENDPOINT_ID;

    char unid[MAXIMUM_UNID_SIZE] = {};

    snprintf(unid, MAXIMUM_UNID_SIZE, UNID_FORMAT, unid_value);

    if(strcmp(eed_get_config()->device_type, "") != 0){
      char cluster_list[MAXIMUM_CLUSTER_LIST_SIZE] = {0}; // Allocate a buffer for the cluster list
      if (SL_STATUS_OK != getClustersForDeviceType(eed_get_config()->device_type, cluster_list, sizeof(cluster_list)))
      {
        sl_log_error(LOG_TAG, "Failed to convert deviceType: %s", eed_get_config()->device_type);
        return SL_STATUS_FAIL;
      }
      cluster_config_configure(unid, endpoint_id, cluster_list);
    }
    else{
      cluster_config_configure(unid, endpoint_id, eed_get_config()->cluster_list);
    }

    // Check if the UNID node already exists
    if (ATTRIBUTE_STORE_INVALID_NODE == eed_attribute_store_get_unid_node(unid)) {
      // Add the UNID node to the attribute store
      attribute_store_node_t unid_node = attribute_store_add_node(ATTRIBUTE_UNID, attribute_store_get_root());
      attribute_store_set_reported_string(unid_node, unid);
    }

    // Check if the endpoint node already exists
    if (ATTRIBUTE_STORE_INVALID_NODE == eed_attribute_store_get_endpoint_node(unid, endpoint_id)) {
      // Add the endpoint node to the attribute store
      attribute_store_node_t endpoint_node = attribute_store_add_node(ATTRIBUTE_ENDPOINT_ID, eed_attribute_store_get_unid_node(unid));
      attribute_store_set_reported(endpoint_node, &endpoint_id, sizeof(endpoint_id));
    }
  }

  // Then, process the multi-endpoint config (endpoint_configs)
  eed_endpoint_configs_init();

  return SL_STATUS_OK;
}

// List of init functions.
static uic_fixt_setup_step_t uic_fixt_setup_steps_list[]
  = {{&eed_config_fixt_setup, "EED Configuration"},
     {&eed_application_monitoring_init, "EED ApplicationMonitoring"},
     {&eed_datastore_fixt_setup, "Datastore"},
     {&attribute_store_init, "Attribute store"},
     {&eed_node_state_init, "EED Node State"},
     {&unify_dotdot_attribute_store_init, "Unify DotDot Attribute Store"},
     {&eed_group_dispatch_init, "EED Group Dispatch"},
     {&uic_mqtt_dotdot_init, "DotDot MQTT"},
     {&eed_stdin_setup_fixt, "EED stdin"},
     {&eed_attribute_mapper_init, "EED Attribute Mapper"},
     {&eed_attribute_store_init, "EED Attribute Store init"},
     {&eed_supported_cluster_list_init, "EED cluster list init on startup"},
     {NULL, "Terminator"}};

// List of shutdown functions
static uic_fixt_shutdown_step_t uic_fixt_shutdown_steps_list[]
  = {{&attribute_store_teardown, "Attribute store"},
     {&datastore_fixt_teardown, "Datastore"},
     {NULL, "Terminator"}};

/**
 * @brief Main function for the Emulated End Device (EED)
 *
 * @param argc
 * @param argv
 * @return 0 on success (EXIT_SUCCESS), any other code in case of error
 */
int main(int argc, char **argv)
{
  // Initialize the EED Configuration, add new keys/default values as needed.
  attribute_mapper_config_init();
  if (eed_config_init()) {
    return EXIT_FAILURE;
  }

  sl_log_info(LOG_TAG, "Starting the EED main loop");

  // Run uic_main and return result
  return uic_main(uic_fixt_setup_steps_list,
                  uic_fixt_shutdown_steps_list,
                  argc,
                  argv,
                  CMAKE_PROJECT_VERSION);
}
