cmake_minimum_required(VERSION 3.7)

project(Early)

###
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

###
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

find_package(Qwt REQUIRED)
add_library(Qwt INTERFACE)
target_include_directories(Qwt INTERFACE ${QWT_INCLUDE_DIRS})
target_link_libraries(Qwt INTERFACE ${QWT_LIBRARIES})

find_package(PkgConfig REQUIRED)
pkg_check_modules(Jack "jack" REQUIRED IMPORTED_TARGET)

###
add_executable(Early WIN32
  sources/app.cpp
  sources/app.h
  sources/ergen.cpp
  sources/ergen.h
  sources/erproc.cpp
  sources/erproc.h
  sources/mainwindow.cpp
  sources/mainwindow.h)
target_link_libraries(Early
  PRIVATE Qt5::Widgets Qwt PkgConfig::Jack)
