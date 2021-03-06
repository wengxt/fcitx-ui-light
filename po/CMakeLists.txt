set(PO_FILES zh_CN.po)
file(RELATIVE_PATH REL_SOURCE_ROOT ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})
if ("${REL_SOURCE_ROOT}" STREQUAL "")
    set(REL_SOURCE_ROOT ".")
endif("${REL_SOURCE_ROOT}" STREQUAL "")

set(POT_FILE ${CMAKE_CURRENT_BINARY_DIR}/fcitx-light-ui.pot)

configure_file(POTFILES.in.in ${CMAKE_CURRENT_BINARY_DIR}/POTFILES.in)

extract_fcitx_addon_conf_postring()

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/desc.po
                   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/getdescpo ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
                   DEPENDS getdescpo)

add_custom_command(
    OUTPUT ${POT_FILE}
    COMMAND INTLTOOL_EXTRACT=${INTLTOOL_EXTRACT} srcdir=${CMAKE_CURRENT_BINARY_DIR} ${INTLTOOL_UPDATE} --gettext-package fcitx-light-ui --pot
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/conf.po ${CMAKE_CURRENT_BINARY_DIR}/desc.po
    )

# Update .po files and compile them to binary .gmo files
gettext_create_translations(${POT_FILE} ALL ${PO_FILES})

