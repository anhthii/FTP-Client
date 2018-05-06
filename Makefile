CXX = g++
CXXFLAGS = -c -g -pthread -std=c++11 

.PHONY: libs all
all: cli_exec

cli_exec: libs main.o
	g++ -pthread $(wildcard builds/*.o) -o  cli.out; \

main.o: main.cc
	$(CXX) $(CXXFLAGS) -Ilibs main.cc -o builds/main.o

libs:
	cd libs; \
	$(CXX) $(CXXFLAGS) *.cc; \
	mv *.o ../builds; \

clean:
	rm -rf builds/ cli.out
