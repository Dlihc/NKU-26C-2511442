cmake_minimum_required(VERSION 3.21)
project(NKU_26C_MGS_tribute LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets Multimedia)
qt_standard_project_setup()

qt_add_executable(NKU_26C_MGS_tribute
    src/main.cpp
    src/MainWindow.cpp
    src/GameWidget.cpp
    src/AudioManager.cpp
    include/MainWindow.h
    include/GameWidget.h
    include/AudioManager.h
    resources.qrc
)

target_include_directories(NKU_26C_MGS_tribute PRIVATE include)
target_link_libraries(NKU_26C_MGS_tribute PRIVATE Qt6::Widgets Qt6::Multimedia)
set_target_properties(NKU_26C_MGS_tribute PROPERTIES OUTPUT_NAME "NKU-26C-MGS-tribute")

if (WIN32)
    set_target_properties(NKU_26C_MGS_tribute PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
