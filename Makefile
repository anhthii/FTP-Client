CXX = g++
CXXFLAGS = -Wall -Wextra -c -g -std=c++14 -lreadline 

.PHONY: libs all
all: cli_exec

cli_exec: libs main.o
	$(CXX) $(wildcard builds/*.o) -lreadline -lncurses -o ftp.out; \

main.o: main.cc
	$(CXX) $(CXXFLAGS) -Ilibs main.cc -o builds/main.o

libs:
	cd libs; \
	$(CXX) $(CXXFLAGS) *.cc; \
	mv *.o ../builds; \

clean:
	rm -rf builds/* ftp.out
