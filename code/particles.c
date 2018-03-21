/**
* @author Samuel A. Cruz Alegría
* @version 2017-06-04 (YYYY-MM-DD)
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

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#include <unistd.h>
#else
#include <CL/cl.h>
#endif

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
static const char * kernel_sources[] = { "draw_particles.cl", "update_particles.cl", "trace_particles.cl" };
static const char * kernel_names[] = { "draw_particles_kernel", "update_particles_kernel", "trace_particles_kernel" };

static GdkPixbuf * pixbuf;  // pixel buffer
static float *pd;           // particle details array

// OpenCL variables
cl_int err;
cl_device_id device;
cl_context context;
cl_command_queue queue;
// OpenCL kernels
cl_kernel draw_kernel;
cl_kernel update_kernel;
cl_kernel trace_kernel;
// OpenCL memory objects
cl_mem kernel_pixels; // device input/output pixels
cl_mem kernel_pd;     // device particle details

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
  if (!init_params(argc, argv) || !set_up_opencl_prelim() || !init_particles())
  return -1;

  #ifdef DEBUGGING
    // print all initial particle information
    print_pd_all();
  #endif

  // animate the particles
  animate_particles();

  // release OpenCL memory objects
  if (kernel_pixels)
  clReleaseMemObject(kernel_pixels), kernel_pixels = 0;

  if (kernel_pd)
  clReleaseMemObject(kernel_pd), kernel_pd = 0;

  // release OpenCL kernels
  clReleaseKernel(draw_kernel);
  clReleaseKernel(update_kernel);
  clReleaseKernel(trace_kernel);
  // release OpenCL context
  clReleaseContext(context);
  // release OpenCL queue
  clReleaseCommandQueue(queue);
  // free particle details
  free(pd);
  return 0;
}

/* Animate the particles. */
void
animate_particles() {
  // initial window width
  int width = DEFAULT_WIDTH;
  // initial window height
  int height = DEFAULT_HEIGHT;

  // create new pixel buffer
  pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 0, 8, width, height);
  gtk_init(0, 0);

  // create new window
  GtkWidget * window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  // set window title
  gtk_window_set_title((GtkWindow *)window, "Particle Simulation (GPU)");
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
  int status = trace_particles(user_data);
  status |= draw_particles(user_data);
  return (status |= update_particles(user_data));
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

  // total bytes from pixels
  size_t size_pixels = sizeof(guchar) * width * height * n_channels;
  // total number of global work items in x and y dimensions, respectively
  size_t g_work_size[2] = { width, height };
  // number of dimensions used to specify the global work-items and work-items
  // in the work-group
  size_t work_dim = 2;

  // create buffer object for kernel pixels
  kernel_pixels = clCreateBuffer(context, CL_MEM_READ_WRITE, size_pixels, NULL, &err);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to create pixels buffer on device.\n%s\n",
    util_error_message(err));
    goto cleanup_kernel_pixels;
  }

  // write our pixels into the input kernel_pixels in device memory
  err = clEnqueueWriteBuffer(queue, kernel_pixels, CL_TRUE, 0, size_pixels, pixels, 0, NULL, NULL);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to write input pixels onto the device.\n%s\n",
    util_error_message(err));
    goto cleanup_kernel_pixels;
  }

  // set the arguments to our trace_kernel
  err  = clSetKernelArg(trace_kernel, 0, sizeof(cl_mem),  &kernel_pixels);
  err |= clSetKernelArg(trace_kernel, 2, sizeof(int),     &width);
  err |= clSetKernelArg(trace_kernel, 1, sizeof(int),     &height);
  err |= clSetKernelArg(trace_kernel, 3, sizeof(int),     &row_stride);
  err |= clSetKernelArg(trace_kernel, 4, sizeof(int),     &n_channels);
  err |= clSetKernelArg(trace_kernel, 5, sizeof(float),   &dim_factor);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to set kernel parameters.\n%s\n",
    util_error_message(err));
    goto cleanup_kernel_pixels;
  }

  // execute the kernel over the entire range of the data set
  err = clEnqueueNDRangeKernel(queue, trace_kernel, work_dim, NULL, g_work_size, NULL, 0, NULL, NULL);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to invoke kernel\n%s\n",
    util_error_message(err));
    goto cleanup_kernel_pixels;
  }

  // wait for the command queue to get serviced before reading back results
  clFinish(queue);

  // read the kernel_pixels from the device (blocking)
  err = clEnqueueReadBuffer(queue, kernel_pixels, CL_TRUE, 0, size_pixels, pixels , 0, NULL, NULL);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to read output pixels from device.\n%s\n",
    util_error_message(err));
    goto cleanup_kernel_pixels;
  }

  // draw the updated pixel buffer
  gdk_draw_pixbuf(widget->window, NULL, pixbuf, 0, 0, 0, 0, width, height, GDK_RGB_DITHER_NONE, 0, 0);

  // release OpenCL pixels on success
  clReleaseMemObject(kernel_pixels), kernel_pixels = 0;
  return 1;

  // release OpenCL pixels on error
  cleanup_kernel_pixels:
  clReleaseMemObject(kernel_pixels), kernel_pixels = 0;
  return 0;
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

  // total bytes from particle details
  size_t size_pd = sizeof(*pd) * n * 4;
  // total number of global work items
  size_t g_work_size = n * 4;
  // number of dimensions used to specify the global work-items and work-items
  // in the work-group
  size_t work_dim = 1;

  // create buffer object for kernel particle details
  kernel_pd = clCreateBuffer(context, CL_MEM_READ_WRITE, size_pd, NULL, &err);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to create particle details buffer on device.\n%s\n",
    util_error_message(err));
    goto cleanup_kernel_pd;
  }

  // write our particle details into the input kernel_pd in device memory
  err = clEnqueueWriteBuffer(queue, kernel_pd, CL_TRUE, 0, size_pd, pd, 0, NULL, NULL);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to write input particle details onto the device.\n%s\n",
    util_error_message(err));
    goto cleanup_kernel_pd;
  }

  // set the arguments to our update_kernel
  err |= clSetKernelArg(update_kernel, 0, sizeof(cl_mem),  &kernel_pd);
  err |= clSetKernelArg(update_kernel, 1, sizeof(int),     &n);
  err |= clSetKernelArg(update_kernel, 2, sizeof(int),     &width);
  err |= clSetKernelArg(update_kernel, 3, sizeof(int),     &height);
  err |= clSetKernelArg(update_kernel, 4, sizeof(float),   &radius);
  err |= clSetKernelArg(update_kernel, 5, sizeof(float),   &delta);
  err |= clSetKernelArg(update_kernel, 6, sizeof(float),   &g);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to set kernel parameters.\n%s\n",
    util_error_message(err));
    goto cleanup_kernel_pd;
  }

  // execute the kernel over the entire range of the data set
  err = clEnqueueNDRangeKernel(queue, update_kernel, work_dim, NULL, &g_work_size, NULL, 0, NULL, NULL);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to invoke kernel.\n%s\n",
    util_error_message(err));
    goto cleanup_kernel_pd;
  }

  // wait for the command queue to get serviced before reading back results
  clFinish(queue);

  // read the kernel_pd from the device (blocking)
  err = clEnqueueReadBuffer(queue, kernel_pd, CL_TRUE, 0, size_pd, pd , 0, NULL, NULL);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to read output particle details from device.\n%s\n",
    util_error_message(err));
    goto cleanup_kernel_pd;
  }

  // release OpenCL particle details on success
  clReleaseMemObject(kernel_pd), kernel_pd = 0;
  return 1;

  // release OpenCL particle details on error
  cleanup_kernel_pd:
  clReleaseMemObject(kernel_pd), kernel_pd = 0;
  return 0;
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

  // total bytes from particle details
  size_t size_pd = sizeof(*pd) * n * 4;
  // total bytes from pixels
  size_t size_pixels = sizeof(guchar) * width * height * n_channels;
  // total number of global work items
  size_t g_work_size = n * 4;
  // number of dimensions used to specify the global work-items and work-items
  // in the work-group
  size_t work_dim = 1;

  // create buffer object for kernel pixels
  kernel_pixels = clCreateBuffer(context, CL_MEM_READ_WRITE, size_pixels, NULL, &err);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to create pixels buffer on device.\n%s\n",
    util_error_message(err));
    goto cl_release_mem_objects;
  }

  // write our pixels into the input kernel_pixels in device memory
  err = clEnqueueWriteBuffer(queue, kernel_pixels, CL_TRUE, 0, size_pixels, pixels, 0, NULL, NULL);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to write input pixels onto the device.\n%s\n",
    util_error_message(err));
    goto cl_release_mem_objects;
  }

  // create buffer object for kernel particle details
  kernel_pd = clCreateBuffer(context, CL_MEM_READ_WRITE, size_pd, NULL, &err);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to create particle details buffer on device.\n%s\n",
    util_error_message(err));
    goto cl_release_mem_objects;
  }

  // write our particle details into the input kernel_pd in device memory
  err = clEnqueueWriteBuffer(queue, kernel_pd, CL_TRUE, 0, size_pd, pd, 0, NULL, NULL);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to write input particle details onto the device.\n%s\n",
    util_error_message(err));
    goto cl_release_mem_objects;
  }

  // set the arguments to our draw_kernel
  err  = clSetKernelArg(draw_kernel, 0 , sizeof(cl_mem) , &kernel_pixels);
  err |= clSetKernelArg(draw_kernel, 1 , sizeof(cl_mem),  &kernel_pd);
  err |= clSetKernelArg(draw_kernel, 2 , sizeof(int),     &n);
  err |= clSetKernelArg(draw_kernel, 3 , sizeof(int),     &width);
  err |= clSetKernelArg(draw_kernel, 4 , sizeof(int),     &height);
  err |= clSetKernelArg(draw_kernel, 5 , sizeof(int),     &row_stride);
  err |= clSetKernelArg(draw_kernel, 6 , sizeof(int),     &n_channels);
  err |= clSetKernelArg(draw_kernel, 7 , sizeof(float),   &radius);
  err |= clSetKernelArg(draw_kernel, 8 , sizeof(float),   &delta);
  err |= clSetKernelArg(draw_kernel, 9 , sizeof(float),   &dim_factor);
  err |= clSetKernelArg(draw_kernel, 10, sizeof(float),   &g);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to set kernel parameters.\n%s\n",
    util_error_message(err));
    goto cl_release_mem_objects;
  }

  // execute the kernel over the entire range of the data set
  err = clEnqueueNDRangeKernel(queue, draw_kernel, work_dim, NULL, &g_work_size, NULL, 0, NULL, NULL);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to invoke kernel.\n%s\n",
    util_error_message(err));
    goto cl_release_mem_objects;
  }

  // wait for the command queue to get serviced before reading back results
  clFinish(queue);

  // read the kernel_pixels from the device (blocking)
  err = clEnqueueReadBuffer(queue, kernel_pixels, CL_TRUE, 0, size_pixels, pixels , 0, NULL, NULL);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to read output pixels from device.\n%s\n",
    util_error_message(err));
    goto cl_release_mem_objects;
  }

  // read the kernel_pd from the device (blocking)
  err = clEnqueueReadBuffer(queue, kernel_pd, CL_TRUE, 0, size_pd, pd , 0, NULL, NULL);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to read output particle details from device.\n%s\n",
    util_error_message(err));
    goto cl_release_mem_objects;
  }

  // draw the updated pixel buffer
  gdk_draw_pixbuf(widget->window, NULL, pixbuf, 0, 0, 0, 0, width, height, GDK_RGB_DITHER_NONE, 0, 0);

  // release OpenCL resources on success
  clReleaseMemObject(kernel_pixels), kernel_pixels = 0;
  clReleaseMemObject(kernel_pd), kernel_pd = 0;
  return 1;

  // release OpenCL resources on error
  cl_release_mem_objects:
  clReleaseMemObject(kernel_pixels), kernel_pixels = 0;
  clReleaseMemObject(kernel_pd), kernel_pd = 0;
  return 0;
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

