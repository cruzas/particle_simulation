/**
* Authors: Samuel A. Cruz Alegría, Alessandra M. de Felice, Hrishikesh R. Gupta.
*
* This program simulates particle movement in 2D space.
*/

#include <algorithm>
#include <ctime>
#include <cstring>
#include <chrono>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */
#include <vector>

#include "particles.h"

using namespace std;
using namespace std::chrono; // For timing.

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 800
#define DEFAULT_DEPTH 800

#define DEBUGGING 1
#undef DEBUGGING

static const string PDPATH = "./particle_positions/";  // Path to write files to.
static int use_openacc;     // Whether we use OpenACC or not.
static int width;         // Width of box containing particles.
static int height;        // Height of box containing particles.
static int depth;         // Depth of box containing particles.
static int n;             // Number of particles.
static float fx;          // Horizontal component of the force field.
static float fy;          // Vertical component of the force field.
static float fz;          // Depth component of the force field.
static float radius;      // Radius of the particles, in pixels.
static float delta;       // Time, in seconds, for inter-frame interval.
static int total_time_interval;    // Time, in seconds, for total time interval.
static float g;           // Gravitational factor (in y direction).

float * pxvec;      // Vector of particle x positions.
float * pyvec;      // Vector of particle y positions.
float * pzvec;      // Vector of particle z positions.

float * vxvec;      // Vector of particle velocity x components.
float * vyvec;      // Vectory of particle velocity y components.
float * vzvec;      // Vector of particle velocity z components.

void print_all_particle_details();
int write_all_particle_details_to_file(string filename);

/*
* Print expected usage of this program.
*/
void
print_usage()
{
  cerr << "Usage: << [use_openacc=1 or 0] "
       << "[width=box_width] "
       << "[height=box_height] "
       << "[n=num_particles] "
       << "[fx=forcefield_x] "
       << "[fy=forcefield_y] "
       << "[fz=forcefield_z]"
       << "[trace=shading_factor_trace] "
       << "[radius=particle_radius] "
       << "[delta=inter_frame_interval_in_seconds] "
       << "[total_time_interval=total_time_in_seconds]\n";
}


// main() is where program execution begins.
int
main(int argc, char *argv[])
{
  // Do all necessary initializations.
  if (!init_params(argc, argv) || !init_particles()) {
    return -1;
  }

  // double avg_cpu_time_used = 0;
  struct timespec t1;
  struct timespec t2;

  double avg_cpu_time = 0;

  // Calculate number of loop cycles to be performed given a total time interval
  // and time per frame.
  int nCycles = total_time_interval / delta;

  for (int cycle = 0; cycle < nCycles; ++cycle) {
    string filename ("positions_" + to_string(cycle) + ".vtk");

    // Call function to update particle details and time it.
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
    update_particles();
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t2);

    // Add current duration to average, to be later divided by number of cycles,
    // which is the number of times update_particles() is called.
    avg_cpu_time += t2.tv_nsec-t1.tv_nsec ;

    if (!write_all_particle_details_to_file(filename)) {
      cerr << "Could not write file: " << filename << "\n";
      return -1;
    }
  }

  // Calculate average duration.
  avg_cpu_time /= nCycles;
  cout << "avg_cpu_time (ns)=" << avg_cpu_time << "\n";


  delete [] pxvec;
  delete [] pyvec;
  delete [] pzvec;

  delete [] vxvec;
  delete [] vyvec;
  delete [] vzvec;

  return 0;
}

