# ZigPC config
add_library(zigpc_config src/zigpc_config.c src/zigpc_cluster_config.c)

target_link_libraries(zigpc_config PRIVATE unify uic_log)
target_include_directories(zigpc_config PUBLIC include)
target_compile_definitions(
  zigpc_config PRIVATE CMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
                       CMAKE_PROJECT_VERSION="${CMAKE_PROJECT_VERSION}")

if(BUILD_TESTING)
  add_subdirectory(test)
  target_add_mock(zigpc_config)
endif()

