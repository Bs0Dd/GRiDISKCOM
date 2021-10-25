cmake_minimum_required(VERSION 3.5)

project(ccosclib)
add_library(ccosclib STATIC
        ccos_image/string_utils.h
        ccos_image/string_utils.c
        ccos_image/common.h
        ccos_image/common.c
        ccos_image/ccos_image.h
        ccos_image/ccos_image.c
        ccos_image/ccos_private.c
        ccos_image/ccos_private.h
        )

project(GRiDISKCOM LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(ccos_image)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

if(WIN32)
    set(GUI_TYPE WIN32)
endif()

add_executable(GRiDISKCOM ${GUI_TYPE}
        main.cpp
        dialogs/mainwindow.cpp
        dialogs/mainwindow.h
        dialogs/mainwindow.ui
        dialogs/abdlg.cpp
        dialogs/abdlg.h
        dialogs/abdlg.ui
        dialogs/rendlg.cpp
        dialogs/rendlg.h
        dialogs/rendlg.ui
        icon/icon.qrc
        ccos_image/wrapper.h
        ccos_image/wrapper.c
        execico.rc
        )

if(WIN32)
    target_link_libraries(GRiDISKCOM PRIVATE Qt5::Widgets Qt${QT_VERSION_MAJOR}::Widgets ccosclib)
else()
    target_link_libraries(GRiDISKCOM PRIVATE Qt5::Widgets Qt${QT_VERSION_MAJOR}::Widgets ccosclib -no-pie)
endif()
