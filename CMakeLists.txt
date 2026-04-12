cmake_minimum_required(VERSION 3.21)
project(OperationShadowBox LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets Multimedia)
qt_standard_project_setup()
qt_add_executable(OperationShadowBox
    src/main.cpp
    src/MainWindow.cpp
    src/GameWidget.cpp
    src/AudioManager.cpp
    include/MainWindow.h
    include/GameWidget.h
    include/AudioManager.h
    resources.qrc
)
target_include_directories(OperationShadowBox PRIVATE include)
target_link_libraries(OperationShadowBox PRIVATE Qt6::Widgets Qt6::Multimedia)
if (WIN32)
    set_target_properties(OperationShadowBox PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
