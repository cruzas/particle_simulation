/**
* @author Samuel A. Cruz Alegría
* @version 2018-03-24 (YYYY-MM-DD)
*
* This program produces an animation of a number of particles subjected to a
* constant force field. The animation is based on a simulation of n
* ball-shaped particles moving and bouncing around in a 2D space defined by
* the animation window. Particles bounce off walls, but do do not interact
* with each other.
*
* NOTE: You can close the GTK window by pressing 'q' or 'Q'.
*
* NOTE: I give special thanks to Marco Tollini for giving me the idea of
* storing all particle information in a float array.
* A single particle is represented by four consecutive elements in the
* float array. This float array is named pd (see below).
* For instance, for the first particle, pd[0] represents the x-coordinate of
* the particle, pd[1] represents the y-coordinate of the particle, pd[2]
* represents the horizontal component of the the particle's velocity, and pd[3]
* represents the vertical component of the particle's velocity.
*
* Through another suggestion of Marco Tollini's, I also realized that I could
* set the total number of global work items in the draw_particles and
* update_particles kernels to be n*4, where n is the number of particles:
* this allowed for straightforward referencing of the particle details in
* these kernels.
*
* NOTE: Furthermore, the algorithm for drawing the particles is based on
* https://stackoverflow.com/questions/1201200/fast-algorithm-for-drawing-filled-circles/1201227
*
* NOTE: The motion of particles was implemented based on
* https://cg.ivd.kit.edu/downloads/GPGPU_assignment_4.pdf
* A paper which may have given me insight into other issues as well.
*
* REVIEW: The known limitations in this program are:
* 1. The behaviour is not as expected when the window is resized: although
* having a resizable window was not a specification of the assignment.
* 2. The motion of the particles may not be perfectly calculated (please refer
* to update_particles.cl).
* 3. The drawing of the balls may not be perfect (please refer to
* draw_particles.cl). If you look carefully to the left of the window,
* especially when there are many balls (e.g., 20), it seems a few pixels are
* painted when they should not be.
*/

#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include "opencl_util.h"
#include "particles.h"

#define DEFAULT_WIDTH 800   // default WIDTH of the window
#define DEFAULT_HEIGHT 800  // default HEIGHT of the window
#define DEBUGGING 1
#undef DEBUGGING

static int n;             // number of particles
static float fx;          // horizontal component of the force field
static float fy;          // vertical component of the force field
static float radius;      // radius of the particles, in pixels
static float trace;       // shading factor for the trace of a particle
static float delta;       // time, in seconds, inter-frame interval
static float dim_factor;  // 1.0-trace
static float g;           // gravitational factor (in y direction)

static GdkPixbuf * pixbuf;  // pixel buffer
static float *pd;           // particle details array

static void print_usage();
static gint timeout(void * user_data);
static gint draw_particles(void *user_data);
static gint update_particles(void *user_data);
static gint trace_particles(void *user_data);
static void destroy_window(void);
static gint expose_event();
static gint resize_pixbuf(void *user_data);
static gint keyboard_input(GtkWidget *widget, GdkEventKey *event);

void
print_usage()
{
  fprintf(stderr, "Usage: [n=num_particles] [fx=forcefield_x] [fy=forcefield_y] [trace=shading_factor_trace] [radius=particle_radius] [delta=inter_frame_interval_in_seconds]\n");
}

int
main(int argc, char *argv[])
{
  // do all necessary initializations
  if (!init_params(argc, argv) || !init_particles())
    return -1;

  #ifdef DEBUGGING
    // print all initial particle information
    print_pd_all();
  #endif

  // animate the particles
  return animate_particles();
}

/* Animate the particles. */
int
animate_particles() {
  // initial window width
  int width = DEFAULT_WIDTH;
  // initial window height
  int height = DEFAULT_HEIGHT;

  #ifdef DEBUGGING
    printf("width=%d, height=%d\n", width, height);
  #endif

  // create new pixel buffer
  pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 0, 8, width, height);
  if (!pixbuf) {
    fprintf(stderr, "Could not allocate space for image buffer.\n");
    return -1;
  }

  gtk_init(0, 0);

  // create new window
  GtkWidget * window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  // set window title
  gtk_window_set_title((GtkWindow *)window, "Particle Simulation (CPU)");
  // resize window to desired size
  gtk_window_resize(GTK_WINDOW(window), width, height);

  // set up signal for destroying the window
  gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(destroy_window), window);
  // set up signal for exposing event
  gtk_signal_connect(GTK_OBJECT(window), "expose_event", GTK_SIGNAL_FUNC(expose_event), window);
  // set up signal for resizing window
  gtk_signal_connect(GTK_OBJECT(window), "configure_event", GTK_SIGNAL_FUNC(resize_pixbuf), window);
  // set up signal for certain key presses
  gtk_signal_connect(GTK_OBJECT(window), "key_press_event", GTK_SIGNAL_FUNC(keyboard_input), NULL);

  // set event masks for our window
  gtk_widget_set_events (window, GDK_EXPOSURE_MASK);
  // show all widgets on our window
  gtk_widget_show_all(window);

  // periodically call timeout function every delta/1000 milliseconds
  g_timeout_add(delta/1000, timeout, window);
  // gtk_main() runs the main loop until gtk_main_quit() is called
  gtk_main();

  return 0;
}

