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

#ifndef ZIGPC_DATASTORE_FIXT_H
#define ZIGPC_DATASTORE_FIXT_H

#include "sl_status.h"

/**
 * @brief Versioning of the ZigPC datastore schema.
 */
typedef enum zigpc_datastore_version {
  /**
   * @brief Initial ZigPC datastore revision.
   */
  ZIGPC_DATASTORE_VERSION_V1 = 1,
  /**
   * @brief Keep this after the latest version to detect the latest schema.
   */
  ZIGPC_DATASTORE_VERSION_LAST,
} zigpc_datastore_version_t;

/**
 * @brief Current datastore version used by ZigPC.
 */
#define ZIGPC_CURRENT_DATASTORE_VERSION (ZIGPC_DATASTORE_VERSION_LAST - 1)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fixture for setting up the ZigPC datastore.
 *
 * @return SL_STATUS_OK for success, SL_STATUS_FAIL if an error occurred.
 */
sl_status_t zigpc_datastore_fixt_setup(void);

#ifdef __cplusplus
}
#endif

#endif  // ZIGPC_DATASTORE_FIXT_H

