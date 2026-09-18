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
#ifndef ZIGPC_ATTRMGMT_AVAILABILITY_H
#define ZIGPC_ATTRMGMT_AVAILABILITY_H

#include "sl_status.h"
#include "zigpc_common_zigbee.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Record that a device is alive. This is expected to be invoked
 * whenever any ZCL frame is received from the device (read attribute response
 * or attribute report).
 *
 * If the device was previously marked Unavailable by the heartbeat monitor,
 * its network status is restored to Included and the new state is published on
 * MQTT.
 *
 * @param eui64 Device identifier.
 * @return sl_status_t SL_STATUS_OK on success, SL_STATUS_NULL_POINTER if an
 * invalid device identifier is passed in.
 */
sl_status_t zigpc_attrmgmt_mark_device_alive(const zigbee_eui64_t eui64);

/**
 * @brief Check the availability of all persisted devices. Devices without a
 * response for longer than zigpc.poll_interval seconds are counted as
 * "missed". After zigpc.poll_max_retry consecutive misses, the device network
 * status is set to Unavailable and the state is published on MQTT
 * (ucl/by-unid/<unid>/State).
 *
 * @return sl_status_t SL_STATUS_OK on success, or an error status if a device
 * could not be updated.
 */
sl_status_t zigpc_attrmgmt_check_device_availability(void);

/**
 * @brief Clear all tracked device availability state.
 */
void zigpc_attrmgmt_availability_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* ZIGPC_ATTRMGMT_AVAILABILITY_H */
