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

#include "zigpc_config.h"
#include "zigpc_config_fixt.h"
#include "zigpc_datastore_fixt.h"

#include "attribute_store_fixt.h"
#include "datastore_fixt.h"
#include "dotdot_mqtt.h"
#include "sl_log.h"
#include "uic_main.h"
#include "unify_dotdot_attribute_store.h"

#include <stdlib.h>

#define LOG_TAG "zigpc_main"

static uic_fixt_setup_step_t uic_fixt_setup_steps_list[]
  = {  {&zigpc_config_fixt_setup, "ZigPC Configuration"},
  {&zigpc_datastore_fixt_setup, "ZigPC Datastore"},
  {&attribute_store_init, "Attribute store"},
  {&unify_dotdot_attribute_store_init, "Unify DotDot Attribute Store"},
  {&uic_mqtt_dotdot_init, "DotDot MQTT"},
     {NULL, "Terminator"}};

static uic_fixt_shutdown_step_t uic_fixt_shutdown_steps_list[]
  = {{&attribute_store_teardown, "Attribute store"},
     {&datastore_fixt_teardown, "Datastore"},
     {NULL, "Terminator"}};

int main(int argc, char **argv)
{
  if (zigpc_config_init()) {
    return EXIT_FAILURE;
  }

  sl_log_info(LOG_TAG, "Starting the ZigPC main loop");

  return uic_main(uic_fixt_setup_steps_list,
                  uic_fixt_shutdown_steps_list,
                  argc,
                  argv,
                  CMAKE_PROJECT_VERSION);
}
