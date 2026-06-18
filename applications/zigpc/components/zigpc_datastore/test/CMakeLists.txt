target_add_unittest(
  zigpc_datastore
  NAME
  zigpc_datastore_test
  SOURCES
  zigpc_datastore_test.c
  DEPENDS
  zigpc_config_mock
  uic_datastore_mock)

