add_library(
  zigpc_network_management_mqtt_shared SHARED
  "../$<JOIN:$<TARGET_PROPERTY:zigpc_network_management_mqtt,SOURCES>,;../>")

target_include_directories(
  zigpc_network_management_mqtt_shared
  PUBLIC
    $<TARGET_PROPERTY:zigpc_network_management_mqtt,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:zigpc_gateway,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:uic_mqtt,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:uic_log,INTERFACE_INCLUDE_DIRECTORIES>)

target_link_libraries(
  zigpc_network_management_mqtt_shared
  PRIVATE zigpc_common unify)

target_add_unittest(
  zigpc_network_management_mqtt_shared
  NAME
  zigpc_network_management_mqtt_test
  SOURCES
  zigpc_network_management_mqtt_test.c
  DEPENDS
  zigpc_gateway_mock
  uic_mqtt_mock)
