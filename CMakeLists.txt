cmake_minimum_required(VERSION 3.1...3.16)
project(MObs LANGUAGES CXX)
project(MObs VERSION 0.1.3)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
endif()


option(PACKAGE_TESTS "Build the tests" ON)
option(BUILD_DOC "Build Documentation" ON)
option(BUILD_MONGO_INTERFACE "Build the Mongo database modules" ON)
option(BUILD_MARIA_INTERFACE "Build the Maria database modules" ON)
option(BUILD_SQLITE_INTERFACE "Build the SQLite database modules" ON)
option(BUILD_INFORMIX_INTERFACE "Build the Informix database modules" ON)

include_directories(${CMAKE_SOURCE_DIR}/mobs)


if(PACKAGE_TESTS)
    enable_testing()
    include(GoogleTest)
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






add_executable(fileio2 fileio2.cpp)
target_link_libraries(fileio2 mobs)

add_executable(aes2 aes2.cpp)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(aes2 mobs)


#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()