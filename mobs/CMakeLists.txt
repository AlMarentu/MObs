
set(libSrcs objgen.cpp objtypes.cpp logging.cpp strtoobj.cpp objpool.cpp xmlwriter.cpp audittrail.cpp
        xmlout.cpp xmlread.cpp converter.cpp unixtime.cpp dbifc.cpp helper.cpp mchrono.cpp queryorder.cpp
        querygenerator.cpp csb.cpp
        converter.h logging.h objpool.h objtypes.h unixtime.h xmlparser.h xmlwriter.h audittrail.h
        jsonparser.h objgen.h objstore.h union.h xmlout.h xmlread.h dbifc.h helper.h mchrono.h  queryorder.h
        querygenerator.h csb.h)

if (UNIX)
    set(libSrcs ${libSrcs} tcpstream.cpp tcpstream.h)
endif()

find_package(OpenSSL)
if (OPENSSL_FOUND)
    if (CMAKE_HOST_APPLE)
        # wird hier falsch ausgewertet, deshalb korrekt setzen
        set(OPENSSL_LIBRARIES /usr/local/opt/openssl/lib/libssl.dylib /usr/local/opt/openssl/lib/libcrypto.dylib)
        set(OPENSSL_INCLUDE_DIR /usr/local/opt/openssl/include)
    endif()
    set(EXTRA_LIBS ${EXTRA_LIBS} ${OPENSSL_LIBRARIES})

    include_directories(${OPENSSL_INCLUDE_DIR})
    set(libSrcs ${libSrcs} aes.cpp aes.h rsa.cpp rsa.h digest.cpp digest.h)
endif()


if(BUILD_MONGO_INTERFACE)
    find_package(libmongocxx REQUIRED)
    find_package(libbsoncxx REQUIRED)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${LIBMONGOCXX_LIBRARIES})
    include_directories(${LIBMONGOCXX_INCLUDE_DIRS})
    set(libSrcs ${libSrcs} mongo.cpp mongo.h)
endif()


if(BUILD_SQLITE_INTERFACE)
    find_package(SQLite3 REQUIRED)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${SQLite3_LIBRARIES})
    set(libSrcs ${libSrcs} sqlite.cpp sqlite.h)
endif()

if(BUILD_MARIA_INTERFACE)
    find_library(mariadb mariadb)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${mariadb})
    include_directories("/usr/local/include/mysql")
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


#add_library(mobs STATIC ${libSrcs})
add_library(mobs SHARED ${libSrcs})
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


set_target_properties(mobs PROPERTIES PUBLIC_HEADER "converter.h;logging.h;objpool.h;objtypes.h;unixtime.h;xmlparser.h;xmlwriter.h;jsonparser.h;objgen.h;objstore.h;union.h;xmlout.h;xmlread.h;mchrono.h;dbifc.h;mongo.h;csb.h")
install(TARGETS mobs
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include/mobs)

