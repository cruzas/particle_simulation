#include <iostream>
#include "particles.h"

using namespace std;

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 800
#define DEBUGGING 1
// #undef DEBUGGING

static int width;         // Width of box containing particles.
static int height;        // Height of box containing particles.
static int n;             // Number of particles.
static float fx;          // Horizontal component of the force field.
static float fy;          // Vertical component of the force field.
static float radius;      // Radius of the particles, in pixels.
static float delta;       // Time, in seconds, for inter-frame interval.
static int time_frame;    // Time, in seconds, for total time interval.
static float g;           // Gravitational factor (in y direction).
static float *pd;         // Particle details array.

/*
* Print expected usage of this program.
*/
void
print_usage()
{
  fprintf(stderr, "Usage: [n=num_particles] [fx=forcefield_x] [fy=forcefield_y] [trace=shading_factor_trace] [radius=particle_radius] [delta=inter_frame_interval_in_seconds]\n");
}

/* Print the details of all particles. */
void
print_all_particle_details()
{
  for (int i = 0; i < n; ++i) {
    printf("particles[%d]. px=%f, py=%f, vx=%f, vy=%f\n",
            i, pd[i*4], pd[i*4 + 1], pd[i*4 + 2], pd[i*4 + 3]);
  }
}


// main() is where program execution begins.
int
main(int argc, char *argv[])
{
  #ifdef DEBUGGING
  cout << "Width=" << DEFAULT_WIDTH << "\n";
  cout << "Height=" << DEFAULT_WIDTH << "\n";
  #endif

  // Do all necessary initializations.
  if (!init_params(argc, argv) || !init_particles()) {
    return -1;
  }

  // Calculate number of loop cycles to be performed given a time interval and
  // time per frame.
  int nCycles = time_frame / delta;
  #ifdef DEBUGGING
  printf("nCycles=%d\n", nCycles);
  #endif
  for (int cycle = 0; cycle < nCycles; ++cycle) {
    update_particles();
    print_all_particle_details();
  }
}

/*
* Initialize all particle details.
* @return 1 on success, 0 on error.
*/
int
init_particles()
{
  // Allocate space for particle details.
  pd = (float *)malloc(sizeof(*pd) * n * 4);
  if (!pd) {
    fprintf(stderr, "Could not allocate space for particle details.\n");
    return 0;
  }

  // Go through all particles and initialize their details at random.
  for (int id = 0; id < n; ++id) {
    int px_i = id*4;      // x-position index.
    int py_i = id*4 + 1;  // y-position index.
    int vx_i = id*4 + 2;  // vx-component index.
    int vy_i = id*4 + 3;  // vy-component index.

    pd[px_i] = (double) (rand() % DEFAULT_WIDTH);   // Set x position.
    pd[py_i] = (double) (rand() % DEFAULT_HEIGHT);  // Set y position.
    pd[vx_i] = rand() / (float) RAND_MAX * fx;      // Set vx component.
    pd[vy_i] = rand() / (float) RAND_MAX * fy;      // Set vy component.

    // Correct starting x position.
    if (pd[px_i] - radius <= 0) {
      pd[px_i] = radius;
    } else if (pd[px_i] + radius >= DEFAULT_WIDTH) {
      pd[px_i] = DEFAULT_WIDTH - radius;
    }

    // Correct starting y position.
    if (pd[py_i] - radius <= 0) {
      pd[py_i] = radius;
    } else if (pd[py_i] + radius >= DEFAULT_HEIGHT) {
      pd[py_i] = DEFAULT_HEIGHT - radius;
    }
  }

  #ifdef DEBUGGING
  // Set position of first particle at bottom left corner.
  pd[0] = radius;
  pd[1] = DEFAULT_HEIGHT - radius;
  #endif

  return 1;
}

/*
* Update the particle details.
* @return 1 on success, 0 on error.
*/
int
update_particles()
{
  // Loop through all particles
  for (int id = 0; id < n; ++id) {
    int px_i = id*4;      // x-position index.
    int py_i = id*4 + 1;  // y-position index.
    int vx_i = id*4 + 2;  // vx-component index.
    int vy_i = id*4 + 3;  // vy-component index.

    float px = pd[px_i];  // x position.
    float py = pd[py_i];  // y position.
    float vx = pd[vx_i];  // vx component.
    float vy = pd[vy_i];  // vy component.

    // Set new x direction based on horizontal collision with wall.
    if (px + radius >= width || px - radius <= 0) {
      vx = vx * -1;
    }

    // Set new y direction based on vertical collision with wall.
    if (py - radius <= 0 || py + radius >= height) {
      vy = vy * -1;
    }

    // Update x and y positions.
    pd[px_i] = px + (vx*delta);
    pd[py_i] = py - (vy*delta) - (0.5 * g * delta * delta);
    // Update vx and vy components.
    pd[vx_i] = vx;  // vx only as acceleration in x-direction is 0.
    pd[vy_i] = vy + 0.5*(g)*delta;

    // Correct x position in case updated position goes past wall.
    if (pd[px_i] - radius <= 0) {
      pd[px_i] = radius;
    } else if (pd[px_i] + radius >= height) {
      pd[px_i] = width - radius;
    }

    // Correct y position in case updated position goes past wall.
    if (pd[py_i] - radius <= 0) {
      pd[py_i] = radius;
    } else if (pd[py_i] + radius >= height) {
      pd[py_i] = height - radius;
    }
  }

  return 1;
}

/* Initialize default parameters.
* @return 1 on success, 0 on failure.*/
int
init_params(int argc, char *argv[])
{

  width = DEFAULT_WIDTH;    // Width of box containing particles.
  height = DEFAULT_HEIGHT;  // Height of box containing particles.
  n = 5;                    // Number of particles.
  fx = 50;                  // Horizontal component of the force field.
  fy = 50;                  // Vertical component of the force field.
  radius = 5;               // Radius of the particles, in pixels.
  delta = 1.0;              // Time, in seconds, for inter-frame interval.
  time_frame = 10;          // Time, in seconds, for total time interval.
  g = -9.8;                 // Gravitational factor (in y direction).

  // Read and process command-line arguments.
  for (int i = 1; i < argc; ++i) {
    if(!process_arg(argv[i])) {
      fprintf(stderr, "Invalid argument: %s\n", argv[i]);
      print_usage();
      return 0;
    }
  }

  #ifdef DEBUGGING
  printf("width=%d\n", width);
  printf("height=%d\n", height);
  printf("n=%d\n", n);
  printf("fx=%f\n", fx);
  printf("fy=%f\n", fy);
  printf("radius=%f\n", radius);
  printf("delta=%f\n", delta);
  printf("time_frame=%d\n", time_frame);
  printf("g: %f\n", g);
  #endif

  return 1;
}


/*
* Process the given command-line parameter.
* @param arg The command-line parameter.
* @return 1 on success, 0 on error.*/
int
process_arg(char *arg)
{
  if (strstr(arg, "n="))
  return sscanf(arg, "n=%d", &n) == 1;

  else if (strstr(arg, "fx="))
  return sscanf(arg, "fx=%f", &fx) == 1;

  else if (strstr(arg, "fy="))
  return sscanf(arg, "fy=%f", &fy) == 1;

  else if (strstr(arg, "radius="))
  return sscanf(arg, "radius=%f", &radius) == 1;

  else if (strstr(arg, "delta="))
  return sscanf(arg, "delta=%f", &delta) == 1;

  // Return 0 if the given command-line parameter was invalid.
  return 0;
}
