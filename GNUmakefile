# :vim noet

CXX     = clang++

TARGETS		= mom hw db

CPPFLAGS	= -I../rapidjson/include
CXXFLAGS	= -std=c++11 -g
LDLIBS		=
LDFLAGS		= 

all::	$(TARGETS)
	

clean::
	$(RM) *.o $(TARGETS)

# Achtung linkage: objgen.o immer zuerst
mom: objgen.o dumpjson.o readjson.o objpool.o momtest.o
	$(CXX) $(LDFLAGS) $(LDLIBS) -o $@ $^

hw: objpool.o hw.o
	$(CXX) $(LDFLAGS) $(LDLIBS) -o $@ $^

db: objgen.o objpool.o readjson.o dumpjson.o db.o 
	$(CXX) $(LDFLAGS) $(LDLIBS) -o $@ $^
