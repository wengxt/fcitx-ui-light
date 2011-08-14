set(FCITX_LIGHT_UI_SRCS
    font.c
    InputWindow.c
    lightui.c
    lightuiconfig.c
    MenuWindow.c
    MessageWindow.c
    skin.c
    tray.c
    TrayWindow.c
    MainWindow.c
)

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR}/xpm)

add_definitions( -DLOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/share/locale\" )

fcitx_add_addon(fcitx-light-ui ${FCITX_LIGHT_UI_SRCS})
target_link_libraries(fcitx-light-ui
                      ${FCITX4_FCITX_LIBRARIES}
                      ${FCITX4_FCITX_CONFIG_LIBRARIES}
                      ${X11_LIBRARIES}
                      ${X11_Xpm_LIB}
)
