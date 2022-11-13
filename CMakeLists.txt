cmake_minimum_required(VERSION 3.5)

project(vicedebug VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT
    NAMES Qt6
    REQUIRED COMPONENTS
        Core
        Network
        Widgets
)
find_package(Qt6
    REQUIRED COMPONENTS
        Core
        Network
        Widgets
)

include_directories(src)

set(PROJECT_SOURCES
        resources/resources.qrc
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/fonts.h
        src/fonts.cpp
        src/widgets/disassemblywidget.h
        src/widgets/disassemblywidget.cpp
        src/widgets/watcheswidget.h
        src/widgets/watcheswidget.cpp
        src/widgets/registerswidget.h
        src/widgets/registerswidget.cpp
        src/widgets/breakpointswidget.h
        src/widgets/breakpointswidget.cpp
        src/machinestate.h
        src/viceclient.h
        src/viceclient.cpp
        src/connectionworker.h
        src/connectionworker.cpp
        src/vectorutils.h
        src/vectorutils.cpp
        src/controller.h
        src/controller.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(vicedebug
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET vicedebug APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(vicedebug SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(vicedebug
            ${PROJECT_SOURCES}
        )
    endif()
endif()

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

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(vicedebug)
endif()
