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

#define DEFAULT_WIDTH 1600
#define DEFAULT_HEIGHT 1600
#define DEFAULT_DEPTH 1600

#define DEBUGGING 1
#undef DEBUGGING

static const string PDPATH = "./particle_positions/";  // Path to write files to.
static const float center_x = DEFAULT_WIDTH/2;
static const float center_y = DEFAULT_HEIGHT/2;
static const float scale_x = DEFAULT_WIDTH;
static const float scale_y = DEFAULT_HEIGHT;
static const float scale_mass = 1.0e6;
static const float G = 6.67384e-11;

static int width;         // Width of box containing particles.
static int height;        // Height of box containing particles.
static int depth;         // Depth of box containing particles.
static int n;             // Number of particles.
static float radius;      // Radius of the particles, in pixels.
static float delta;       // Time, in seconds, for inter-frame interval.
static int total_time_interval;    // Time, in seconds, for total time interval.
static float g;           // Gravitational factor (in y direction).

static float * pxvec;      // Vector of particle x positions.
static float * pyvec;      // Vector of particle y positions.
static float * pzvec;      // Vector of particle z positions.

static float * vxvec;      // Vector of particle velocity x components.
static float * vyvec;      // Vector of particle velocity y components.
static float * vzvec;      // Vector of particle velocity z components.

static float * axvec;      // Vector of particle acceleration x components.
static float * ayvec;      // Vector of particle acceleration y components.
static float * azvec;      // Vector of particle acceleration z components.

static float * massvec;    // Vector of particle masses.

void print_all_particle_details();
int write_all_particle_details_to_file(string filename);

/*
* Print expected usage of this program.
*/
void
print_usage()
{
  cerr << "Usage: [width=box_width] "
       << "[height=box_height] "
       << "[n=num_particles] "
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

  auto avg_cpu_time = 0;

  // Calculate number of loop cycles to be performed given a total time interval
  // and time per frame.
  int nCycles = total_time_interval / delta;

  for (int cycle = 0; cycle < nCycles; ++cycle) {
    string filename ("positions_" + to_string(cycle) + ".vtk");

    // Call function to update particle details and time it.
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    update_particles();
    high_resolution_clock::time_point t2 = high_resolution_clock::now();

    // Add current duration to average, to be later divided by number of cycles,
    // which is the number of times update_particles() is called.
    avg_cpu_time += duration_cast<nanoseconds>( t2 - t1 ).count();

    if (!write_all_particle_details_to_file(filename)) {
      cerr << "Could not write file: " << filename << "\n";
      return -1;
    }
  }

  // Calculate average duration.
  avg_cpu_time /= nCycles;
  cout << "avg_cpu_time for update_particles() in ns=" << avg_cpu_time << "\n";

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
  pxvec = new float[n];
  pyvec = new float[n];
  pzvec = new float[n];

  // Allocate space for particle velocities.
  vxvec = new float[n];
  vyvec = new float[n];
  vzvec = new float[n];

  // Allocate space for particle accelerations.
  axvec = new float[n];
  ayvec = new float[n];
  azvec = new float[n];

  // Allocate space for particle masses.
  massvec = new float[n];

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

// Go through all particles and initialize their details.
#pragma acc parallel loop copy(pxvec[0:n]) copy(pyvec[0:n]) copy(pzvec[0:n]) \
copy(vxvec[0:n]) copy(vyvec[0:n]) copy(vzvec[0:n]) copy(axvec[0:n]) \
copy(ayvec[0:n]) copy(azvec[0:n]) copy(massvec[0:n]) copy(randnums[0:n*6])
  for (int id = 0; id < n; ++id) {
    // Set x, y, and z positions, respectively.
    pxvec[id] = (float) (randnums[id*6] % DEFAULT_WIDTH);
    pyvec[id] = (float) (randnums[id*6 + 1] % DEFAULT_HEIGHT);
    // pzvec[id] = (float) (randnums[id*6 + 2] % DEFAULT_DEPTH);
    pzvec[id] = 0.0;

    pxvec[id] *= scale_x;
    pyvec[id] *= scale_y;
    pxvec[id] += center_x;
    pyvec[id] += center_y;

    // Set velocity x,y, and z components, respectively.
    vxvec[id] = 0.0;
    vyvec[id] = 0.0;
    vzvec[id] = 0.0;

    // Set acceleration x,y, and z components, respectively.
    axvec[id] = 0.0;
    ayvec[id] = 0.0;
    azvec[id] = 0.0;

    // Set particle mass.
    massvec[id] = randnums[id*6] * scale_mass;

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

    // TODO: fix this to say it describes 3D position data.
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
  width = DEFAULT_WIDTH;    // Width of box containing particles.
  height = DEFAULT_HEIGHT;  // Height of box containing particles.
  depth = DEFAULT_DEPTH;    // Depth of box containing particles.
  n = 5;                    // Number of particles.
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
  printf("width=%d\n", width);
  printf("height=%d\n", height);
  printf("n=%d\n", n);
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
  if (strstr(arg, "width="))
  return sscanf(arg, "width=%d", &width) == 1;

  else if (strstr(arg, "height="))
  return sscanf(arg, "height=%d", &height) == 1;

  else if (strstr(arg, "n="))
  return sscanf(arg, "n=%d", &n) == 1;

  else if (strstr(arg, "radius="))
  return sscanf(arg, "radius=%f", &radius) == 1;

  else if (strstr(arg, "delta="))
  return sscanf(arg, "delta=%f", &delta) == 1;

  else if (strstr(arg, "total_time_interval="))
  return sscanf(arg, "total_time_interval=%d", &total_time_interval) == 1;

  // Return 0 if the given command-line parameter was invalid.
  return 0;
}
