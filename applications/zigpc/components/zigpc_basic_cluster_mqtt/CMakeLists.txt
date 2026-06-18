add_library(zigpc_basic_cluster_mqtt src/zigpc_basic_cluster_mqtt.c)

target_include_directories(zigpc_basic_cluster_mqtt PUBLIC include)

target_link_libraries(
  zigpc_basic_cluster_mqtt
  PRIVATE zigpc_common zigpc_gateway zigpc_zcl_util uic_mqtt unify)

if(BUILD_TESTING)
  target_add_mock(zigpc_basic_cluster_mqtt)
  add_subdirectory(test)
endif()
