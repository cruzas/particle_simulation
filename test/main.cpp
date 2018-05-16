/**
* Authors: Samuel A. Cruz Alegría, Alessandra M. de Felice, Hrishikesh R. Gupta.
*
* This program simulates particle movement in 3D space.
*/

#include <time.h>
#include "utils.h"
// #include "sim.h"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <string>
#include "particles.h"
#include <math.h>
#include <stdlib.h>

using namespace std;

int write_all_particle_details_to_file(string filename);
static const string PDPATH = "./particle_positions/";
//16384
const size_t npart = 1000;
const size_t nsteps = 1000;
const float size_x = 1024.0;
const float size_y = 512.0;
//
const float size_z = 512.0;
//
const float center_x = size_x/3.0;
const float center_y = size_y/3.0;
//
const float center_z = size_z/3.0;
//
const float scale_x = size_x;
const float scale_y = size_y;
//
const float scale_z = size_z;
//
const float delta_t = 1.0e2;
const float scale_mass = 1.0e6;
const float G = 6.67384e-11;
float dt_ms;

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

int main() {

  clock_t begin;
  clock_t end;
  float tacc = 0.0;

  init_particles();

  for(size_t i=0; i<nsteps; i++) {
    begin = clock();
    update_particle_details();

    end = clock();
    dt_ms = ((float)(end-begin))/((float)(CLOCKS_PER_SEC)/1.0e9);
    tacc += dt_ms;

    //string filename ("positions_" + to_string(current_time_frame) + ".txt");
    string filename("positions_" + to_string(i) + ".vtk");

    if (!write_all_particle_details_to_file(filename)) {
      cerr << "Could not write file: " << filename << "\n";}

    }
    printf("update particles_3 for loops: %fns/particle/step\n", tacc/npart/nsteps);

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

  void init_particles() {

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
    for(size_t i=0; i < npart; i++) {
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
    for(size_t i=0; i < npart; i++) {
      vxvec[i] = 0.0;
      vyvec[i] = 0.0;
      vzvec[i] = 0.0;
    }

    // Initialize particle accelerations.
    for(size_t i=0; i < npart; i++) {
      axvec[i] = 0.0;
      ayvec[i] = 0.0;
      azvec[i] = 0.0;
    }

    // Initialize particle mass for all particles.
    for(size_t i=0; i < npart; i++) {
      massvec[i] = randu();
      massvec[i] *= scale_mass;
    }
  }

  void update_particle_details() {
    for(size_t i=0; i < npart; i++) {
      float xi = pxvec[i];
      float yi = pyvec[i];
      float zi = pzvec[i];

      float mi = massvec[i];

      axvec[i] = 0.0;
      ayvec[i] = 0.0;
      azvec[i] = 0.0;

      for(size_t j=0; j < npart; j++) {
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
