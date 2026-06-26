add_library(zigpc_network_management_mqtt src/zigpc_network_management_mqtt.c)

target_include_directories(zigpc_network_management_mqtt PUBLIC include)

target_link_libraries(
  zigpc_network_management_mqtt
  PRIVATE zigpc_common zigpc_gateway uic_mqtt unify)

if(BUILD_TESTING)
  target_add_mock(zigpc_network_management_mqtt)
  add_subdirectory(test)
endif()
