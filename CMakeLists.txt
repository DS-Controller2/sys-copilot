cmake_minimum_required(VERSION 3.16)
project(SystemMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_subdirectory(src/copilot)

add_executable(SystemMonitor
  src/main.cpp
  src/core/systemmonitor.cpp
  src/ui/mainwindow.cpp
  src/core/systemmonitor.h
  src/ui/mainwindow.h
  src/common/systemdata.h
  resources.qrc
)

target_include_directories(SystemMonitor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(SystemMonitor PRIVATE Qt6::Widgets Qt6::Network copilot)