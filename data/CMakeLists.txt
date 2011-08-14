fcitx_add_addon_conf_file(fcitx-light-ui.conf)
fcitx_add_configdesc_file(fcitx-light-ui.desc)

set(FCITX_DSEKTOP_FILES
    fcitx-light.desktop
)

intltool_merge_translation(${CMAKE_CURRENT_SOURCE_DIR}/fcitx-light.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/fcitx-light.desktop )

add_custom_target(desktopfile ALL DEPENDS ${FCITX_DSEKTOP_FILES})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fcitx-light.desktop DESTINATION share/applications/)