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

#include "zigpc_cluster_config.h"

#include "sl_log.h"

#include <ctype.h>
#include <string.h>

#define LOG_TAG "zigpc_cluster_config"
#define ZIGPC_MAX_ACTIVE_CLUSTERS 3

typedef struct {
  const char *name;
  unsigned int cluster_id;
} zigpc_cluster_definition_t;

static const zigpc_cluster_definition_t supported_cluster_catalog[] = {
  {"Basic", 0x0000},
  {"OnOff", 0x0006},
  {"Level", 0x0008},
};

static const char *const default_cluster_names[] = {"Basic", "OnOff"};

static const zigpc_cluster_definition_t
  *active_clusters[ZIGPC_MAX_ACTIVE_CLUSTERS];
static size_t active_cluster_count;

static const zigpc_cluster_definition_t *find_cluster(const char *cluster_name)
{
  for (size_t i = 0; i < sizeof(supported_cluster_catalog)
                         / sizeof(supported_cluster_catalog[0]);
       i++) {
    if (strcmp(supported_cluster_catalog[i].name, cluster_name) == 0) {
      return &supported_cluster_catalog[i];
    }
  }

  return NULL;
}

static void trim_token(char *token)
{
  char *start = token;
  while (isspace((unsigned char)*start)) {
    start++;
  }

  char *end = start + strlen(start);
  while ((end > start) && isspace((unsigned char)*(end - 1))) {
    end--;
  }
  *end = '\0';

  if (start != token) {
    memmove(token, start, strlen(start) + 1);
  }
}

static sl_status_t append_cluster(
  const zigpc_cluster_definition_t *cluster,
  const zigpc_cluster_definition_t **clusters,
  size_t *cluster_count)
{
  for (size_t i = 0; i < *cluster_count; i++) {
    if (clusters[i] == cluster) {
      return SL_STATUS_OK;
    }
  }

  if (*cluster_count >= ZIGPC_MAX_ACTIVE_CLUSTERS) {
    sl_log_error(LOG_TAG, "Too many ZigPC clusters configured");
    return SL_STATUS_FAIL;
  }

  clusters[*cluster_count] = cluster;
  (*cluster_count)++;
  return SL_STATUS_OK;
}

static sl_status_t configure_default_clusters(
  const zigpc_cluster_definition_t **clusters, size_t *cluster_count)
{
  for (size_t i = 0; i < sizeof(default_cluster_names)
                         / sizeof(default_cluster_names[0]);
       i++) {
    const zigpc_cluster_definition_t *cluster
      = find_cluster(default_cluster_names[i]);
    if ((cluster == NULL)
        || (append_cluster(cluster, clusters, cluster_count) != SL_STATUS_OK)) {
      return SL_STATUS_FAIL;
    }
  }

  return SL_STATUS_OK;
}

sl_status_t zigpc_cluster_configure(const char *cluster_list)
{
  const zigpc_cluster_definition_t *new_clusters[ZIGPC_MAX_ACTIVE_CLUSTERS]
    = {NULL};
  size_t new_cluster_count = 0;

  if ((cluster_list == NULL) || (strlen(cluster_list) == 0)) {
    if (configure_default_clusters(new_clusters, &new_cluster_count)
        != SL_STATUS_OK) {
      return SL_STATUS_FAIL;
    }
  } else {
    char cluster_list_copy[128] = {0};
    if (strlen(cluster_list) >= sizeof(cluster_list_copy)) {
      sl_log_error(LOG_TAG, "ZigPC cluster list is too long");
      return SL_STATUS_FAIL;
    }

    strncpy(cluster_list_copy, cluster_list, sizeof(cluster_list_copy) - 1);

    char *saveptr = NULL;
    char *token   = strtok_r(cluster_list_copy, ",", &saveptr);
    while (token != NULL) {
      trim_token(token);
      const zigpc_cluster_definition_t *cluster = find_cluster(token);
      if (cluster == NULL) {
        sl_log_error(LOG_TAG, "Unsupported ZigPC cluster: %s", token);
        return SL_STATUS_FAIL;
      }

      if (append_cluster(cluster, new_clusters, &new_cluster_count)
          != SL_STATUS_OK) {
        return SL_STATUS_FAIL;
      }
      token = strtok_r(NULL, ",", &saveptr);
    }
  }

  memcpy(active_clusters, new_clusters, sizeof(active_clusters));
  active_cluster_count = new_cluster_count;

  return SL_STATUS_OK;
}

bool zigpc_cluster_is_supported(const char *cluster_name)
{
  if (cluster_name == NULL) {
    return false;
  }

  for (size_t i = 0; i < active_cluster_count; i++) {
    if (strcmp(active_clusters[i]->name, cluster_name) == 0) {
      return true;
    }
  }

  return false;
}

size_t zigpc_cluster_count(void)
{
  return active_cluster_count;
}

