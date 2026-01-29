cmake_minimum_required(VERSION 3.13)
project(MObs LANGUAGES CXX)
project(MObs VERSION 0.3.0)

# alternative OpenSSL
if(APPLE)
    # homebrew Version
    set (CMAKE_PREFIX_PATH /usr/local/opt/openssl)
elseif(WIN32)
    # https://curl.se/windows/dl-7.76.1/openssl-1.1.1k-win32-mingw.zip
    # set (CMAKE_PREFIX_PATH "C:/openssl-1.1.1k-win32-mingw")
endif()
message(STATUS "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")

option(BUILD_SHARED_LIBS "Build shared lib" OFF)
option(PACKAGE_TESTS "Build the tests" ON)
option(BUILD_DOC "Build Documentation" OFF)
option(BUILD_MONGO_INTERFACE "Build the Mongo database modules" ON)
option(BUILD_MARIA_INTERFACE "Build the Maria database modules" ON)
option(BUILD_SQLITE_INTERFACE "Build the SQLite database modules" ON)
option(BUILD_INFORMIX_INTERFACE "Build the Informix database modules" OFF)


if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif()

add_compile_options(-Wall -Wextra -Wconversion -Woverloaded-virtual -Wnon-virtual-dtor -Wpointer-arith -Wcast-align
        -Wno-unused-local-typedef -Wno-unknown-pragmas -Wno-unused-function
        #TODO
        -Wno-unused-parameter -Wno-type-limits -Wno-sign-conversion
        -Wno-unused-variable -Wno-braced-scalar-init -Wno-implicit-float-conversion -Wno-shorten-64-to-32
        -Wno-deprecated-declarations -Wno-sign-compare -Wno-implicit-int-conversion -Wno-pessimizing-move
        -Wno-unused-private-field -Wno-unused-but-set-variable)

include_directories(${CMAKE_SOURCE_DIR}/mobs)


if(PACKAGE_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    #include(GoogleTest)
    add_subdirectory(tests)
endif()

add_subdirectory(mobs)




add_executable(fileio fileio.cpp)
target_link_libraries(fileio mobs)

add_executable(rsafile rsafile.cpp)
target_link_libraries(rsafile mobs)

add_executable(db db.cpp)
target_link_libraries(db mobs)



# Doxygen Build
if(BUILD_DOC)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(BUILD_DOC_DIR ${CMAKE_SOURCE_DIR}/doc)
        if(NOT EXISTS ${BUILD_DOC_DIR})
            file(MAKE_DIRECTORY ${BUILD_DOC_DIR})
        endif()

        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doxygen.conf)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

        message("Doxygen buld started")
        add_custom_target(Doxygen ALL
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM)
    else(DOXYGEN_FOUND)
        message("Doxygen needs to be installed to generate the documentation.")
    endif(DOXYGEN_FOUND)
endif()






#add_executable(fileio2 fileio2.cpp)
#target_link_libraries(fileio2 mobs)

#add_executable(aes2 aes2.cpp)
#include_directories(${OPENSSL_INCLUDE_DIR})
#target_link_libraries(aes2 mobs)


#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()