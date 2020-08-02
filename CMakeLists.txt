cmake_minimum_required(VERSION 3.1...3.16)
project(MObs LANGUAGES CXX)
project(MObs VERSION 0.1.3)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
endif()

include_directories(${CMAKE_SOURCE_DIR}/mobs)

option(PACKAGE_TESTS "Build the tests" OFF)
option(BUILD_DOC "Build Documentation" ON)
option(BUILD_MONGO_INTERFACE "Build the Mongo database modules" ON)
option(BUILD_MARIA_INTERFACE "Build the Maria database modules" ON)
option(BUILD_INFORMIX_INTERFACE "Build the Informix database modules" ON)



if(PACKAGE_TESTS)
    enable_testing()
    include(GoogleTest)
    add_subdirectory(tests)
endif()

add_subdirectory(mobs)




add_executable(db db.cpp)
target_link_libraries(db mobs)

add_executable(fileio fileio.cpp)
target_link_libraries(fileio mobs)



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





if(BUILD_MONGO_INTERFACE)
    find_package(libmongocxx REQUIRED)
    find_package(libbsoncxx REQUIRED)
    include_directories(${LIBMONGOCXX_INCLUDE_DIR})
    include_directories(${LIBBSONCXX_INCLUDE_DIR})
    include_directories("/usr/local/include/mongocxx/v_noabi")
    include_directories("/usr/local/include/bsoncxx/v_noabi")
    include_directories("/usr/local/include/libmongoc-1.0")
    include_directories("/usr/local/include/libbson-1.0")
    include_directories("/usr/local/lib")

    target_link_libraries(db ${LIBMONGOCXX_LIBRARIES})
    target_link_libraries(db ${LIBBSONCXX_LIBRARIES})
endif()


if(BUILD_MARIA_INTERFACE)
    find_library(mariadb mariadb)
    target_link_libraries(db ${mariadb})
endif()

if(BUILD_INFORMIX_INTERFACE)
#    find_library(informix informix)
#        MESSAGE(FATAL_ERROR "BOOM ${informix}")
    set(INFORMIXDIR /opt/IBM/Informix_Software_Bundle)
    set(INFX_LIBS ixsql ixasf ixgen ixos ixgls ${INFORMIXDIR}/lib/esql/checkapi.o)

#    add_link_options("LINKER:-rpath,${INFORMIXDIR}/lib/esql")
    target_link_directories(db PUBLIC ${INFORMIXDIR}/lib ${INFORMIXDIR}/lib/esql)
    target_link_libraries(db  ${INFX_LIBS})
endif()


