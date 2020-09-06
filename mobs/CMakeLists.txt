
set(libSrcs objgen.cpp objtypes.cpp logging.cpp strtoobj.cpp objpool.cpp xmlwriter.cpp
        xmlout.cpp xmlread.cpp converter.cpp unixtime.cpp dbifc.cpp helper.cpp mchrono.cpp queryorder.cpp
        converter.h logging.h objpool.h objtypes.h unixtime.h xmlparser.h xmlwriter.h
        jsonparser.h objgen.h objstore.h union.h xmlout.h xmlread.h dbifc.h helper.h mchrono.h  queryorder.h)



if(BUILD_MONGO_INTERFACE)
    find_package(libmongocxx REQUIRED)
    find_package(libbsoncxx REQUIRED)
    include_directories(${LIBMONGOCXX_INCLUDE_DIR})
    include_directories(${LIBBSONCXX_INCLUDE_DIR})
    include_directories("/usr/local/include/mongocxx/v_noabi")
    include_directories("/usr/local/include/bsoncxx/v_noabi")
    include_directories("/usr/local/include/libmongoc-1.0")
    include_directories("/usr/local/include/libbson-1.0")

    set(libSrcs ${libSrcs} mongo.cpp mongo.h)
endif()


if(BUILD_SQLITE_INTERFACE)
    set(libSrcs ${libSrcs} sqlite.cpp sqlite.h)
endif()

if(BUILD_MARIA_INTERFACE)
    include_directories("/usr/local/include/mysql")

    set(libSrcs ${libSrcs} maria.cpp maria.h)
endif()

if(BUILD_INFORMIX_INTERFACE)
    set(INFORMIXDIR /opt/IBM/Informix_Software_Bundle)
    include_directories(${INFORMIXDIR}/incl)
    add_custom_command(OUTPUT infxtools.o
            COMMAND INFORMIXDIR=${INFORMIXDIR} ${INFORMIXDIR}/bin/esql -I${CMAKE_SOURCE_DIR}/mobs -c ${CMAKE_SOURCE_DIR}/mobs/infxtools.ec
            MAIN_DEPENDENCY infxtools.ec DEPENDS infxtools.h BYPRODUCTS infxtools.c
            COMMENT generating esql)

    set(libSrcs ${libSrcs} informix.cpp informix.h infxtools.o)
endif()




add_library(mobs STATIC ${libSrcs})

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
    #target_compile_definitions(mobs PUBLIC INFORMIX_DTFMT_BUG)  # bei Informix-ESQL Version 4.10.4 oder kleiner setzen
endif()

set_property(TARGET mobs PROPERTY CXX_STANDARD 11)
set_property(TARGET mobs PROPERTY POSITION_INDEPENDENT_CODE ON)


set_target_properties(mobs PROPERTIES PUBLIC_HEADER "converter.h;logging.h;objpool.h;objtypes.h;unixtime.h;xmlparser.h;xmlwriter.h;jsonparser.h;objgen.h;objstore.h;union.h;xmlout.h;xmlread.h;mchrono.h;dbifc.h;mongo.h")
install(TARGETS mobs
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include/mobs)

