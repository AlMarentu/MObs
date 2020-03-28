# :vim noet

CXX     = clang++

TARGETS		= mom hw db tests

CPPFLAGS	= -I/usr/local/include -I.
CXXFLAGS	= -std=c++11 -g -Wall
LDLIBS		= 
LDFLAGS		= -L/usr/local/lib

all::	$(TARGETS)
	

clean::
	$(RM) *.o $(TARGETS)

mobs.a: objgen.o logging.o objpool.o dumpjson.o readjson.o xmlout.o xmlread.o
	$(AR) -rc $@ $^

# Achtung linkage: objgen.o immer zuerst
mom: mobs.a momtest.o
	$(CXX) $(LDFLAGS) $(LDLIBS) -o $@ $^

hw: mobs.a hw.o
	$(CXX) $(LDFLAGS) $(LDLIBS) -o $@ $^

db: mobs.a db.o 
	$(CXX) $(LDFLAGS) $(LDLIBS) -o $@ $^

#testobjects := $(patsubst %.cpp,%.o,$(wildcard googletest/*.cpp))

tests: mobs.a googletest/testObjgen.o
	$(CXX) $(LDFLAGS) $(LDLIBS) -lgtest_main  -lgtest -o $@ $^
# -lpthread

doc: 
	doxygen doxygen.conf

