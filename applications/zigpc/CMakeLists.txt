add_subdirectory(components)

add_executable(zigpc main.c)

target_link_libraries(
  zigpc
  PUBLIC zigpc_config
         zigpc_datastore_fixt
         unify)

target_compile_definitions(
  zigpc PRIVATE CMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
                CMAKE_PROJECT_VERSION="${CMAKE_PROJECT_VERSION}")

install(TARGETS zigpc RUNTIME DESTINATION "bin" COMPONENT uic-zigpc)

if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
  install(
    DIRECTORY
    DESTINATION /var/lib/uic
    COMPONENT uic-zigpc)
  install(
    FILES debconf/uic-zigpc.service
    DESTINATION /lib/systemd/system/
    COMPONENT uic-zigpc)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/debconf/config.in
               ${CMAKE_CURRENT_BINARY_DIR}/debconf/config)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/debconf/postinst.in
               ${CMAKE_CURRENT_BINARY_DIR}/debconf/postinst)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/debconf/postrm.in
               ${CMAKE_CURRENT_BINARY_DIR}/debconf/postrm)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/debconf/prerm.in
               ${CMAKE_CURRENT_BINARY_DIR}/debconf/prerm)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/debconf/templates.in
               ${CMAKE_CURRENT_BINARY_DIR}/debconf/templates)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/debconf/conffiles.in
               ${CMAKE_CURRENT_BINARY_DIR}/debconf/conffiles)

add_component_to_uic(
  uic-zigpc # Package Name
  "Universal IOT controller - Zigbee Protocol Controller"
  "uic-zigpc" # Package Debian filename
  "libunify" # Package Depends on
  ""
  "${CMAKE_BINARY_DIR}/applications/zigpc/debconf/config;\
    ${CMAKE_BINARY_DIR}/applications/zigpc/debconf/templates;\
    ${CMAKE_BINARY_DIR}/applications/zigpc/debconf/conffiles;\
    ${CMAKE_BINARY_DIR}/applications/zigpc/debconf/postinst;\
    ${CMAKE_BINARY_DIR}/applications/zigpc/debconf/prerm")
