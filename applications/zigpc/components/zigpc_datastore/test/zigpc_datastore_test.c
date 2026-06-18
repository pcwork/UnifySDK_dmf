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

#include "zigpc_datastore_fixt.h"

#include "datastore_fixt_mock.h"
#include "unity.h"
#include "zigpc_config_mock.h"

#include <string.h>

static zigpc_config_t zigpc_config;

void setUp()
{
  memset(&zigpc_config, 0, sizeof(zigpc_config));
}

void tearDown() {}

void test_zigpc_datastore_uses_configured_database_file()
{
  zigpc_config.datastore_file = "zigpc-test.db";

  zigpc_get_config_ExpectAndReturn(&zigpc_config);
  datastore_fixt_setup_and_handle_version_ExpectAndReturn(
    zigpc_config.datastore_file,
    ZIGPC_CURRENT_DATASTORE_VERSION,
    SL_STATUS_OK);

  TEST_ASSERT_EQUAL(SL_STATUS_OK, zigpc_datastore_fixt_setup());
}

void test_zigpc_datastore_fails_when_config_is_not_ready()
{
  zigpc_get_config_ExpectAndReturn(NULL);

  TEST_ASSERT_EQUAL(SL_STATUS_FAIL, zigpc_datastore_fixt_setup());
}
