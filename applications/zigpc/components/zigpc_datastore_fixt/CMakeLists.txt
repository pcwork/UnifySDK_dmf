# ZigPC datastore fixture
add_library(zigpc_datastore_fixt src/zigpc_datastore_fixt.c)

target_link_libraries(zigpc_datastore_fixt PRIVATE unify zigpc_config)
target_include_directories(zigpc_datastore_fixt PUBLIC include)

if(BUILD_TESTING)
  target_add_mock(zigpc_datastore_fixt)
  add_subdirectory(test)
endif()
