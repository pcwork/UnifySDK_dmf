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

#include <algorithm>
#include <vector>

#include "sl_log.h"
#include "sl_status.h"

#include "sys/clock.h"

#include "zigpc_common_zigbee.h"
#include "zigpc_config.h"
#include "zigpc_datastore.h"
#include "zigpc_datastore.hpp"
#include "zigpc_ucl.hpp"

#include "zigpc_attrmgmt_availability.h"

static const char LOG_TAG[] = "zigpc_attrmgmt_availability";

typedef struct {
  zigbee_eui64_uint_t eui64;
  clock_time_t last_seen;
  uint32_t miss_count;
  bool unavailable_published;
} device_availability_entry_t;

static std::vector<device_availability_entry_t> availability_list;

static device_availability_entry_t *
  find_availability_entry(const zigbee_eui64_uint_t eui64)
{
  auto entry_iter
    = std::find_if(availability_list.begin(),
                   availability_list.end(),
                   [&eui64](const device_availability_entry_t &entry) {
                     return entry.eui64 == eui64;
                   });

  return (entry_iter != availability_list.end()) ? &(*entry_iter) : nullptr;
}

void zigpc_attrmgmt_availability_reset(void)
{
  availability_list.clear();
}

sl_status_t zigpc_attrmgmt_mark_device_alive(const zigbee_eui64_t eui64)
{
  sl_status_t status = SL_STATUS_OK;

  if (eui64 == nullptr) {
    status = SL_STATUS_NULL_POINTER;
  } else {
    const zigbee_eui64_uint_t eui64_uint = zigbee_eui64_to_uint(eui64);

    device_availability_entry_t *entry = find_availability_entry(eui64_uint);
    if (entry == nullptr) {
      availability_list.push_back(
        device_availability_entry_t{eui64_uint, clock_time(), 0U, false});
      entry = &availability_list.back();
    }

    entry->last_seen   = clock_time();
    entry->miss_count  = 0U;

    // If the device was previously marked Unavailable by the heartbeat
    // monitor, restore it and notify subscribers.
    if (entry->unavailable_published) {
      zigpc_device_data_t device_data;
      status = zigpc_datastore_read_device(eui64, &device_data);
      if (status == SL_STATUS_OK) {
        if (device_data.network_status == ZIGBEE_NODE_STATUS_UNAVAILABLE) {
          device_data.network_status = ZIGBEE_NODE_STATUS_INCLUDED;

          status = zigpc_datastore_write_device(eui64, &device_data);
          if (status == SL_STATUS_OK) {
            status = zigpc_ucl::node_state::publish_state(
              eui64_uint,
              device_data.network_status,
              device_data.max_cmd_delay);
          }
        }

        if (status == SL_STATUS_OK) {
          entry->unavailable_published = false;
        }
      }
    }
  }

  return status;
}

sl_status_t zigpc_attrmgmt_check_device_availability(void)
{
  sl_status_t status = SL_STATUS_OK;

  const zigpc_config_t *const config = zigpc_get_config();

  if ((config == nullptr) || (config->poll_interval <= 0)
      || (config->poll_max_retry <= 0)) {
    // Heartbeat monitoring disabled by configuration.
    return SL_STATUS_OK;
  }

  // The coordinator (NCP + zigpc host) is registered in the datastore but
  // is never polled, so exclude it from heartbeat monitoring.
  zigpc_network_data_t network_data;
  status = zigpc_datastore_read_network(&network_data);
  if (status != SL_STATUS_OK) {
    sl_log_warning(LOG_TAG,
                   "Failed to read network info, skipping availability check: 0x%X",
                   status);
    return SL_STATUS_OK;
  }
  const zigbee_eui64_uint_t gateway_uint
    = zigbee_eui64_to_uint(network_data.gateway_eui64);

  const clock_time_t now              = clock_time();
  const clock_time_t missed_threshold
    = (clock_time_t)(CLOCK_SECOND * config->poll_interval);

  const std::vector<zigbee_eui64_uint_t> stored_devices
    = zigpc_datastore::device::get_id_list();

  for (const zigbee_eui64_uint_t eui64_uint: stored_devices) {
    if (eui64_uint == gateway_uint) {
      // Drop any stale tracking entry left behind by an earlier version.
      availability_list.erase(
        std::remove_if(
          availability_list.begin(),
          availability_list.end(),
          [&gateway_uint](const device_availability_entry_t &entry) {
            return entry.eui64 == gateway_uint;
          }),
        availability_list.end());
      continue;
    }

    device_availability_entry_t *entry = find_availability_entry(eui64_uint);

    if (entry == nullptr) {
      // First time this device is observed: start tracking from now. It will
      // only be marked Unavailable after poll_max_retry consecutive misses.
      availability_list.push_back(
        device_availability_entry_t{eui64_uint, now, 0U, false});
      continue;
    }

    const clock_time_t elapsed = (clock_time_t)(now - entry->last_seen);

    if (elapsed < missed_threshold) {
      entry->miss_count = 0U;
      continue;
    }

    entry->miss_count++;
    if (entry->miss_count > (uint32_t)config->poll_max_retry) {
      entry->miss_count = (uint32_t)config->poll_max_retry;
    }

    if ((entry->miss_count >= (uint32_t)config->poll_max_retry)
        && (entry->unavailable_published == false)) {
      zigbee_eui64_t eui64;
      status = zigbee_uint_to_eui64(eui64_uint, eui64);
      if (status == SL_STATUS_OK) {
        zigpc_device_data_t device_data;
        status = zigpc_datastore_read_device(eui64, &device_data);
        if (status == SL_STATUS_OK) {
          if (device_data.network_status != ZIGBEE_NODE_STATUS_UNAVAILABLE) {
            device_data.network_status = ZIGBEE_NODE_STATUS_UNAVAILABLE;

            status = zigpc_datastore_write_device(eui64, &device_data);
          }

          if (status == SL_STATUS_OK) {
            status = zigpc_ucl::node_state::publish_state(
              eui64_uint,
              device_data.network_status,
              device_data.max_cmd_delay);

            if (status == SL_STATUS_OK) {
              entry->unavailable_published = true;
              sl_log_info(LOG_TAG,
                          "Device EUI64:%016" PRIX64
                          " marked Unavailable (heartbeat missed)",
                          eui64_uint);
            }
          }
        }
      }

      if (status != SL_STATUS_OK) {
        sl_log_warning(LOG_TAG,
                       "Failed to mark device EUI64:%016" PRIX64
                       " Unavailable: 0x%X",
                       eui64_uint,
                       status);
        status = SL_STATUS_OK;
      }
    }
  }

  // Remove tracking entries for devices no longer in the datastore.
  availability_list.erase(
    std::remove_if(
      availability_list.begin(),
      availability_list.end(),
      [&stored_devices](const device_availability_entry_t &entry) {
        return std::find(stored_devices.begin(),
                         stored_devices.end(),
                         entry.eui64)
               == stored_devices.end();
      }),
    availability_list.end());

  return SL_STATUS_OK;
}
