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

#include <string.h>

#include "unity.h"
#include "sl_status.h"

#include "zigpc_common_zigbee.h"
#include "zcl_util.h"

/**
 * @brief Setup the test suite (called once before all test_xxx functions are called)
 *
 */
void suiteSetUp(void) {}

/**
 * @brief Teardown the test suite (called once after all test_xxx functions are called)
 *
 */
int suiteTearDown(int num_failures)
{
  return num_failures;
}

/**
 * @brief Setup before each test case
 *
 */
void setUp(void) {}

/**
 * @brief Teardown after each test case
 *
 */
void tearDown(void) {}

void test_zcl_frame_init_command_input_sanity(void)
{
  // ARRANGE
  sl_status_t status_null_pointer = SL_STATUS_FAIL;
  sl_status_t status_ok = SL_STATUS_FAIL;
  zcl_frame_t frame = {0};
  // ACT
  status_null_pointer = zigpc_zcl_frame_init_command(NULL, 0, 0, 0);
  status_ok           = zigpc_zcl_frame_init_command(&frame, 0, 10, 0xFF);
  // ASSERT
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_NULL_POINTER, status_null_pointer);
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_OK, status_ok);
  TEST_ASSERT_EQUAL(3, frame.size);
}

void test_zcl_frame_fill_data_array_sanity(void)
{
  // ARRANGE
  sl_status_t status = SL_STATUS_FAIL;
  zcl_frame_t frame = {.size = 0};
  uint8_t test_data = 0;

  // ACT
  status = zigpc_zcl_frame_fill_data_array(NULL,
                                           ZIGPC_ZCL_DATA_TYPE_NODATA,
                                           0,
                                           NULL);
  // ASSERT
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_NULL_POINTER, status);
  TEST_ASSERT_EQUAL(0, frame.size);

  // ACT
  status = zigpc_zcl_frame_fill_data_array(&frame,
                                           ZIGPC_ZCL_DATA_TYPE_NODATA,
                                           0,
                                           NULL);
  // ASSERT
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_NULL_POINTER, status);
  TEST_ASSERT_EQUAL(0, frame.size);

  // ACT
  status = zigpc_zcl_frame_fill_data_array(&frame,
                                           ZIGPC_ZCL_DATA_TYPE_UINT8,
                                           0,
                                           &test_data);
  // ASSERT
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_INVALID_COUNT, status);
  TEST_ASSERT_EQUAL(0, frame.size);
}

void test_zcl_frame_fill_data_array_nodata_should_not_modify_frame(void)
{
  // ARRANGE
  sl_status_t status = SL_STATUS_OK;
  zcl_frame_t frame = {.size = 0};
  uint8_t test_data = 0;

  // ACT
  status = zigpc_zcl_frame_fill_data_array(&frame,
                                           ZIGPC_ZCL_DATA_TYPE_NODATA,
                                           1,
                                           &test_data);
  // ASSERT
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_INVALID_TYPE, status);
  TEST_ASSERT_EQUAL(0, frame.size);
}

void test_zcl_frame_fill_data_array_uint8_data(void)
{
  // ARRANGE
  sl_status_t status = SL_STATUS_OK;
  zcl_frame_t frame = {.size = 0};
  uint8_t test_data = 0xF9;

  // ACT
  status = zigpc_zcl_frame_fill_data_array(&frame,
                                           ZIGPC_ZCL_DATA_TYPE_UINT8,
                                           1,
                                           &test_data);
  // ASSERT
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_OK, status);
  TEST_ASSERT_EQUAL(1, frame.size);
  TEST_ASSERT_EQUAL_HEX8(test_data, frame.buffer[0]);
}

void test_zcl_frame_fill_data_array_multiple_uint8_data(void)
{
  // ARRANGE
  sl_status_t status = SL_STATUS_FAIL;
  zcl_frame_t frame    = {.size = 0};
  uint8_t test_data[]  = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
  size_t test_data_len = 6;

  // ACT
  status = zigpc_zcl_frame_fill_data_array(&frame,
                                           ZIGPC_ZCL_DATA_TYPE_UINT8,
                                           test_data_len,
                                           test_data);
  // ASSERT
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_OK, status);
  TEST_ASSERT_EQUAL(6, frame.size);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(test_data, frame.buffer, test_data_len);
}

void test_zcl_frame_fill_data_array_frame_overflow(void)
{
  // ARRANGE
  sl_status_t status= SL_STATUS_FAIL;
  zcl_frame_t frame = {.size = 0};

  // ACT for valid entries
  for (uint8_t i = 0; i < (ZCL_FRAME_BUFFER_SIZE_MAX); i++) {
    status = zigpc_zcl_frame_fill_data_array(&frame,
                                             ZIGPC_ZCL_DATA_TYPE_UINT8,
                                             1,
                                             &i);
    // ASSERT for valid entries
    TEST_ASSERT_EQUAL_HEX8(SL_STATUS_OK, status);
  }
  TEST_ASSERT_EQUAL(255U, frame.size);

  // ACT for overflow value add
  uint8_t data = 0xAB;
  status       = zigpc_zcl_frame_fill_data_array(&frame,
                                           ZIGPC_ZCL_DATA_TYPE_UINT8,
                                           1,
                                           &data);
  // ASSERT for overflow
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_WOULD_OVERFLOW, status);
}