/* Set up OpenCL preliminaries.
* @return 1 on success, 0 on error. */
int
set_up_opencl_prelim(){
  // choose a device
  if (util_choose_device(&device) != 0) {
    printf("Failed to load device.\n");
    return 0;
  }

  // set up OpenCL context
  context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to create context.\n%s\n", util_error_message(err));
    return 0;
  }

  // compile OpenCL draw_kernel
  if (util_compile_kernel(kernel_sources, sizeof(kernel_sources)/sizeof(const char *), kernel_names[0], device, context, &draw_kernel) != 0) {
    clReleaseContext(context);
    printf("Could not compile draw_kernel.\n");
    return 0;
  }

  // compile OpenCL update_kernel
  if (util_compile_kernel(kernel_sources, sizeof(kernel_sources)/sizeof(const char *), kernel_names[1], device, context, &update_kernel) != 0) {
    clReleaseKernel(draw_kernel);
    clReleaseContext(context);
    printf("Could not compile update_kernel.\n");
    return 0;
  }

  // compile OpenCL trace_kernel
  if (util_compile_kernel(kernel_sources, sizeof(kernel_sources)/sizeof(const char *), kernel_names[2], device, context, &trace_kernel) != 0) {
    clReleaseKernel(draw_kernel);
    clReleaseKernel(update_kernel);
    clReleaseContext(context);
    printf("Could not compile trace_kernel.\n");
    return 0;
  }

  // set up device queue
  queue = clCreateCommandQueue(context, device, 0, &err);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "Failed to create command queue.\n%s\n",
    util_error_message(err));
    clReleaseKernel(draw_kernel);
    clReleaseKernel(update_kernel);
    clReleaseKernel(trace_kernel);
    clReleaseContext(context);
    return 0;
  }

  return 1;
}

/*
* Initialize all particle details.
* @return 1 on success, 0 on error.
*/
int
init_particles()
{
  pd = malloc(sizeof(*pd) * n * 4);
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

    fprintf(stdout, "v_x=%f\n", pd[vx_i]);
    fprintf(stdout, "v_y=%f\n", pd[vy_i]);

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
