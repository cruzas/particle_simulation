/**
* Authors: Samuel A. Cruz Alegría, Alessandra M. de Felice, Hrishikesh R. Gupta.
*
* This program simulates particle movement in 3D space.
*/

// System header files.
#include <time.h>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <string>
#include <math.h>
#include <stdlib.h>

// User defined header files.
#include "particles.h"
#include "utils.h"

// User defined macros.
#define DEBUGGING 1
#define DEFAULT_NPART 1000
#define DEFAULT_NSTEPS 1000
#define DEFAULT_WIDTH 1024
#define DEFAULT_HEIGHT 512
#define DEFAULT_DEPTH 512
#define DEFAULT_DELTA_T 1e2

// Namespaces.
using namespace std;
using namespace std::chrono; // For timing.

static int write_all_particle_details_to_file(string filename);
static const string PDPATH = "./particle_positions/";

static size_t npart = DEFAULT_NPART;
static size_t nsteps = DEFAULT_NSTEPS;
static float size_x = DEFAULT_WIDTH;
static float size_y = DEFAULT_HEIGHT;
static float size_z = DEFAULT_DEPTH;

static float center_x = size_x/3.0;
static float center_y = size_y/3.0;
static float center_z = size_z/3.0;

static float scale_x = size_x;
static float scale_y = size_y;
static float scale_z = size_z;

static float delta_t = DEFAULT_DELTA_T;
static float scale_mass = 1.0e6;
static float G = 6.67384e-11;

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

/*
* Print expected usage of this program.
*/
void
print_usage()
{
  cerr << "Usage: [width=box_width] "
  << "[height=box_height] "
  << "[depth=box_depth] "
  << "[npart=number_of_particles] "
  << "[delta_t=inter_frame_interval_in_seconds] "
  << "[nsteps=number_of_steps]\n";
}

int main(int argc, char *argv[]) {

  // Do all necessary initializations.
  if (!init_params(argc, argv) || !init_particles()) {
    return -1;
  }

  auto avg_cpu_time = 0;
  for(size_t i = 0; i < nsteps; i++) {
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    update_particle_details();
    high_resolution_clock::time_point t2 = high_resolution_clock::now();

    // Add current duration to average, to be later divided by number of cycles,
    // which is the number of times update_particles() is called.
    avg_cpu_time += duration_cast<nanoseconds>( t2 - t1 ).count();

    // Write file with all particle details in current frame.
    string filename("positions_" + to_string(i) + ".vtk");
    if (!write_all_particle_details_to_file(filename)) {
      cerr << "Could not write file: " << filename << "\n";
      return -1;
    }
  }

  // Calculate average duration.
  avg_cpu_time /= nsteps;

  delete [] pxvec;
  delete [] pyvec;
  delete [] pzvec;

  delete [] vxvec;
  delete [] vyvec;
  delete [] vzvec;

  delete [] axvec;
  delete [] ayvec;
  delete [] azvec;

  delete [] massvec;

  return 0;
}

