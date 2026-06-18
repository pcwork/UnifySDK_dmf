# ZigPC datastore fixture
add_library(zigpc_datastore src/zigpc_datastore_fixt.c)

target_link_libraries(zigpc_datastore PRIVATE unify zigpc_config)
target_include_directories(zigpc_datastore PUBLIC include)

if(BUILD_TESTING)
  target_add_mock(zigpc_datastore)
  add_subdirectory(test)
endif()