/* Function called at each delta/1000 milliseconds time interval.
* @param user_data The data to process.
* @return 1 on success, 0 on failure. */
static gint
timeout(void * user_data)
{
  #ifdef DEBUGGING
    print_pd_all(); // print all particle details
  #endif

  // status is 1 on success, 0 on error
  // int status = trace_particles(user_data);
  // status |= draw_particles(user_data);
  return update_particles(user_data);
}

/*
* Draw the trace of the particles.
* @param user_data The data to process.
* @return 1 on success, 0 on error.
*/
static gint
trace_particles(void *user_data)
{
  GtkWidget * widget = user_data;

  // width of pixel buffer
  int width = gdk_pixbuf_get_width(pixbuf);
  // height of pixel buffer
  int height = gdk_pixbuf_get_height(pixbuf);
  // number of bytes between the start of a row and the start of the next row
  int row_stride = gdk_pixbuf_get_rowstride(pixbuf);
  // number of channels
  int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
  // pixel data of the pixel buffer
  guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

  // Multiply each pixel by dim factor to create trace
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      guchar * pixel = pixels + y*row_stride + x*n_channels;
      for(int i = 0; i < n_channels; ++i)
        *pixel++ *= dim_factor;
    }
  }

  // draw the updated pixel buffer
  gdk_draw_pixbuf(widget->window, NULL, pixbuf, 0, 0, 0, 0, width, height, GDK_RGB_DITHER_NONE, 0, 0);

  return 1;
}

/*
* Update the particle details.
* @param user_data The data to process.
* @return 1 on success, 0 on error.
*/
static gint
update_particles(void *user_data)
{
  // width of pixel buffer
  int width = gdk_pixbuf_get_width(pixbuf);
  // height of pixel buffer
  int height = gdk_pixbuf_get_height(pixbuf);

  // Loop through all particles
  for (int i = 0; i < n; ++i) {
    int px_i = i*4;      // x-position index
    int py_i = i*4 + 1;  // y-position index
    int vx_i = i*4 + 2;  // vx-component index
    int vy_i = i*4 + 3;  // vy-component index

    float px = pd[px_i];
    float py = pd[py_i];
    float vx = pd[vx_i];
    float vy = pd[vy_i];

    // set new x direction based on horizontal collision with wall
    if (px + radius >= width || px - radius <= 0)
      vx = vx * -1;

    // set new y direction based on vertical collision with wall
    if (py - radius <= 0 || py + radius >= height)
      vy = vy * -1;

    pd[px_i] = px + (vx*delta);
    pd[py_i] = py - (vy*delta) - (0.5 * g * delta * delta);

    pd[vx_i] = vx;  // vx only as acceleration in x-direction is 0
    pd[vy_i] = vy + 0.5*(g)*delta;

    // correct x position in case updated position goes past wall
    if (pd[px_i] - radius <= 0)
      pd[px_i] = radius;
    else if (pd[px_i] + radius >= height)
      pd[px_i] = width - radius;

    // correct y position in case updated position goes past wall
    if (pd[py_i] - radius <= 0)
      pd[py_i] = radius;
    else if (pd[py_i] + radius >= height)
      pd[py_i] = height - radius;
  }

  return 1;
}

/*
* Draw the ball-shaped particles on the pixel buffer.
* @param user_data The data to process.
* @return 1 on success, 0 on error.
*/
static gint
draw_particles(void *user_data) {
  GtkWidget * widget = user_data;

  // width of pixel buffer
  int width = gdk_pixbuf_get_width(pixbuf);
  // height of pixel buffer
  int height = gdk_pixbuf_get_height(pixbuf);
  // number of bytes between the start of a row and the start of the next row
  int row_stride = gdk_pixbuf_get_rowstride(pixbuf);
  // number of channels
  int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
  // pixel data of the pixel buffer
  guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

  // Loop through all particles
  for (int i = 0; i < n; ++i) {
    int px_i = i*4;      // x-position index
    int py_i = i*4 + 1;  // y-position index

    float px = pd[px_i];
    float py = pd[py_i];

    // draw filled circle
    // based on: https://stackoverflow.com/questions/1201200/fast-algorithm-for-drawing-filled-circles/1201227
    for(int y = -radius; y <= radius; ++y) {
      for(int x = -radius; x <= radius; ++x) {
        if(x*x + y*y <= radius*radius) {
          int yind = (int)(py+y) * row_stride;  // y index
          int xind = (int)(px+x) * n_channels;  // x index
          int pixind = yind + xind; // pixel index

          for (int c = 0; c < n_channels; ++c)
            pixels[pixind + c] = 255;

        }
      }
    }
  }

  // draw the updated pixel buffer
  gdk_draw_pixbuf(widget->window, NULL, pixbuf, 0, 0, 0, 0, width, height, GDK_RGB_DITHER_NONE, 0, 0);

  return 1;
}

