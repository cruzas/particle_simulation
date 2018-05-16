CC= g++
COPTS= -Wall -O3

NVCC=nvcc
NVOPTS=-O3 -arch sm_20 -use_fast_math

all: main

main: main.o utils.o
	$(CC) main.o utils.o -o main
		$(COPTS) $(NVLIBS)

main.o: main.cpp
	$(CC) -c main.cpp $(COPTS)

utils.o: utils.cpp utils.h
	$(CC) -c utils.cpp $(COPTS)


clean:
	rm -f *.o main
