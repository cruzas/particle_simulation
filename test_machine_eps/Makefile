# Modified from example found at the following link:
# https://stackoverflow.com/questions/2481269/how-to-make-a-simple-c-makefile
CC=pgcc
CXX=pgc++
RM=rm -f
CPPFLAGS=-g -std=c++11 $(shell pkg-config --cflags)
LDFLAGS = -std=c++11 -L/cluster_nfs/scratch/clutest/cluster_nfs/Data_Apps/apps/gcc/gcc-6.1.0/lib64

SRCS=test.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

PROGS=test

all: test

test:
	$(CXX) $(LDFLAGS) -o test $(SRCS)

#depend: .depend

# TODO: fix this for the cluster.
#.depend: $(SRCS)
#	$(RM) ./.depend
#	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)
	$(RM) $(PROGS)

distclean: clean
	$(RM) *~ .depend

#include .depend
