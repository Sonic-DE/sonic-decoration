# SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

set(dbusmenu_SRCS
    dbusmenu/dbusmenuimporter.cpp
    dbusmenu/dbusmenuimporter.h
    dbusmenu/dbusmenushortcut_p.cpp
    dbusmenu/dbusmenushortcut_p.h
    dbusmenu/dbusmenutypes_p.cpp
    dbusmenu/dbusmenutypes_p.h
    dbusmenu/utils.cpp
    dbusmenu/utils_p.h
)
set_source_files_properties(dbusmenu/com.canonical.dbusmenu.xml PROPERTIES
    NO_NAMESPACE true
    INCLUDE "dbusmenutypes_p.h"
    CLASSNAME DBusMenuInterface
)
qt_add_dbus_interface(dbusmenu_SRCS dbusmenu/com.canonical.dbusmenu.xml dbusmenu_interface)

add_library(kdecorations3titlebarmenu SHARED
    ${dbusmenu_SRCS}
    titlebarmenu.cpp
    titlebarmenu.h
    titlebarmenufactory.cpp
    titlebarmenufactory.h
    titlebarmenuitem.cpp
    titlebarmenuitem.h
    titlebarmenumodel.cpp
    titlebarmenumodel.h
    titlebarmenupopup.cpp
    titlebarmenupopup.h
)
ecm_generate_export_header(kdecorations3titlebarmenu
    VERSION ${PROJECT_VERSION}
    EXPORT_FILE_NAME kdecoration3/titlebarmenu_export.h
)

add_library(KDecoration3::TitleBarMenu ALIAS kdecorations3titlebarmenu)

target_include_directories(kdecorations3titlebarmenu
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}/..;${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR}>"
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/dbusmenu
    INTERFACE
        "$<INSTALL_INTERFACE:${KDECORATION3_INCLUDEDIR}>"
)

target_link_libraries(kdecorations3titlebarmenu
    PUBLIC
        KDecoration3::KDecoration
        Qt6::Widgets
    PRIVATE
        Qt6::DBus
        KF6::I18n
        Sonic::Logging
)

set_target_properties(kdecorations3titlebarmenu PROPERTIES
    VERSION ${KDECORATION3_VERSION}
    SOVERSION ${KDECORATION3_SOVERSION}
    EXPORT_NAME TitleBarMenu
)

ecm_generate_headers(KDecoration3TitleBarMenu_CamelCase_HEADERS
  HEADER_NAMES
    TitleBarMenu
    TitleBarMenuFactory
    TitleBarMenuItem
  PREFIX
    KDecoration3
  REQUIRED_HEADERS KDecoration3TitleBarMenu_HEADERS
)
install(FILES ${KDecoration3TitleBarMenu_CamelCase_HEADERS}
        DESTINATION ${KDECORATION3_INCLUDEDIR}/KDecoration3
        COMPONENT Devel)

install(TARGETS kdecorations3titlebarmenu EXPORT KDecoration3Targets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/kdecoration3/titlebarmenu_export.h
        ${KDecoration3TitleBarMenu_HEADERS}
    DESTINATION
        ${KDECORATION3_INCLUDEDIR}/kdecoration3
    COMPONENT
        Devel
)
