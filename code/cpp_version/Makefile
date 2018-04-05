# Modified from example found at the following link:
# https://stackoverflow.com/questions/2481269/how-to-make-a-simple-c-makefile
CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-g -fsanitize=address $(shell pkg-config --cflags)
LDFLAGS= -fsanitize=address

SRCS=particles_serial.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: particles_serial

particles_serial: $(OBJS)
	$(CXX) $(LDFLAGS) -o particles_serial $(OBJS)

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

include .depend
