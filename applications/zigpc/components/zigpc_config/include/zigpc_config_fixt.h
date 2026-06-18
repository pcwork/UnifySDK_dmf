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

#ifndef ZIGPC_CONFIG_FIXT_H
#define ZIGPC_CONFIG_FIXT_H

#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read and validate ZigPC configuration after config_parse().
 */
sl_status_t zigpc_config_fixt_setup(void);

#ifdef __cplusplus
}
#endif

#endif  // ZIGPC_CONFIG_FIXT_H