/*
* Initialize all particle details.
* @return 1 on success, 0 on error.
*/
int
init_particles()
{
  // Allocate space for particle positions.
  // pxvec.reserve(n);
  // pyvec.reserve(n);
  // pzvec.reserve(n);

  pxvec = new float[n];
  pyvec = new float[n];
  pzvec = new float[n];

  // Allocate space for particle velocities.
  // vxvec.reserve(n);
  // vyvec.reserve(n);
  // vzvec.reserve(n);
  vxvec = new float[n];
  vyvec = new float[n];
  vzvec = new float[n];

  /* The srand() function sets its argument seed as the seed for a new
   * sequence of pseudo-random numbers to be returned by rand().  These
   * sequences are repeatable by calling srand() with the same seed value.
   *
   *
   * time(0) explanation from: https://stackoverflow.com/questions/4736485/srandtime0-and-random-number-generation
   * time(0) gives the time in seconds since the Unix epoch, which is a
   * pretty good "unpredictable" seed (you're guaranteed your seed will be the
   * same only once, unless you start your program multiple times within the
   * same second).*/
  srand(time(0));

// Create vector of random numbers.
// REVIEW: Do fix this. This is only done for now since we can't use rand() call in OpenACC.
int * randnums = new int[n*6];
for (int i = 0; i < n*6; ++i) {
  randnums[i] = rand();
}

  // Go through all particles and initialize their details at random.
//#pragma acc kernels
#pragma acc parallel loop copy(pxvec[0:n]) copy(pyvec[0:n]) copy(pzvec[0:n]) \
copy(vxvec[0:n]) copy(vyvec[0:n]) copy(vzvec[0:n]) copy(randnums[0:n*6])
  for (int id = 0; id < n; ++id) {
    // Set x position.
    pxvec[id] = (float) (randnums[id*6] % DEFAULT_WIDTH);
    // Set y position.
    pyvec[id] = (float) (randnums[id*6 + 1] % DEFAULT_HEIGHT);
    // Set z position.
    pzvec[id] = (float) (randnums[id*6 + 2] % DEFAULT_DEPTH);
    // Set velocity x-component.
    vxvec[id] = randnums[id*6 + 3] / (float) RAND_MAX * fx;
    // Set velocity y-component.
    vyvec[id] = randnums[id*6 + 4] / (float) RAND_MAX * fy;
    // Set velocity z-component.
    vzvec[id] = randnums[id*6 + 5] / (float) RAND_MAX * fz;

    // Correct starting x position.
    if (pxvec[id] - radius <= 0) {
      pxvec[id] = radius;
    } else if (pxvec[id] + radius >= DEFAULT_WIDTH) {
      pxvec[id] = DEFAULT_WIDTH - radius;
    }

    // Correct starting y position.
    if (pyvec[id] - radius <= 0) {
      pyvec[id] = radius;
    } else if (pyvec[id] + radius >= DEFAULT_HEIGHT) {
      pyvec[id] = DEFAULT_HEIGHT - radius;
    }

    // Correct starting z position.
    if (pzvec[id] - radius <= 0) {
      pzvec[id] = radius;
    } else if (pzvec[id] + radius >= DEFAULT_DEPTH) {
      pzvec[id] = DEFAULT_DEPTH - radius;
    }
  }

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
#pragma acc parallel loop copy(pxvec[0:n]) copy(pyvec[0:n]) copy(pzvec[0:n]) \
copy(vxvec[0:n]) copy(vyvec[0:n]) copy(vzvec[0:n])
for (int id = 0; id < n; ++id) {
    float px = pxvec[id];  // x position.
    float py = pyvec[id];  // y position.
    float pz = pzvec[id];  // z position.
    float vx = vxvec[id];  // velocity x-component.
    float vy = vyvec[id];  // velocity y-component.
    float vz = vzvec[id];  // velocity z-component.

    // Set new x direction based on horizontal collision with wall.
    if (px + radius >= width || px - radius <= 0) {
      vx = vx * -1;
    }

    // Set new y direction based on vertical collision with wall.
    if (py - radius <= 0 || py + radius >= height) {
      vy = vy * -1;
    }

    // Set new z direction based on depth collision with wall.
    if (pz + radius >= depth || pz - radius <= 0) {
      vz = vz * -1;
    }

    // Update particle position.
    pxvec[id] = px + (vx*delta);
    pyvec[id] = py - (vy*delta) - (0.5 * g * delta * delta);
    pzvec[id] = pz + (vz*delta);

    // Update velocity components.
    vxvec[id] = vx;  // vx only as acceleration in x-direction is 0.
    vyvec[id] = vy + 0.5*(g)*delta;
    vzvec[id] = vz; // vz only as acceleration in z-direction is 0.

    // Correct x position in case updated position goes past wall.
    if (pxvec[id] - radius <= 0) {
      pxvec[id] = radius;
    } else if (pxvec[id] + radius >= height) {
      pxvec[id] = width - radius;
    }

    // Correct y position in case updated position goes past wall.
    if (pyvec[id] - radius <= 0) {
      pyvec[id] = radius;
    } else if (pyvec[id] + radius >= height) {
      pyvec[id] = height - radius;
    }

    // Correct z position in case updated position goes past wall.
    if (pzvec[id] - radius <= 0) {
      pzvec[id] = radius;
    } else if (pzvec[id] + radius >= depth) {
      pzvec[id] = depth - radius;
    }
  }

  return 1;
}

