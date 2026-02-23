
add_compile_options(-Wextra -Wall -Wno-float-conversion)


set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC OFF)
set (CMAKE_AUTOUIC ON)
find_package(Qt5 COMPONENTS Widgets Network REQUIRED)
set (QT_VERSION 5)

add_executable(mrpcqt mrpcqt.cpp mrpcqt.h mainwindow.ui
        mrpcEventLoop.cpp mrpcEventLoop.h
)
target_include_directories(mrpcqt PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(mrpcqt PUBLIC BEFORE SYSTEM ${Qt5_DIR}/../../../include)
target_link_libraries(mrpcqt mobs Qt5::Widgets Qt5::Network)