int write_all_particle_details_to_file(string filename)
{
  ofstream myfile;

  myfile.open(PDPATH + filename, ios::out);
  if (myfile.is_open()) {

    myfile << "# vtk DataFile Version 1.0\n";
    myfile << "3D position data\n";

    myfile << "ASCII\n\n";

    myfile << "DATASET POLYDATA\n";
    myfile << "POINTS " << npart << " float\n";

    for (int i = 0; i < npart; ++i) {
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

int init_particles() {
  // TODO: add check for proper memory allocation.

  // Allocate space for particle positions.
  pxvec =  new float[npart];
  pyvec =  new float[npart];
  pzvec =  new float[npart];

  // Allocate space for particle velocities.
  vxvec =  new float[npart];
  vyvec =  new float[npart];
  vzvec =  new float[npart];

  // Allocate space for particle accelerations.
  axvec =  new float[npart];
  ayvec =  new float[npart];
  azvec =  new float[npart];

  // Allocate space for particle masses.
  massvec =  new float[npart];

  // Initialize particle positions.
  for(size_t i = 0; i < npart; i++) {
    pxvec[i] = randu()-0.5;
    pyvec[i] = randu()-0.5;
    pzvec[i] = randu()-0.5;

    pxvec[i] *= scale_x;
    pyvec[i] *= scale_y;
    pzvec[i] *= scale_z;

    pxvec[i] += center_x;
    pyvec[i] += center_y;
    pzvec[i] += center_z;

  }

  // Initialize particle velocities.
  for(size_t i = 0; i < npart; i++) {
    vxvec[i] = 0.0;
    vyvec[i] = 0.0;
    vzvec[i] = 0.0;
  }

  // Initialize particle accelerations.
  for(size_t i = 0; i < npart; i++) {
    axvec[i] = 0.0;
    ayvec[i] = 0.0;
    azvec[i] = 0.0;
  }

  // Initialize particle mass for all particles.
  for(size_t i = 0; i < npart; i++) {
    massvec[i] = randu();
    massvec[i] *= scale_mass;
  }

  return 1;
}


void update_particle_details() {
  for(size_t i = 0; i < npart; i++) {
    float xi = pxvec[i];
    float yi = pyvec[i];
    float zi = pzvec[i];

    float mi = massvec[i];

    axvec[i] = 0.0;
    ayvec[i] = 0.0;
    azvec[i] = 0.0;

    for(size_t j = 0; j < npart; j++) {
      float xj = pxvec[j];
      float yj = pyvec[j];
      float zj = pzvec[j];

      float mj = massvec[j];

      float dx = xj-xi;
      float dy = yj-yi;
      float dz = zj-zi;

      float d = sqrt(dx*dx+dy*dy+dz*dz)+eps;

      float f = G*mi*mj/(d*d);
      float fx = f*(dx/d);
      float fy = f*(dy/d);
      float fz = f*(dz/d);

      axvec[i] += fx/mi;
      ayvec[i] += fy/mi;
      azvec[i] += fz/mi;
    }

    // Update particle velocities.
    vxvec[i] += axvec[i]*delta_t;
    vyvec[i] += ayvec[i]*delta_t;
    vzvec[i] += azvec[i]*delta_t;

    // Update particle positions.
    pxvec[i] += vxvec[i]*delta_t;
    pyvec[i] += vyvec[i]*delta_t;
    pzvec[i] += vzvec[i]*delta_t;
  }
}

/* Initialize default parameters.
* @return 1 on success, 0 on failure.*/
int
init_params(int argc, char *argv[])
{
  // Read and process command-line arguments.
  for (int i = 1; i < argc; ++i) {
    if(!process_arg(argv[i])) {
      cerr << "Invalid argument: " << argv[i] << "\n";
      print_usage();
      return 0;
    }
  }

  #ifdef DEBUGGING
  printf("width=%f\n", size_x);
  printf("height=%f\n", size_y);
  printf("depth=%f\n", size_z);
  printf("npart=%lu\n", npart);
  printf("delta_t=%f\n", delta_t);
  printf("nsteps=%lu\n", nsteps);
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
  return sscanf(arg, "width=%f", &size_x) == 1;

  else if (strstr(arg, "height="))
  return sscanf(arg, "height=%f", &size_y) == 1;

  else if (strstr(arg, "depth="))
  return sscanf(arg, "depth=%f", &size_z) == 1;

  else if (strstr(arg, "npart="))
  return sscanf(arg, "npart=%zu", &npart) == 1;

  else if (strstr(arg, "delta_t="))
  return sscanf(arg, "delta_t=%f", &delta_t) == 1;

  else if (strstr(arg, "nsteps="))
  return sscanf(arg, "nsteps=%zu", &nsteps) == 1;

  // Return 0 if the given command-line parameter was invalid.
  return 0;
}
