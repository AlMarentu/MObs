

include_directories(BEFORE ${PROJECT_SOURCE_DIR}/mobs GTest::include)

add_executable(test1 gtest_main.cc
        testObjtypes.cpp testObjgen.cpp testUnion.cpp testBlob.cpp testParser.cpp
        testUnixTime.cpp testObjpool.cpp testCharset.cpp testWriter.cpp testHelper.cpp testCrypt.cpp
        testStreamBuffer.cpp testMChrono.cpp testMrpc.cpp)

target_link_libraries(test1 GTest::gtest mobs pthread)
if(WIN32)
    target_link_libraries(test1 ${EXTRA_LIBS} stdc++)
else()
    target_link_libraries(test1 ${OPENSSL_LIBRARIES})
endif()


gtest_discover_tests(test1
        # set a working directory so your project root so that you can find test data via paths relative to the project root
        WORKING_DIRECTORY ${PROJECT_DIR}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
        )

add_executable(logtest logmain.cpp)
if(WIN32)
    target_link_libraries(logtest mobs ${EXTRA_LIBS} stdc++)
else()
    target_link_libraries(logtest mobs ${OPENSSL_LIBRARIES})
endif()
