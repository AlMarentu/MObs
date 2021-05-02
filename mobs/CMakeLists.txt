
set(libSrcs objgen.cpp objtypes.cpp logging.cpp strtoobj.cpp objpool.cpp xmlwriter.cpp audittrail.cpp
        xmlout.cpp xmlread.cpp converter.cpp unixtime.cpp dbifc.cpp helper.cpp mchrono.cpp queryorder.cpp
        querygenerator.cpp csb.cpp tcpstream.cpp
        converter.h logging.h objpool.h objtypes.h unixtime.h xmlparser.h xmlwriter.h audittrail.h
        jsonparser.h objgen.h objstore.h union.h xmlout.h xmlread.h dbifc.h helper.h mchrono.h  queryorder.h
        querygenerator.h csb.h tcpstream.h)

find_package(OpenSSL)
if (OPENSSL_FOUND)
    if (BUILD_SHARED_LIBS)
        set(EXTRA_LIBS ${EXTRA_LIBS} ${OPENSSL_LIBRARIES})
    else()
        set(EXTRA_LIBS ${EXTRA_LIBS} ${OPENSSL_INCLUDE_DIR}/../lib/libssl.a)
        set(EXTRA_LIBS ${EXTRA_LIBS} ${OPENSSL_INCLUDE_DIR}/../lib/libcrypto.a)
    endif ()
    include_directories(${OPENSSL_INCLUDE_DIR})
    set(libSrcs ${libSrcs} aes.cpp aes.h rsa.cpp rsa.h digest.cpp digest.h)
endif()



if(BUILD_SQLITE_INTERFACE)
    set(libSrcs ${libSrcs} sqlite.cpp sqlite.h)
    if (EXISTS ${CMAKE_SOURCE_DIR}/extern/sqlite-amalgamation/sqlite3.c)
        include_directories(${CMAKE_SOURCE_DIR}/extern/sqlite-amalgamation)
        set(libSrcs ${libSrcs} ${CMAKE_SOURCE_DIR}/extern/sqlite-amalgamation/sqlite3.c)
    else()
        find_package(SQLite3 REQUIRED)
        set(EXTRA_LIBS ${EXTRA_LIBS} ${SQLite3_LIBRARIES})
    endif()
endif()


if(BUILD_MONGO_INTERFACE)
    find_package(libmongocxx REQUIRED)
    find_package(libbsoncxx REQUIRED)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${LIBMONGOCXX_LIBRARIES})
    include_directories(${LIBMONGOCXX_INCLUDE_DIRS})
    set(libSrcs ${libSrcs} mongo.cpp mongo.h)
endif()


if(BUILD_MARIA_INTERFACE)
    find_library(mariadb mariadb)
    find_path(mariadb_inc mysql)
    if (mariadb_inc)
        include_directories(${mariadb_inc}/mysql)
    else()
        find_path(mariadb_inc mariadb)
        if (mariadb_inc)
            include_directories(${mariadb_inc}/mariadb)
        endif()
    endif()
    set(EXTRA_LIBS ${EXTRA_LIBS} ${mariadb})
    set(libSrcs ${libSrcs} maria.cpp maria.h)
endif()


if(BUILD_INFORMIX_INTERFACE)
    find_package(informix REQUIRED)
    include_directories(${INFORMIX_INCLUDE_DIRS})
    add_custom_command(OUTPUT infxtools.o
            COMMAND INFORMIXDIR=${INFORMIXDIR} ${INFORMIXDIR}/bin/esql -I${CMAKE_SOURCE_DIR}/mobs -c ${CMAKE_SOURCE_DIR}/mobs/infxtools.ec
            MAIN_DEPENDENCY infxtools.ec DEPENDS infxtools.h BYPRODUCTS infxtools.c
            COMMENT generating esql)

    set(libSrcs ${libSrcs} informix.cpp informix.h infxtools.o)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${INFORMIX_LIBRARIES})
endif()

#"C:/Program Files (x86)/mingw-w64/i686-8.1.0-posix-dwarf-rt_v6-rev0/mingw32/i686-w64-mingw32/lib/libws2_32.a" )
if(CMAKE_HOST_WIN32)
    # WinSock2
    set(EXTRA_LIBS ${EXTRA_LIBS} ws2_32)
endif()

add_library(mobs  ${libSrcs})
set_target_properties(mobs PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(mobs PROPERTIES SOVERSION 0)
target_link_libraries(mobs ${EXTRA_LIBS})

if(BUILD_MONGO_INTERFACE)
    target_compile_definitions(mobs PUBLIC USE_MONGO)
endif()

if(BUILD_MARIA_INTERFACE)
    target_compile_definitions(mobs PUBLIC USE_MARIA)
endif()

if(BUILD_SQLITE_INTERFACE)
    target_compile_definitions(mobs PUBLIC USE_SQLITE)
endif()

if(BUILD_INFORMIX_INTERFACE)
    target_compile_definitions(mobs PUBLIC USE_INFORMIX)
endif()

set_property(TARGET mobs PROPERTY CXX_STANDARD 11)
set_property(TARGET mobs PROPERTY POSITION_INDEPENDENT_CODE ON)


set_target_properties(mobs PROPERTIES PUBLIC_HEADER "converter.h;logging.h;objpool.h;objtypes.h;unixtime.h;xmlparser.h;xmlwriter.h;jsonparser.h;objgen.h;objstore.h;union.h;xmlout.h;xmlread.h;mchrono.h;dbifc.h;mongo.h;csb.h;tcpstream.h")
install(TARGETS mobs
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include/mobs)

#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()