/* Initialize default parameters.
* @return 1 on success, 0 on failure. */
int
init_params(int argc, char *argv[])
{
  // default parameters
  n = 5;
  fx = 50;
  fy = 50;
  trace = 0.5;
  radius = 5;
  delta = 1.0;
  g = -9.8;

  // read and process command-line arguments
  for (int i = 1; i < argc; ++i) {
    if(!process_arg(argv[i])) {
      fprintf(stderr, "Invalid argument: %s\n", argv[i]);
      print_usage();
      return 0;
    }
  }

  dim_factor = 1.0-trace;

  #ifdef DEBUGGING
    printf("n=%d\n", n);
    printf("fx=%f\n", fx);
    printf("fy=%f\n", fy);
    printf("trace=%f\n", trace);
    printf("radius=%f\n", radius);
    printf("delta=%f\n", delta);
    printf("dim_factor: %f\n", dim_factor);
    printf("g: %f\n", g);
  #endif

  return 1;
}

/*
* Initialize all particle details.
* @return 1 on success, 0 on error.
*/
int
init_particles()
{
  pd = (float *)malloc(sizeof(*pd) * n * 4);
  if (!pd) {
    fprintf(stderr, "Could not allocate space for particle details.\n");
    return 0;
  }

  // go through all particles and initialize their details at random
  // where sensible
  for (int id = 0; id < n; ++id) {
    int px_i = id*4;      // x-position index
    int py_i = id*4 + 1;  // y-position index
    int vx_i = id*4 + 2;  // vx-component index
    int vy_i = id*4 + 3;  // vy-component index

    pd[px_i] = (double) (rand() % DEFAULT_WIDTH);   // set x position
    pd[py_i] = (double) (rand() % DEFAULT_HEIGHT);  // set y position
    pd[vx_i] = rand() / (float) RAND_MAX * fx;  // set vx component
    pd[vy_i] = rand() / (float) RAND_MAX * fy;  // set vy component

    // fprintf(stdout, "v_x=%f\n", pd[vx_i]);
    // fprintf(stdout, "v_y=%f\n", pd[vy_i]);

    // correct starting x position
    if (pd[px_i] - radius <= 0)
    pd[px_i] = radius;
    else if (pd[px_i] + radius >= DEFAULT_WIDTH)
    pd[px_i] = DEFAULT_WIDTH - radius;

    // correct starting y position
    if (pd[py_i] - radius <= 0)
    pd[py_i] = radius;
    else if (pd[py_i] + radius >= DEFAULT_HEIGHT)
    pd[py_i] = DEFAULT_HEIGHT - radius;
  }

  #ifdef DEBUGGING
    // set position of first particle at bottom left corner
    pd[0] = radius;
    pd[1] = DEFAULT_HEIGHT - radius;
  #endif

  return 1;
}

/* Print the details of all particles. */
void
print_pd_all()
{
  for (int i = 0; i < n; ++i)
  printf("particles[%d]. px=%f, py=%f, vx=%f, vy=%f\n", i, pd[i*4], pd[i*4 + 1], pd[i*4 + 2], pd[i*4 + 3]);
}

/*
* Process the given command-line parameter.
* @param arg The command-line parameter.
* @return 1 on success, 0 on error.
*/
int
process_arg(char *arg)
{
  if (strstr(arg, "n="))
  return sscanf(arg, "n=%d", &n) == 1;

  else if (strstr(arg, "fx="))
  return sscanf(arg, "fx=%f", &fx) == 1;

  else if (strstr(arg, "fy="))
  return sscanf(arg, "fy=%f", &fy) == 1;

  else if (strstr(arg, "trace="))
  return sscanf(arg, "trace=%f", &trace) == 1;

  else if (strstr(arg, "radius="))
  return sscanf(arg, "radius=%f", &radius) == 1;

  else if (strstr(arg, "delta="))
  return sscanf(arg, "delta=%f", &delta) == 1;

  // return 0 if the given command-line parameter was invalid
  return 0;
}

/* Destroy the gtk window. */
static void
destroy_window(void) {
  gtk_main_quit();
}

/* GTK exposure event.
* @param user_data The data to process.
* @return 1 on success, 0 on error. */
static gint expose_event(void *user_data) {
  return timeout(user_data);
}

/* Resize the pixel buffer.
* @param user_data The data to process.
* @return 1 on success, 0 on error. */
static gint resize_pixbuf(void *user_data) {
  GtkWidget * widget = user_data;
  if (pixbuf) {
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);

    if (width == widget->allocation.width && height == widget->allocation.height)
    return 0;
    g_object_unref(pixbuf);
  }

  pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 0, 8, widget->allocation.width, widget->allocation.height);

  return timeout(user_data);
}

/* Handle keyboard input.
 * @param widget The GtkWidget.
 * @param event The event triggered.
 * @return 1 on success, 0 on error. */
static gint
keyboard_input(GtkWidget *widget, GdkEventKey *event) {
  if (event->type != GDK_KEY_PRESS)
  return 0;

  switch(event->keyval) {
    case GDK_KEY_Q:
    case GDK_KEY_q:
    gtk_main_quit();
    break;
    default:
    return 0;
  }

  return 1;
}
