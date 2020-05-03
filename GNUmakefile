# :vim noet

#CXX     = clang++

TARGETS		= gtests db

# Includepfade für googletest
CPPFLAGS	= -I/usr/local/include -I.
LDFLAGS		= -L/usr/local/lib
CXXFLAGS	= -std=c++11 -g -Wall
LDLIBS		= 

all::	$(TARGETS)
	

testobjects := $(patsubst %.cpp,%.o,$(wildcard googletest/*.cpp))

clean::
	$(RM) *.o $(TARGETS) $(testobjects)

mobs.a: objgen.o objtypes.o logging.o strtoobj.o objpool.o xmlwriter.o xmlout.o xmlread.o unixtime.o 
	$(AR) -rc $@ $^

# Achtung linkage: objgen.o immer zuerst

db: mobs.a db.o 
	$(CXX) $(LDFLAGS) $(LDLIBS) -o $@ $^

gtests: mobs.a $(testobjects)
	$(CXX) $(LDFLAGS) $(LDLIBS) -lgtest_main  -lgtest -o $@ $^
	./gtests
# -lpthread

doc:
	doxygen doxygen.conf

