
set(libSrcs objgen.cpp objtypes.cpp logging.cpp strtoobj.cpp objpool.cpp xmlwriter.cpp
        xmlout.cpp xmlread.cpp converter.cpp unixtime.cpp)

add_library(mobs STATIC ${libSrcs})
set_property(TARGET mobs PROPERTY CXX_STANDARD 11)
set_property(TARGET mobs PROPERTY POSITION_INDEPENDENT_CODE ON)

set_target_properties(mobs PROPERTIES PUBLIC_HEADER "converter.h;logging.h;objpool.h;objtypes.h;unixtime.h;xmlparser.h;xmlwriter.h;jsonparser.h;objgen.h;objstore.h;union.h;xmlout.h;xmlread.h")
install(TARGETS mobs
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include/mobs)