/* Print the details of all particles.*/
void
print_all_particle_details()
{
  for (int i = 0; i < n; ++i) {
    cout << "particles" << i
    << ": "
    << "px=" << pxvec[i] << ", "
    << "py=" << pyvec[i] << ", "
    << "pz=" << pzvec[i] << ", "
    << "vx=" << vxvec[i] << ", "
    << "vy=" << vyvec[i] << ", "
    << "vz=" << vzvec[i] << "\n";
  }
}

/* Write the details of all particles to file with given filename.
* The path PDPATH is used to specify where to save the file.
* @return 1 on success, 0 on failure.*/
int
write_all_particle_details_to_file(string filename)
{
  ofstream myfile;

  myfile.open(PDPATH + filename, ios::out);
  if (myfile.is_open()) {

    myfile << "# vtk DataFile Version 1.0\n";
    myfile << "3D triangulation data\n";
    myfile << "ASCII\n\n";

    myfile << "DATASET POLYDATA\n";
    myfile << "POINTS " << n << " float\n";

    for (int i = 0; i < n; ++i) {
      // Write particle i's position to file.
      myfile << pxvec[i] << " " << pyvec[i] << " " << pzvec[i] << "\n";
    }

    myfile.close();
  } else {
    cerr << "Unable to open file: " << filename << "\n";
    return 0;
  }

  return 1;
}

/* Initialize default parameters.
* @return 1 on success, 0 on failure.*/
int
init_params(int argc, char *argv[])
{

  use_openacc = 0;          // By default, do not use OpenACC.
  width = DEFAULT_WIDTH;    // Width of box containing particles.
  height = DEFAULT_HEIGHT;  // Height of box containing particles.
  depth = DEFAULT_DEPTH;    // Depth of box containing particles.
  n = 5;                    // Number of particles.
  fx = 50;                  // Horizontal component of the force field.
  fy = 50;                  // Vertical component of the force field.
  fz = 50;                  // Depth component of the force field.
  radius = 5;               // Radius of the particles, in pixels.
  delta = 1.0;              // Time, in seconds, for inter-frame interval.
  total_time_interval = 10; // Time, in seconds, for total time interval.
  g = -9.8;                 // Gravitational factor (in y direction).


  // Read and process command-line arguments.
  for (int i = 1; i < argc; ++i) {
    if(!process_arg(argv[i])) {
      cerr << "Invalid argument: " << argv[i] << "\n";
      print_usage();
      return 0;
    }
  }

  #ifdef DEBUGGING
  printf("use_openacc=%d\n", use_openacc);
  printf("width=%d\n", width);
  printf("height=%d\n", height);
  printf("n=%d\n", n);
  printf("fx=%f\n", fx);
  printf("fy=%f\n", fy);
  printf("fz=%f\n", fz);
  printf("radius=%f\n", radius);
  printf("delta=%f\n", delta);
  printf("total_time_interval=%d\n", total_time_interval);
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
  if (strstr(arg, "use_openacc="))
  return sscanf(arg, "use_openacc=%d", &use_openacc) == 1;

  else if (strstr(arg, "width="))
  return sscanf(arg, "width=%d", &width) == 1;

  else if (strstr(arg, "height="))
  return sscanf(arg, "height=%d", &height) == 1;

  else if (strstr(arg, "n="))
  return sscanf(arg, "n=%d", &n) == 1;

  else if (strstr(arg, "fx="))
  return sscanf(arg, "fx=%f", &fx) == 1;

  else if (strstr(arg, "fy="))
  return sscanf(arg, "fy=%f", &fy) == 1;

  else if (strstr(arg, "fz="))
  return sscanf(arg, "fz=%f", &fy) == 1;

  else if (strstr(arg, "radius="))
  return sscanf(arg, "radius=%f", &radius) == 1;

  else if (strstr(arg, "delta="))
  return sscanf(arg, "delta=%f", &delta) == 1;

  else if (strstr(arg, "total_time_interval="))
  return sscanf(arg, "total_time_interval=%d", &total_time_interval) == 1;

  // Return 0 if the given command-line parameter was invalid.
  return 0;
}
