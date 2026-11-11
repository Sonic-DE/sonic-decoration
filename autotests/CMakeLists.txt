include(ECMMarkAsTest)

set(decorationButtonTest_SRCS
    mockbridge.cpp mockbridge.h
    mockbutton.cpp mockbutton.h
    mockwindow.cpp mockwindow.h
    mockdecoration.cpp mockdecoration.h
    mocksettings.cpp mocksettings.h
    decorationbuttontest.cpp
    )
add_executable(decorationButtonTest ${decorationButtonTest_SRCS})
target_link_libraries(decorationButtonTest kdecorations3 kdecorations3private Qt::Test)
add_test(NAME kdecoration3-decorationButtonTest COMMAND decorationButtonTest)
ecm_mark_as_test(decorationButtonTest)

set(decorationTest_SRCS
    mockbridge.cpp mockbridge.h
    mockbutton.cpp mockbutton.h
    mockwindow.cpp mockwindow.h
    mockdecoration.cpp mockdecoration.h
    mocksettings.cpp mocksettings.h
    decorationtest.cpp
    )
add_executable(decorationTest ${decorationTest_SRCS})
target_link_libraries(decorationTest kdecorations3 kdecorations3private Qt::Test)
add_test(NAME kdecoration3-decorationTest COMMAND decorationTest)
ecm_mark_as_test(decorationTest)

set(decorationShadowTest_SRCS
    shadowtest.cpp
    )
add_executable(decorationShadowTest ${decorationShadowTest_SRCS})
target_link_libraries(decorationShadowTest kdecorations3 Qt::Test)
add_test(NAME kdecoration3-decorationShadowTest COMMAND decorationShadowTest)
ecm_mark_as_test(decorationShadowTest)

set(titleBarMenuTest_SRCS
    mockbridge.cpp mockbridge.h
    mockbutton.cpp mockbutton.h
    mockwindow.cpp mockwindow.h
    mockdecoration.cpp mockdecoration.h
    mocksettings.cpp mocksettings.h
    titlebarmenutest.cpp
    )
add_executable(titleBarMenuTest ${titleBarMenuTest_SRCS})
target_link_libraries(titleBarMenuTest kdecorations3 kdecorations3private kdecorations3titlebarmenu Qt::Test Qt::Widgets)
add_test(NAME kdecoration3-titleBarMenuTest COMMAND titleBarMenuTest)
set_tests_properties(kdecoration3-titleBarMenuTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
ecm_mark_as_test(titleBarMenuTest)

# A fake menu exporter in its own process, so the D-Bus menu types are registered once per process.
add_executable(titlebarmenudbushelper
    titlebarmenudbushelper.cpp
    ../src/titlebarmenu/dbusmenu/dbusmenutypes_p.cpp
    ../src/titlebarmenu/dbusmenu/dbusmenushortcut_p.cpp
    )
target_include_directories(titlebarmenudbushelper PRIVATE ../src/titlebarmenu/dbusmenu)
target_link_libraries(titlebarmenudbushelper Qt::DBus Qt::Gui)

add_executable(titleBarMenuDBusTest titlebarmenudbustest.cpp)
target_link_libraries(titleBarMenuDBusTest kdecorations3titlebarmenu Qt::Test Qt::Widgets Qt::DBus)
target_compile_definitions(titleBarMenuDBusTest PRIVATE HELPER_PATH="$<TARGET_FILE:titlebarmenudbushelper>")
add_dependencies(titleBarMenuDBusTest titlebarmenudbushelper)
find_program(DBUS_RUN_SESSION_EXECUTABLE dbus-run-session REQUIRED)
add_test(NAME kdecoration3-titleBarMenuDBusTest
    COMMAND ${DBUS_RUN_SESSION_EXECUTABLE} -- $<TARGET_FILE:titleBarMenuDBusTest>)
set_tests_properties(kdecoration3-titleBarMenuDBusTest PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HOME=${CMAKE_CURRENT_BINARY_DIR}/titlebarmenudbustest-home"
    TIMEOUT 60)
ecm_mark_as_test(titleBarMenuDBusTest)
