add_library(zigpc_basic_cluster_mqtt_shared SHARED
            "../$<JOIN:$<TARGET_PROPERTY:zigpc_basic_cluster_mqtt,SOURCES>,;../>")

target_include_directories(
  zigpc_basic_cluster_mqtt_shared
  PUBLIC $<TARGET_PROPERTY:zigpc_basic_cluster_mqtt,INCLUDE_DIRECTORIES>)

target_link_libraries(zigpc_basic_cluster_mqtt_shared PRIVATE zigpc_common unify)

target_add_unittest(
  zigpc_basic_cluster_mqtt_shared
  NAME
  zigpc_basic_cluster_mqtt_test
  SOURCES
  zigpc_basic_cluster_mqtt_test.c
  DEPENDS
  zigpc_gateway_mock
  zigpc_zcl_util_mock
  uic_mqtt_mock)
