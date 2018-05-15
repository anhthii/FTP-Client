CXX = g++
CXXFLAGS = -c -g -std=c++14 

.PHONY: libs all
all: cli_exec

cli_exec: libs main.o
	$(CXX) $(wildcard builds/*.o) -o  ftp.out; \

main.o: main.cc
	$(CXX) $(CXXFLAGS) -Ilibs main.cc -o builds/main.o

libs:
	cd libs; \
	$(CXX) $(CXXFLAGS) *.cc; \
	mv *.o ../builds; \

clean:
	rm -rf builds/ cli.out
