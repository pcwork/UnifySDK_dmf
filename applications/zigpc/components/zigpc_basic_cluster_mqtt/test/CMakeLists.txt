target_add_unittest(
  zigpc_basic_cluster_mqtt
  SOURCES
  zigpc_basic_cluster_mqtt_test.c
  DEPENDS
  zigpc_gateway_mock
  zigpc_zcl_util_mock
  uic_mqtt_mock)
