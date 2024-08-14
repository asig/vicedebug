cmake_minimum_required(VERSION 3.5)

project(vicedebug VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Prevent building in the source directory
#

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds not allowed. Use `cmake -B build; cd build; make -j$(nproc)` instead.\n")
endif()

find_package(QT
    NAMES Qt6
    REQUIRED COMPONENTS
        Core
        Network
        Test
        Widgets
)
find_package(Qt6
    REQUIRED COMPONENTS
        Core
        Network
        Test
        Widgets
)

configure_file(src/config.h.in config.h)

include_directories(
    src
    ${PROJECT_BINARY_DIR} # for config.h
)

set(PROJECT_SOURCES
        resources/resources.qrc
        src/resources.h
        src/resources.cpp
        src/dialogs/aboutdialog.h
        src/dialogs/aboutdialog.cpp
        src/dialogs/breakpointdialog.h
        src/dialogs/breakpointdialog.cpp
        src/dialogs/watchdialog.h
        src/dialogs/watchdialog.cpp
        src/dialogs/symboldialog.h
        src/dialogs/symboldialog.cpp
        src/widgets/breakpointswidget.h
        src/widgets/breakpointswidget.cpp
        src/widgets/disassemblywidget.h
        src/widgets/disassemblywidget.cpp
        src/widgets/memorywidget.h
        src/widgets/memorywidget.cpp
        src/widgets/registerswidget.h
        src/widgets/registerswidget.cpp
        src/widgets/watcheswidget.h
        src/widgets/watcheswidget.cpp
        src/widgets/symbolswidget.h
        src/widgets/symbolswidget.cpp
        src/machinestate.h
        src/machinestate.cpp
        src/breakpoints.h
        src/watches.h
        src/watches.cpp
        src/petscii.h
        src/viceclient.h
        src/viceclient.cpp
        src/connectionworker.h
        src/connectionworker.cpp
        src/vectorutils.h
        src/vectorutils.cpp
        src/controller.h
        src/controller.cpp
        src/disassembler.h
        src/disassembler.cpp
        src/disassembler_6502.h
        src/disassembler_6502.cpp
        src/disassembler_z80.h
        src/disassembler_z80.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/focuswatcher.h
        src/focuswatcher.cpp
        src/tooltipgenerators.h
        src/tooltipgenerators.cpp
        src/symtab.h
        src/symtab.cpp
        src/main.cpp
)

qt_add_executable(vicedebug
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_compile_definitions(vicedebug PRIVATE
  $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

target_link_libraries(vicedebug
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Widgets
)

set_target_properties(vicedebug PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS vicedebug
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

qt_finalize_executable(vicedebug)


#
# TESTS
#
enable_testing(true)

qt_add_executable(disassembler_6502_test
    MANUAL_FINALIZATION
    test/disassembler_6502_test.cpp
    src/symtab.h
    src/symtab.cpp
    src/disassembler.h
    src/disassembler.cpp
    src/disassembler_6502.h
    src/disassembler_6502.cpp
)
add_test(NAME disassembler_6502_test COMMAND disassembler_6502_test)

target_link_libraries(disassembler_6502_test
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
)
qt_finalize_executable(disassembler_6502_test)

qt_add_executable(disassembler_z80_test
    MANUAL_FINALIZATION
    test/disassembler_z80_test.cpp
    src/symtab.h
    src/symtab.cpp
    src/disassembler.h
    src/disassembler.cpp
    src/disassembler_z80.h
    src/disassembler_z80.cpp
)
add_test(NAME disassembler_z80_test COMMAND disassembler_z80_test)

target_link_libraries(disassembler_z80_test
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
)
qt_finalize_executable(disassembler_z80_test)


qt_add_executable(watches_test
    MANUAL_FINALIZATION
    test/watches_test.cpp
    src/watches.h
    src/watches.cpp
)
add_test(NAME watches_test COMMAND watches_test)

target_link_libraries(watches_test
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
)
qt_finalize_executable(watches_test)
