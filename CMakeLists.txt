cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(ccos_image)

project(GRiDISKCOM LANGUAGES CXX C)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

if(WIN32)
    set(GUI_TYPE WIN32)
endif()

include_directories(${CMAKE_SOURCE_DIR})
include_directories(ccos_image)
include_directories(ui)
include_directories(ui/dialogs)
include_directories(ui/preview)
include_directories(common)


add_executable(GRiDISKCOM ${GUI_TYPE}
        main.cpp
        common/imd2raw.c
        common/imd2raw.h
        common/diskconstants.h
        common/mbr.cpp
        common/mbr.h
        common/omf_record.cpp
        common/omf_record.h
        ui/mainwindow.cpp
        ui/mainwindow.h
        ui/mainwindow.ui
        ui/filepanelwidget.cpp
        ui/filepanelwidget.h
        ui/themedicon.h
        ui/dialogs/about.cpp
        ui/dialogs/about.h
        ui/dialogs/about.ui
        ui/dialogs/customdisk.cpp
        ui/dialogs/customdisk.h
        ui/dialogs/customdisk.ui
        ui/dialogs/imagewizard.cpp
        ui/dialogs/imagewizard.h
        ui/dialogs/date.cpp
        ui/dialogs/date.h
        ui/dialogs/date.ui
        ui/dialogs/partition.cpp
        ui/dialogs/partition.h
        ui/dialogs/partition.ui
        ui/dialogs/rename.cpp
        ui/dialogs/rename.h
        ui/dialogs/rename.ui
        ui/dialogs/version.cpp
        ui/dialogs/version.h
        ui/dialogs/version.ui
        ui/preview/file.h
        ui/preview/dialog.cpp
        ui/preview/dialog.h
        ui/preview/text.cpp
        ui/preview/text.h
        ui/preview/canvas.cpp
        ui/preview/canvas.h
        ui/preview/font.cpp
        ui/preview/font.h
        ui/preview/worksheet.cpp
        ui/preview/worksheet.h
        ui/preview/database.cpp
        ui/preview/database.h
        ui/resources/icon.qrc
        execico.rc
        )

if(WIN32)
    target_link_libraries(GRiDISKCOM PRIVATE Qt5::Widgets Qt${QT_VERSION_MAJOR}::Widgets ccos_api)
else()
    target_link_libraries(GRiDISKCOM PRIVATE Qt5::Widgets Qt${QT_VERSION_MAJOR}::Widgets ccos_api -no-pie)
endif()
