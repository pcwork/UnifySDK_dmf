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

#include "zigpc_cmdmapper_test_cpp_includes.hpp"

struct cmdparser_callback_hash {
  template<class T1, class T2>
  std::size_t operator()(const std::pair<T1, T2> &pair) const
  {
    return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
  }
};

extern "C" {

// Unify includes
#include <unity.h>
#include <sl_status.h>
#include "dotdot_mqtt_mock.h"
#include "dotdot_mqtt_generated_commands_mock.h"

// ZigPC includes
#include <zigpc_common_zigbee.h>
#include "zcl_command_parser_mock.h"

// Component includes
#include "zigpc_command_mapper_int.h"

static std::unordered_map<std::pair<zcl_cluster_id_t, zcl_command_id_t>,
                          zigpc_zclcmdparse_callback_t,
                          cmdparser_callback_hash>
  cmdparser_callbacks;
static std::string captured_generic_report_record_payload;
static uint16_t captured_generic_report_record_tableid;
static uint8_t captured_generic_report_record_record_index;
static uint16_t captured_generic_report_record_total_records;
static zigbee_endpoint_id_t captured_generic_report_record_endpoint;
static std::string captured_raw_fixture_notification_uid;
static uint16_t captured_raw_fixture_notification_modelid;
static zigbee_endpoint_id_t captured_raw_fixture_notification_endpoint;

sl_status_t zigpc_zclcmdparse_register_callback_handler(
  zcl_cluster_id_t cluster_id,
  zcl_command_id_t command_id,
  zigpc_zclcmdparse_callback_t callback,
  int cmock_num_calls)
{
  cmdparser_callbacks.insert({{cluster_id, command_id}, callback});

  return SL_STATUS_OK;
}

sl_status_t zigpc_zclcmdparse_remove_callback_handler(
  zcl_cluster_id_t cluster_id,
  zcl_command_id_t command_id,
  zigpc_zclcmdparse_callback_t callback,
  int cmock_num_calls)
{
  cmdparser_callbacks.erase({cluster_id, command_id});

  return SL_STATUS_OK;
}

void uic_mqtt_dotdot_dmf_bridge_config_publish_generated_raw_fixture_notification_command_handler(
  const dotdot_unid_t,
  const dotdot_endpoint_id_t endpoint,
  const uic_mqtt_dotdot_dmf_bridge_config_command_raw_fixture_notification_fields_t
    *fields,
  int)
{
  captured_raw_fixture_notification_endpoint = endpoint;
  captured_raw_fixture_notification_uid      = fields->uid;
  captured_raw_fixture_notification_modelid  = fields->modelid;
}

void uic_mqtt_dotdot_dmf_bridge_config_publish_generated_generic_report_record_command_handler(
  const dotdot_unid_t,
  const dotdot_endpoint_id_t endpoint,
  const uic_mqtt_dotdot_dmf_bridge_config_command_generic_report_record_fields_t
    *fields,
  int)
{
  captured_generic_report_record_endpoint     = endpoint;
  captured_generic_report_record_tableid      = fields->tableid;
  captured_generic_report_record_record_index = fields->record_index;
  captured_generic_report_record_total_records = fields->total_records;
  captured_generic_report_record_payload      = fields->record_payload;
}

/**
 * @brief Setup the test suite (called once before all test_xxx functions are called)
 *
 */
void suiteSetUp(void)
{
  cmdparser_callbacks.clear();
}

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
void setUp(void)
{
  zigpc_zclcmdparse_register_callback_Stub(
    zigpc_zclcmdparse_register_callback_handler);
  zigpc_zclcmdparse_remove_callback_Stub(
    zigpc_zclcmdparse_remove_callback_handler);
  captured_generic_report_record_payload.clear();
  captured_generic_report_record_tableid       = 0;
  captured_generic_report_record_record_index  = 0;
  captured_generic_report_record_total_records = 0;
  captured_generic_report_record_endpoint      = 0;
  captured_raw_fixture_notification_uid.clear();
  captured_raw_fixture_notification_modelid  = 0;
  captured_raw_fixture_notification_endpoint = 0;
}

/**
 * @brief Teardown after each test case
 *
 */
void tearDown(void) {}

void test_zigpc_command_mapper_setup_gen_cmd_publish_listeners_sanity(void)
{
  // ARRANGE

  // ACT
  sl_status_t status = zigpc_command_mapper_setup_gen_cmd_publish_listeners();

  // ASSERT
  TEST_ASSERT_EQUAL_HEX(SL_STATUS_OK, status);
  TEST_ASSERT_GREATER_THAN(0, cmdparser_callbacks.size());
  TEST_ASSERT_TRUE(cmdparser_callbacks.find(
                     {ZIGPC_ZCL_CLUSTER_GROUPS,
                      ZIGPC_ZCL_CLUSTER_GROUPS_COMMAND_ADD_GROUP_RESPONSE})
                   != cmdparser_callbacks.end());
}

void test_zigpc_command_mapper_publish_add_group_response_sanity(void)
{
  zigbee_eui64_t eui64    = {0xD, 0x01, 0x77, 0x09, 0xD3, 0x8A, 0xD, 0xFF};
  zigbee_endpoint_id_t ep = 5;
  zigpc_zclcmdparse_callback_data_t cb_data
    = {.groups_add_group_response = {.status = 0x25, .group_id = 0x1238,},};

  const uic_mqtt_dotdot_groups_command_add_group_response_fields_t dotdot_fields
    = {
      cb_data.groups_add_group_response.status,
      cb_data.groups_add_group_response.group_id,
    };

  // ARRANGE
  uic_mqtt_dotdot_groups_publish_generated_add_group_response_command_Expect(
    nullptr,
    ep,
    &dotdot_fields);
  uic_mqtt_dotdot_groups_publish_generated_add_group_response_command_IgnoreArg_unid();

  // ACT
  auto group_add_resp_cb = cmdparser_callbacks.find(
    {ZIGPC_ZCL_CLUSTER_GROUPS,
     ZIGPC_ZCL_CLUSTER_GROUPS_COMMAND_ADD_GROUP_RESPONSE});

  group_add_resp_cb->second(eui64, ep, &cb_data);

  // ASSERT (Handled by CMock)
}

void test_zigpc_command_mapper_publish_get_group_membership_response_sanity(
  void)
{
  zigbee_eui64_t eui64    = {0xD, 0x01, 0x77, 0x09, 0xD3, 0x8A, 0xD, 0xFF};
  zigbee_endpoint_id_t ep = 5;
  uint8_t group_count     = 3;
  uint16_t group_list[group_count]          = {0x3023, 0x2193, 0xA};
  zigpc_zclcmdparse_callback_data_t cb_data = {
    .groups_get_group_membership_response
    = {.capacity = 10, .group_list_count = 3, .group_list = group_list},
  };

  const uic_mqtt_dotdot_groups_command_get_group_membership_response_fields_t
    dotdot_fields
    = {
      cb_data.groups_get_group_membership_response.capacity,
      cb_data.groups_get_group_membership_response.group_list_count,
      cb_data.groups_get_group_membership_response.group_list,
    };

  // ARRANGE
  uic_mqtt_dotdot_groups_publish_generated_get_group_membership_response_command_Expect(
    nullptr,
    ep,
    &dotdot_fields);
  uic_mqtt_dotdot_groups_publish_generated_get_group_membership_response_command_IgnoreArg_unid();

  // ACT
  auto group_add_resp_cb = cmdparser_callbacks.find(
    {ZIGPC_ZCL_CLUSTER_GROUPS,
     ZIGPC_ZCL_CLUSTER_GROUPS_COMMAND_GET_GROUP_MEMBERSHIP_RESPONSE});

  group_add_resp_cb->second(eui64, ep, &cb_data);

  // ASSERT (Handled by CMock)
}

void test_zigpc_command_mapper_publish_dmf_bridge_config_generic_report_record_uses_hex_payload(
  void)
{
  zigbee_eui64_t eui64    = {0xD, 0x01, 0x77, 0x09, 0xD3, 0x8A, 0xD, 0xFF};
  zigbee_endpoint_id_t ep = 7;
  uint8_t record_payload[] = {0xCA, 0xFE, 0x00, 0xBE, 0x58, 0x00};
  zigpc_zclcmdparse_callback_data_t cb_data = {
    .dmf_bridge_config_generic_report_record = {
      .tableid               = 0x2211,
      .record_index          = 0x04,
      .total_records         = 0x1020,
      .record_payload_length = 4,
      .record_payload        = reinterpret_cast<const char *>(record_payload),
    },
  };

  // ARRANGE
  sl_status_t status = zigpc_command_mapper_setup_gen_cmd_publish_listeners();
  TEST_ASSERT_EQUAL_HEX(SL_STATUS_OK, status);
  uic_mqtt_dotdot_dmf_bridge_config_publish_generated_generic_report_record_command_Stub(
    uic_mqtt_dotdot_dmf_bridge_config_publish_generated_generic_report_record_command_handler);

  // ACT
  auto generic_report_record_cb = cmdparser_callbacks.find(
    {ZIGPC_ZCL_CLUSTER_DMF_BRIDGE_CONFIG,
     ZIGPC_ZCL_CLUSTER_DMF_BRIDGE_CONFIG_COMMAND_GENERIC_REPORT_RECORD});

  TEST_ASSERT_TRUE(generic_report_record_cb != cmdparser_callbacks.end());
  generic_report_record_cb->second(eui64, ep, &cb_data);

  // ASSERT
  TEST_ASSERT_EQUAL(ep, captured_generic_report_record_endpoint);
  TEST_ASSERT_EQUAL_HEX16(
    cb_data.dmf_bridge_config_generic_report_record.tableid,
    captured_generic_report_record_tableid);
  TEST_ASSERT_EQUAL_HEX8(
    cb_data.dmf_bridge_config_generic_report_record.record_index,
    captured_generic_report_record_record_index);
  TEST_ASSERT_EQUAL_HEX16(
    cb_data.dmf_bridge_config_generic_report_record.total_records,
    captured_generic_report_record_total_records);
  TEST_ASSERT_EQUAL_STRING("CAFE00BE",
                           captured_generic_report_record_payload.c_str());
}

void test_zigpc_command_mapper_publish_dmf_bridge_config_raw_fixture_notification_uses_uid_length(
  void)
{
  zigbee_eui64_t eui64    = {0xD, 0x01, 0x77, 0x09, 0xD3, 0x8A, 0xD, 0xFF};
  zigbee_endpoint_id_t ep = 7;
  char uid[]              = {'A', 'B', 'C', 'D', 'E', 'F', 'X', 'Y', '\0'};
  zigpc_zclcmdparse_callback_data_t cb_data = {
    .dmf_bridge_config_raw_fixture_notification = {
      .uid_length = 6,
      .uid        = uid,
      .modelid    = 0x2211,
    },
  };

  // ARRANGE
  sl_status_t status = zigpc_command_mapper_setup_gen_cmd_publish_listeners();
  TEST_ASSERT_EQUAL_HEX(SL_STATUS_OK, status);
  uic_mqtt_dotdot_dmf_bridge_config_publish_generated_raw_fixture_notification_command_Stub(
    uic_mqtt_dotdot_dmf_bridge_config_publish_generated_raw_fixture_notification_command_handler);

  // ACT
  auto raw_fixture_notification_cb = cmdparser_callbacks.find(
    {ZIGPC_ZCL_CLUSTER_DMF_BRIDGE_CONFIG,
     ZIGPC_ZCL_CLUSTER_DMF_BRIDGE_CONFIG_COMMAND_RAW_FIXTURE_NOTIFICATION});

  TEST_ASSERT_TRUE(raw_fixture_notification_cb != cmdparser_callbacks.end());
  raw_fixture_notification_cb->second(eui64, ep, &cb_data);

  // ASSERT
  TEST_ASSERT_EQUAL(ep, captured_raw_fixture_notification_endpoint);
  TEST_ASSERT_EQUAL_HEX16(
    cb_data.dmf_bridge_config_raw_fixture_notification.modelid,
    captured_raw_fixture_notification_modelid);
  TEST_ASSERT_EQUAL_STRING("414243444546",
                           captured_raw_fixture_notification_uid.c_str());
}

void test_zigpc_command_mapper_cleanup_gen_cmd_publish_listeners_sanity(void)
{
  // ARRANGE

  // ACT
  zigpc_command_mapper_cleanup_gen_cmd_publish_listeners();

  // ASSERT
  TEST_ASSERT_EQUAL(0, cmdparser_callbacks.size());
}

void test_zigpc_command_mapper_setup_gen_cmd_publish_listeners_registers_dmf_bridge_config_command(
  void)
{
  // ACT
  sl_status_t status = zigpc_command_mapper_setup_gen_cmd_publish_listeners();

  // ASSERT
  TEST_ASSERT_EQUAL_HEX(SL_STATUS_OK, status);
  TEST_ASSERT_TRUE(cmdparser_callbacks.find(
                     {ZIGPC_ZCL_CLUSTER_DMF_BRIDGE_CONFIG,
                      ZIGPC_ZCL_CLUSTER_DMF_BRIDGE_CONFIG_COMMAND_GENERIC_REPORT_RECORD})
                   != cmdparser_callbacks.end());
}

}  // extern "C"
