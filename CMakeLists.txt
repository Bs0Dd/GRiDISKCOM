cmake_minimum_required(VERSION 3.5)

project(ccosclib)
add_library(ccosclib STATIC
        string_utils.h
        string_utils.c
        common.h
        common.c
        ccos_image.h
        ccos_image.c
        ccos_private.c
        ccos_private.h
        )

project(GRiDISKCOM LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

add_executable(GRiDISKCOM
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        about.cpp
        about.h
        about.ui
        rename.cpp
        rename.h
        rename.ui
        img.qrc
        wrapper.h
        wrapper.c
        execico.rc
        )

if(WIN32)
    target_link_libraries(GRiDISKCOM PRIVATE Qt5::Widgets Qt${QT_VERSION_MAJOR}::Widgets ccosclib)
else()
    target_link_libraries(GRiDISKCOM PRIVATE Qt5::Widgets Qt${QT_VERSION_MAJOR}::Widgets ccosclib -no-pie)
endif()
