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

#ifndef ZIGPC_CLUSTER_CONFIG_H
#define ZIGPC_CLUSTER_CONFIG_H

#include "sl_status.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure the active ZigPC supported cluster set.
 *
 * The list is a comma-separated set of known cluster names. An empty string uses
 * the intentionally small default set.
 */
sl_status_t zigpc_cluster_configure(const char *cluster_list);

/**
 * @brief Check whether a cluster name is currently enabled.
 */
bool zigpc_cluster_is_supported(const char *cluster_name);

/**
 * @brief Return the number of currently enabled clusters.
 */
size_t zigpc_cluster_count(void);

/**
 * @brief Return the active configured cluster IDs.
 */
const uint16_t *zigpc_cluster_get_active_ids(void);

#ifdef __cplusplus
}
#endif

#endif  // ZIGPC_CLUSTER_CONFIG_H
