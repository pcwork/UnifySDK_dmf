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

#ifndef ZIGPC_BASIC_CLUSTER_MQTT_H
#define ZIGPC_BASIC_CLUSTER_MQTT_H

#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

sl_status_t zigpc_basic_cluster_mqtt_fixt_setup(void);
int zigpc_basic_cluster_mqtt_fixt_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
