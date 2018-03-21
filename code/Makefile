GTK_PACKAGES=gdk-pixbuf-2.0 gtk+-2.0
GTK_CFLAGS=-I/usr/local/Cellar/gtk+/2.24.32_2/include/gtk-2.0 /usr/local/Cellar/gdk-pixbuf/2.36.11/include/gdk-pixbuf-2.0/
# $(shell pkg-config --cflags $(GTK_PACKAGES))
GTK_LIBS=$(shell pkg-config --libs $(GTK_PACKAGES))
#-L/usr/local/Cellar/gtk+/2.24.32_2

CFLAGS=-g -Wall -O2 -std=c99 -framework OpenCL $(GTK_CFLAGS)
LIBS=$(GTK_LIBS)

PROGS=particles

.PHONY: all
all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $*.c $(LIBS) -o $@

particles: particles.o opencl_util.o
	$(CC) $(CFLAGS) $(LDFLAGS) particles.o opencl_util.o $(LIBS) -o $@

.PHONY: clean
clean:
	rm -f *.o $(PROGS)
