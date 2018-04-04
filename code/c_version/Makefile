GTK_PACKAGES=gdk-pixbuf-2.0 gtk+-2.0
GTK_CFLAGS=$(shell pkg-config --cflags $(GTK_PACKAGES))
# -I/usr/local/Cellar/gtk+/2.24.32_2/include/gtk-2.0 -I/usr/local/Cellar/gdk-pixbuf/2.36.11/include/gdk-pixbuf-2.0/ -I/usr/local/Cellar/glib/2.56.0/include/glib-2.0/ -I/usr/local/Cellar/glib/2.56.0/lib/glib-2.0/include/ -I/usr/local/Cellar/cairo/1.14.12/include/cairo/ -I/usr/local/Cellar/pango/1.42.0_1/include/pango-1.0/ -I/usr/local/Cellar/gtk+/2.24.32_2/lib/gtk-2.0/include/ -I/usr/local/Cellar/atk/2.28.1_1/include/atk-1.0/

GTK_LIBS=$(shell pkg-config --libs $(GTK_PACKAGES))
# -L/usr/local/Cellar/gtk+/2.24.32_2/lib -lgtk-quartz-2.0.0 -lgdk-quartz-2.0.0


CFLAGS=-g -fsanitize=address -Wall -O2 -std=c99 -framework OpenCL $(GTK_CFLAGS)
LIBS=$(GTK_LIBS)

PROGS=particles_parallel particles_serial

.PHONY: all
all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $*.c $(LIBS) -o $@

particles_parallel: particles_parallel.o opencl_util.o
	$(CC) $(CFLAGS) $(LDFLAGS) particles_parallel.o opencl_util.o $(LIBS) -o $@

particles_serial: particles_serial.o
	$(CC) $(CFLAGS) $(LDFLAGS) particles_serial.o $(LIBS) -o $@

.PHONY: clean
clean:
	rm -f *.o $(PROGS)
