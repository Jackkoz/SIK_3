TARGETS	= client server
CXX	= g++
CXXFLAGS= -std=c++0x -I ~/boost_1_55_0/ -lboost_system -lboost_program_options

all: $(TARGETS) 

client: client.o 
	$(CXX) $(CXXFLAGS) $^ -o $@ -lpthread

server: server.cpp mixer.cpp 
	$(CXX) $(CXXFLAGS) $^ -o $@ -lpthread

.PHONY: clean TARGET
clean:
	rm -f server client *.o *~ *.bak
