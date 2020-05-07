# :vim noet

#CXX     = clang++

TARGETS		= gtests db
LIB		= libmobs.a

# Includepfade für googletest
CPPFLAGS	= -I/usr/local/include -I.
LDFLAGS		= -L. -L/usr/local/lib
CXXFLAGS	= -std=c++11 -g -Wall
LDLIBS		= -lpthread -lmobs
#ARFLAGS	= -crs

all::	$(TARGETS)
	

testobjects := $(patsubst %.cpp,%.o,$(wildcard googletest/*.cpp))

clean::
	$(RM) *.o $(TARGETS) $(LIB) $(testobjects)
	$(RM) -r doc

$(LIB): objgen.o objtypes.o logging.o strtoobj.o objpool.o xmlwriter.o xmlout.o xmlread.o converter.o unixtime.o 
	$(AR) $(ARFLAGS) $@ $^


db: db.o $(LIB)
	#$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@ 
	$(CXX) $(LDFLAGS) $(LDLIBS) -o $@ $<
	install -d data
	echo '{\n  "id": 2,\n  "typ": "Rollschuh",\n  "achsen": 2\n}\n' >data/Fahrzeug.2

gtests: $(LIB) $(testobjects)
	$(CXX) $(LDFLAGS) $(testobjects) $(LDLIBS) -lgtest -lgtest_main -o $@ 
	./gtests

doc:
	doxygen doxygen.conf