void test_zcl_build_command_frame_sanity(void)
{
  // ARRANGE
  sl_status_t status = SL_STATUS_FAIL;
  zcl_frame_t frame = {0};
  zcl_cluster_id_t cluster_id = 0x0A2F;
  zcl_command_id_t command_id = 0x47;
  size_t arg_count            = 0;

  // ACT
  status = zigpc_zcl_build_command_frame(&frame,
                                         ZIGPC_ZCL_FRAME_TYPE_CMD_TO_SERVER,
                                         cluster_id,
                                         command_id,
                                         arg_count,
                                         NULL);

  // ASSERT
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_OK, status);
  TEST_ASSERT_EQUAL(3, frame.size);
}

void test_zcl_build_command_frame_sanity_multiple_args(void)
{
  // ARRANGE
  sl_status_t status = SL_STATUS_FAIL;
  zcl_frame_t frame = {0};
  zcl_cluster_id_t cluster_id = 0x0A2F;
  zcl_command_id_t command_id = 0x47;
  size_t arg_count            = 3;
  uint8_t arg1_data           = 0x7F;
  uint16_t arg2_data          = 0xFABC;
  char *arg3_data             = "Hello";
  size_t arg3_len             = strlen(arg3_data);

  zigpc_zcl_frame_data_t arg_list[] = {
    {.type = ZIGPC_ZCL_DATA_TYPE_UINT8, .data = &arg1_data},
    {.type = ZIGPC_ZCL_DATA_TYPE_UINT16, .data = &arg2_data},
    {.type = ZIGPC_ZCL_DATA_TYPE_STRING, .data = arg3_data},
  };

  // ACT
  status = zigpc_zcl_build_command_frame(&frame,
                                         ZIGPC_ZCL_FRAME_TYPE_CMD_TO_SERVER,
                                         cluster_id,
                                         command_id,
                                         arg_count,
                                         arg_list);

  // ASSERT
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_OK, status);
  // 3 header + 1 (uint8) + 2 (uint16) + 6 (length + 5 chars)
  TEST_ASSERT_EQUAL(12, frame.size);

  TEST_ASSERT_EQUAL_HEX8(arg1_data, frame.buffer[3]);

  // Stored in little endian
  uint16_t frame_arg2 = (frame.buffer[5] << 8) | frame.buffer[4];
  TEST_ASSERT_EQUAL_HEX16(arg2_data, frame_arg2);

  TEST_ASSERT_EQUAL_HEX8(arg3_len, frame.buffer[6]);
  TEST_ASSERT_EQUAL_STRING_LEN(arg3_data, &frame.buffer[7], arg3_len);
}

void test_zcl_build_command_frame_empty_string_arg(void)
{
  // ARRANGE
  sl_status_t status = SL_STATUS_FAIL;
  zcl_frame_t frame = {0};
  zcl_cluster_id_t cluster_id = 0x0A2F;
  zcl_command_id_t command_id = 0x47;
  size_t arg_count            = 1;
  char *arg1_data             = "";

  zigpc_zcl_frame_data_t arg_list[] = {
    {.type = ZIGPC_ZCL_DATA_TYPE_STRING, .data = arg1_data},
  };

  // ACT
  status = zigpc_zcl_build_command_frame(&frame,
                                         ZIGPC_ZCL_FRAME_TYPE_CMD_TO_SERVER,
                                         cluster_id,
                                         command_id,
                                         arg_count,
                                         arg_list);

  // ASSERT
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_OK, status);
  // 3 header + 1 (string len of 0)
  TEST_ASSERT_EQUAL(4U, frame.size);
  TEST_ASSERT_EQUAL_HEX8(0U, frame.buffer[3]);
}

void test_zcl_build_command_frame_octstr_arg_with_explicit_binary_size(void)
{
  // ARRANGE
  sl_status_t status = SL_STATUS_FAIL;
  zcl_frame_t frame = {0};
  zcl_cluster_id_t cluster_id = 0xFC42;
  zcl_command_id_t command_id = 0x01;
  size_t arg_count            = 1;
  uint8_t arg1_data[]         = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

  zigpc_zcl_frame_data_t arg_list[] = {
    {.type             = ZIGPC_ZCL_DATA_TYPE_OCTSTR,
     .data             = arg1_data,
     .data_size        = sizeof(arg1_data),
     .data_size_is_set = true},
  };

  // ACT
  status = zigpc_zcl_build_command_frame(&frame,
                                         ZIGPC_ZCL_FRAME_TYPE_CMD_TO_SERVER,
                                         cluster_id,
                                         command_id,
                                         arg_count,
                                         arg_list);

  // ASSERT
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_OK, status);
  TEST_ASSERT_EQUAL(11U, frame.size);
  TEST_ASSERT_EQUAL_HEX8(sizeof(arg1_data), frame.buffer[3]);
  TEST_ASSERT_EQUAL_MEMORY(arg1_data, &frame.buffer[4], sizeof(arg1_data));
}

