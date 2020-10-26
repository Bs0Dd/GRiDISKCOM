cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_COMPILER "g++")

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${SOURCE_DIR})

project(ccosclib)
add_library(ccosclib STATIC
        ${SOURCE_DIR}/string_utils.h
        ${SOURCE_DIR}/string_utils.c
        ${SOURCE_DIR}/common.h
        ${SOURCE_DIR}/common.c
        ${SOURCE_DIR}/ccos_image.h
        ${SOURCE_DIR}/ccos_image.c
        ${SOURCE_DIR}/ccos_private.c
        ${SOURCE_DIR}/ccos_private.h
        )

project(GRiDISKCOM LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets LinguistTools REQUIRED)

set(TS_FILES GRiDISKCOM_en_US.ts)

add_executable(GRiDISKCOM
        ${SOURCE_DIR}/main.cpp
        ${SOURCE_DIR}/mainwindow.cpp
        ${SOURCE_DIR}/mainwindow.h
        ${SOURCE_DIR}/mainwindow.ui
        ${SOURCE_DIR}/wrapper.h
        ${SOURCE_DIR}/wrapper.c
        ${TS_FILES}
        )

target_link_libraries(GRiDISKCOM PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ccosclib)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
