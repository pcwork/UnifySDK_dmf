add_library(zigpc_onboarding src/zigpc_onboarding.c)

target_include_directories(zigpc_onboarding PUBLIC include)

target_link_libraries(
  zigpc_onboarding
  PRIVATE zigpc_datastore zigpc_discovery zigpc_gateway unify)

if(BUILD_TESTING)
  target_add_mock(zigpc_onboarding)
  add_subdirectory(test)
endif()
