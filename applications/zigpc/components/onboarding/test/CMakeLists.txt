target_add_unittest(
  zigpc_onboarding
  SOURCES
  zigpc_onboarding_test.c
  DEPENDS
  zigpc_datastore_mock
  zigpc_discovery_mock
  zigpc_gateway_mock)
