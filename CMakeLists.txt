cmake_minimum_required(VERSION 3.10)


add_subdirectory(ccos_image)

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
        dialogs/chsedlg.cpp
        dialogs/chsedlg.h
        dialogs/chsedlg.ui
        dialogs/custdlg.cpp
        dialogs/custdlg.h
        dialogs/custdlg.ui
        dialogs/datedlg.cpp
        dialogs/datedlg.h
        dialogs/datedlg.ui
        dialogs/rendlg.cpp
        dialogs/rendlg.h
        dialogs/rendlg.ui
        dialogs/verdlg.cpp
        dialogs/verdlg.h
        dialogs/verdlg.ui
        icon/icon.qrc
        execico.rc
        )

if(WIN32)
    target_link_libraries(GRiDISKCOM PRIVATE Qt5::Widgets Qt${QT_VERSION_MAJOR}::Widgets ccos_api)
else()
    target_link_libraries(GRiDISKCOM PRIVATE Qt5::Widgets Qt${QT_VERSION_MAJOR}::Widgets ccos_api -no-pie)
endif()
