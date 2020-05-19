project(MObs LANGUAGES CXX)

cmake_minimum_required(VERSION 3.1...3.16)

include_directories(${CMAKE_SOURCE_DIR}/mobs)

option(PACKAGE_TESTS "Build the tests" OFF)
if(PACKAGE_TESTS)
    enable_testing()
    include(GoogleTest)
    add_subdirectory(tests)
endif()

add_subdirectory(mobs)



add_executable(db db.cpp)
set_property(TARGET db PROPERTY CXX_STANDARD 11)
target_link_libraries(db mobs)


