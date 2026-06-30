/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef SILABS_AF_MAIN_H
#define SILABS_AF_MAIN_H

#include <stdbool.h>
#define SIGNED_ENUM
#include "stack/include/sl_zigbee_types.h"
#include "app/util/ezsp/ezsp-enum.h"

/*  Refer to <GSDK>/protocol/zigbee/app/framework/util/af-main.h
    for more details.
*/

sl_status_t sl_zigbee_af_permit_join(uint8_t duration, bool broadcastMgmtPermitJoin);

sl_status_t sl_zigbee_af_set_ezsp_policy(sl_zigbee_ezsp_policy_id_t policyId,
                                         sl_zigbee_ezsp_decision_id_t decisionId,
                                         const char *policyName,
                                         const char *decisionName);

#endif  // SILABS_AF_MAIN_H