void test_zcl_build_command_frame_arg_overflow(void)
{
  // ARRANGE
  sl_status_t status = SL_STATUS_FAIL;
  zcl_frame_t frame = {0};
  zcl_cluster_id_t cluster_id = 0x0A2F;
  zcl_command_id_t command_id = 0x47;
  size_t arg_count            = 127;
  uint16_t arg_data           = 0x5678;

  zigpc_zcl_frame_data_t arg_list[arg_count];

  for (size_t i = 0U; i < arg_count; i++) {
    arg_list[i].type = ZIGPC_ZCL_DATA_TYPE_UINT16;
    arg_list[i].data = &arg_data;
  }

  // ACT
  status = zigpc_zcl_build_command_frame(&frame,
                                         ZIGPC_ZCL_FRAME_TYPE_CMD_TO_SERVER,
                                         cluster_id,
                                         command_id,
                                         arg_count,
                                         arg_list);

  // ASSERT
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_WOULD_OVERFLOW, status);
  // 3 header + 252 (126 uint16 data)
  TEST_ASSERT_EQUAL(255, frame.size);
}

void test_zcl_build_command_frame_string_arg_invalid(void)
{
  // ARRANGE
  sl_status_t status = SL_STATUS_FAIL;
  zcl_frame_t frame = {0};
  zcl_cluster_id_t cluster_id = 0x0A2F;
  zcl_command_id_t command_id = 0x47;
  size_t arg_count            = 2;
  const char *arg1_data
    = "UQMNaZUFcznYhd0LsMfjnEzG1bzs0HS9PQSUh4TJx0MKzrX7dbbpSNHLNJxTQLolHELHUhVr"
      "PglgNOXoTH0Tulli5TGw4ApkldMM2GtIb4Fv0Ag3Zp8XVxziEXgr1VJ7x7XOhZDlkgetwpiH"
      "2YttQGLSAVxG3G4HGwkfHhwHjdh8YtbdaT8MfErpYraanQDHh47mNPv8Qc5rjZeoivrNxNVN"
      "othG0wzXqkzwmObwQez1yesmKcjLkVbJHcxZ342";
  uint8_t arg2_data = 0x12;

  zigpc_zcl_frame_data_t arg_list[] = {
    {.type = ZIGPC_ZCL_DATA_TYPE_STRING, .data = arg1_data},
    {.type = ZIGPC_ZCL_DATA_TYPE_UINT8, .data = &arg2_data},
  };

  // ACT
  status = zigpc_zcl_build_command_frame(&frame,
                                         ZIGPC_ZCL_FRAME_TYPE_CMD_TO_SERVER,
                                         cluster_id,
                                         command_id,
                                         arg_count,
                                         arg_list);

  // ASSERT
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_INVALID_SIGNATURE, status);
  // 3 header (arg 1 & 2 not added)
  TEST_ASSERT_EQUAL(3, frame.size);
}

void test_zcl_build_command_frame_mfg_cluster_should_include_manufacturer_header(void)
{
  // ARRANGE
  sl_status_t status = SL_STATUS_FAIL;
  zcl_frame_t frame  = {0};
  zcl_cluster_id_t cluster_id = 0xFC42;
  zcl_command_id_t command_id = 0x00;

  // ACT
  status = zigpc_zcl_build_command_frame(&frame,
                                         ZIGPC_ZCL_FRAME_TYPE_GLOBAL_CMD_TO_SERVER,
                                         cluster_id,
                                         command_id,
                                         0,
                                         NULL);

  // ASSERT
  TEST_ASSERT_EQUAL_HEX8(SL_STATUS_OK, status);
  // frame control + manufacturer code (2 bytes) + sequence + command id
  TEST_ASSERT_EQUAL(5, frame.size);
  TEST_ASSERT_EQUAL_HEX8(0x04, frame.buffer[0] & 0x04);
  TEST_ASSERT_EQUAL_HEX8(0x1F, frame.buffer[1]);
  TEST_ASSERT_EQUAL_HEX8(0x12, frame.buffer[2]);
  TEST_ASSERT_EQUAL_HEX8(command_id, frame.buffer[4]);
}

void test_supported_cluster_list_should_include_dmf_bridge_config(void)
{
  // ARRANGE
  const uint16_t *cluster_list         = zigpc_zcl_get_supported_cluster_list();
  size_t supported_cluster_count       = zigpc_zcl_get_number_supported_clusters();
  const uint16_t dmf_bridge_cluster_id = 0xFC42;
  bool cluster_found                   = false;

  // ACT
  for (size_t i = 0; i < supported_cluster_count; i++) {
    if (cluster_list[i] == dmf_bridge_cluster_id) {
      cluster_found = true;
      break;
    }
  }

  // ASSERT
  TEST_ASSERT_TRUE(cluster_found);
}
