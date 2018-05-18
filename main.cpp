//#include <time.h>
#include "utils.h"
#include "sim.h"
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
const size_t npart = 1600;
const size_t nsteps = 100;
const float size_x = 1024.0;
const float size_y = 512.0;
const float center_x = size_x/2.0;
const float center_y = size_y/2.0;
const float scale_x = size_x;
const float scale_y = size_y;
const float delta_t = 1.0e2;
const float scale_mass = 1.0e6;
const float G = 6.67384e-11;

static float *pd;         // Particle details array.

int main() {

    pd=(float*)safe_calloc(7*npart, sizeof(*pd));

    init(npart, pd,scale_x, scale_y, center_x, center_y, scale_mass);


    for(size_t i=0; i<nsteps; i++) {


        update_acc(npart, pd, G);

        update_vel(npart, pd, delta_t);

        update_pos(npart, pd, delta_t);

        //string filename ("positions_" + to_string(current_time_frame) + ".txt");
        string filename("positions_" + to_string(i) + ".vtk");

        if (!write_all_particle_details_to_file(filename)) {
            cerr << "Could not write file: " << filename << "\n";}

    }
}

int write_all_particle_details_to_file(string filename)
{
    ofstream myfile;

    myfile.open(PDPATH + filename, ios::out);
    if (myfile.is_open()) {

        myfile << "# vtk DataFile Version 1.0\n";
        myfile << "3D triangulation data\n";
        myfile << "ASCII\n\n";

        myfile << "DATASET POLYDATA\n";
        myfile << "POINTS " << npart << " float\n";

        for (int i = 0; i < npart; ++i) {
            myfile << pd[7*i+0] << " " << pd[7*i+1] << " " << 0 << "\n";
        }

        myfile.close();
    } else {
        cerr << "Unable to open file: " << filename << "\n";
        return 0;
    }

    return 1;
}

void init(size_t n, float *pd,float scale_x, float scale_y,float center_x, float center_y, float scale_mass) {

    for(size_t i=0; i<n; i++) {

        pd[7*i+0] = randu()-0.5;
        pd[7*i+1] = randu()-0.5;
        pd[7*i+0] *= scale_x;
        pd[7*i+1] *= scale_y;
        pd[7*i+0] += center_x;
        pd[7*i+1] += center_y;
    }

    for(size_t i=0; i<n; i++) {

        pd[7*i+2] = 0.0;
        pd[7*i+3] = 0.0;
    }

    for(size_t i=0; i<n; i++) {
        pd[7*i+4] = 0.0;
        pd[7*i+5] = 0.0;
    }

    for(size_t i=0; i<n; i++) {
        pd[7*i+6] = randu();
        pd[7*i+6] *= scale_mass;
    }
}

void update_acc(size_t n, float *pd, float G) {

    for(size_t i=0; i<n; i++) {

        float xi = pd[7*i+0];
        float yi = pd[7*i+1];
        float mi = pd[7*i+6];

        pd[7*i+4] = 0.0;
        pd[7*i+5] = 0.0;

        for(size_t j=0; j<n; j++) {

            float xj = pd[7*j+0];
            float yj = pd[7*j+1];
            float mj = pd[7*j+6];

            float dx = xj-xi;
            float dy = yj-yi;
            float d = sqrt(dx*dx+dy*dy)+eps;

            float f = G*mi*mj/(d*d);
            float fx = f*(dx/d);
            float fy = f*(dy/d);

            pd[7*i+4] += fx/mi;
            pd[7*i+5] += fy/mi;
        }
    }
}

void update_vel(size_t n, float *pd,float delta_t) {
    for(size_t i=0; i<n; i++) {
        pd[7*i+2] += pd[7*i+4]*delta_t;
        pd[7*i+3] += pd[7*i+5]*delta_t;
    }
}

void update_pos(size_t n, float *pd,float delta_t) {
    for(size_t i=0; i<n; i++) {
        pd[7*i+0] += pd[7*i+2]*delta_t;
        pd[7*i+1] += pd[7*i+3]*delta_t;

    }
}
