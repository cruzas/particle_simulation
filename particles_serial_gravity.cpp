/**
* Authors: Samuel A. Cruz Alegría, Alessandra M. de Felice, Hrishikesh R. Gupta.
*
* This program simulates particle movement in 2D space.
*/

#include <ctime>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cmath>


#include "particles.h"

using namespace std;

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 800
#define DEBUGGING 1
#undef DEBUGGING

static const string PDPATH = "./particle_positions/";  // Path to write files to.
static int width;         // Width of box containing particles.
static int height;        // Height of box containing particles.
static int n;             // Number of particles.
static float fx;          // Horizontal component of the force field.
static float fy;          // Vertical component of the force field.
static float radius;      // Radius of the particles, in pixels.
static float delta;       // Time, in seconds, for inter-frame interval.
static int total_time_interval;    // Time, in seconds, for total time interval.
static float g;           // Gravitational factor (in y direction).
static float *pd;         // Particle details array.
const float G = 6.67e-11;
static int m=2;
float F;
//float forces[n]={};

//float* array = new float[ n ];

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
       << "[fx=forcefield_x] "
       << "[fy=forcefield_y] "
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

  // Calculate number of loop cycles to be performed given a total time interval
  // and time per frame.
  int nCycles = total_time_interval / delta;
  #ifdef DEBUGGING
  printf("nCycles=%d\n", nCycles);
  #endif
  for (int cycle = 0; cycle < nCycles; ++cycle) {
    float current_time_frame = delta * cycle;

    string current_time_frame_string = to_string(current_time_frame);
    current_time_frame_string.erase(remove(current_time_frame_string.begin(), current_time_frame_string.end(), '.'), current_time_frame_string.end());
    //string filename ("positions_" + to_string(current_time_frame) + ".txt");
    string filename ("positions_" + current_time_frame_string + ".vtk");

    update_particles();
    if (!write_all_particle_details_to_file(filename)) {
      cerr << "Could not write file: " << filename << "\n";
    }
  }

  free(pd);
  return 0;
}

/*
* Initialize all particle details.
* @return 1 on success, 0 on error.
*/
int
init_particles()
{
  // Allocate space for particle details.
  pd = (float *)malloc(sizeof(*pd) * n * 7);
  if (!pd) {
    fprintf(stderr, "Could not allocate space for particle details.\n");
    return 0;
  }

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

  // Go through all particles and initialize their details at random.
  for (int id = 0; id < n; ++id) {
    int px_i = id*7;      // x-position index.
    int py_i = id*7 + 1;  // y-position index.
    int vx_i = id*7 + 2;  // vx-component index.
    int vy_i = id*7 + 3;  // vy-component index.

    int ax_i = id*7+4;
    int ay_i = id*7+5;
    int m_i = id*7+6;

    //pd[px_i] = (double) (rand() % DEFAULT_WIDTH);   // Set x position.
    //pd[py_i] = (double) (rand() % DEFAULT_HEIGHT);  // Set y position.
    //pd[vx_i] = rand() / (float) RAND_MAX * fx;      // Set vx component.
    //pd[vy_i] = rand() / (float) RAND_MAX * fy;      // Set vy component.
    pd[px_i] = 5*id+5;
    pd[py_i] =5*id+5;

    pd[vx_i] = 5.0;      // Set vx component.
    pd[vy_i] = 5.0;      // Set vy component.

    pd[ax_i] = 0.0;      // Set ax component.
    pd[ay_i] = 0.0;      // Set ay component.
    pd[m_i] = 2; //Set mass

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
    //float totalSimulationTime = 10; // The simulation will run for 10 seconds.
    //float currentTime = 0; // This accumulates the time that has passed.
    for (int id=0;id<n;id++){

        int px_id = id*7;      // x-position index.
        int py_id = id*7 + 1;  // y-position index.
        cout<<"\nPositions before the update:  ";
        cout<<"\nid: "<<id;
        cout<<"X: "<<pd[px_id]<<"Y: "<<pd[py_id];

    }

        for (int id = 0; id < n; id++) {
            int px_i = id*7;      // x-position index.
            int py_i = id*7 + 1;  // y-position index.
            int vx_i = id*7 + 2;  // vx-component index.
            int vy_i = id*7 + 3;  // vy-component index.

            int ax_i = id*7+4;
            int ay_i = id*7+5;
            int m_i = id*7+6;

            //pd[px_i]=1;  // x position.
            //pd[py_i]=1;

            float px = pd[px_i];  // x position.
            float py = pd[py_i];  // y position.
            float vx = pd[vx_i];  // vx component.
            float vy = pd[vy_i];  // vy component.
            float ax = pd[ax_i];
            float ay = pd[ay_i];
            int   mass=pd[m_i];

            cout<<"\nID: "<<id;
            cout<<"\npx: "<<px;
            cout<<"\npy: "<<py;
            cout<<"\nvx: "<<vx;
            cout<<"\nvy: "<<vy;
            cout<<"\nax: "<<ax;
            cout<<"\nay: "<<ay;

            // Set new x direction based on horizontal collision with wall.
            if (px + radius >= width || px - radius <= 0) {
                vx = vx * -1;
            }

            // Set new y direction based on vertical collision with wall.
            if (py - radius <= 0 || py + radius >= height) {
                vy = vy * -1;
            }


            for( int id2 = 0; id2 < n; id2++){



                    int px_2 = id2*7;      // x-position index.
                    int py_2 = id2*7 + 1;  // y-position index.
                    int vx_2 = id2*7 + 2;  // vx-component index.
                    int vy_2 = id2*7 + 3;  // vy-component index.

                    int ax_2 = id2*7+4;
                    int ay_2 = id2*7+5;
                    int m_2 = id2*7+6;
                    //pd[px_2]=4;
                    //pd[px_2]=4;

                    float px2 = pd[px_2];  // x position.
                    float py2 = pd[py_2];  // y position.
                    float vx2 = pd[vx_2];  // vx component.
                    float vy2 = pd[vy_2];  // vy component.
                    float ax2 = pd[ax_2];
                    float ay2 = pd[ay_2];
                    int   mass2=pd[m_2];

                    float d = sqrt((px - px2)*(px - px2) + (py - py2)*(py - py2));
               if (d>1e-6) {
                    float F=0;

                    // REVIEW
                    // Force should be in units: 1 kg	*	1 m/s^2 = 1 Newton
                    // but here, it is m^3 kg^-1 s^-2 / m^2 = m * kg^-1 * s^-2 = m / kg s^2
                    // as G is in m^3 kg^-1 s^-2, and d*d is m^2.
                    F=G*(2/d*d); //

                    float dx=px2-px;
                    float dy=py2-py;

                    //float FX = F*(dx/d);
                    //float FY = F*(dy/d);

                   float FX = F*dx;
                   float FY = F*dy;

                   //pd[id*7+4] += FX/2;
                   //pd[id*7+5] += FY/2;
                   pd[id*7+4] += FX;
                   pd[id*7+5] += FY;

                   //cout<<"\nid: "<<id;
                   //cout<<"\nX: "<<pd[id*7+4]<<"\nY: "<<pd[id*7+5];
                  //cout<<"\nerror: "<<d;
                }

            }

        }

    for(int i=0;i<n;i++)
    {

            pd[i*7 + 2] += pd[i*7 + 4]*delta;
            pd[i*7 + 3] += pd[i*7 + 5]*delta;

    }

    for(int j=0;j<n;j++)
    {

            pd[j*7] = pd[j*7]+pd[j*7 + 2]*delta; // m = m +(m/s * s)
            pd[j*7+1] = pd[j*7+1]+pd[j*7 + 3]*delta;
        //cout<<"\nid: "<<j;
        //cout<<"\nX:"<<pd[j*7]<<"\nY"<<pd[j*7+1];

        }

    //pd[0*7] = pd[0*7]+pd[0*7 + 2]*delta; // m = m +(m/s * s)
    //pd[0*7+1] = pd[0*7+1]+pd[0*7 + 3]*delta;

        /*
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
            }*/

  return 1;
}

/* Print the details of all particles.*/
void
print_all_particle_details()
{
  for (int i = 0; i < n; ++i) {
    printf("particles[%d]: px=%f, py=%f, vx=%f, vy=%f\n",
            i, pd[i*4], pd[i*4 + 1], pd[i*4 + 2], pd[i*4 + 3]);
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
            // myfile << i << " " << pd[i*4] << " " << pd[i*4 + 1] << "\n";
            myfile << pd[i*4] << " " << pd[i*4 + 1] << " " << 0 << "\n";
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
  n = 5;                    // Number of particles.
  fx = 50;                  // Horizontal component of the force field.
  fy = 50;                  // Vertical component of the force field.
  radius = 5;               // Radius of the particles, in pixels.
  delta = 1.0;              // Time, in seconds, for inter-frame interval.
  total_time_interval = 10;          // Time, in seconds, for total time interval.
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
  printf("fx=%f\n", fx);
  printf("fy=%f\n", fy);
  printf("radius=%f\n", radius);
  printf("delta=%f\n", delta);
  printf("total_time_interval=%d\n", total_time_interval);
  printf("g: %f\n", g);
  #endif

  return 1;
}
/*
float compute_force(m,r1,r2)
{
    double d=(r1-r2)^2;
    if (d==0) {
        d=0.001;

    }
    return G*(m/(d));
}
*/
/*
* Process the given command-line parameter.
* @param arg The command-line parameter.
* @return 1 on success, 0 on error.*/
int
process_arg(char *arg)
{
  if (strstr(arg, "width="))
  return sscanf(arg, "width=%d", &width) == 1;

  if (strstr(arg, "height="))
  return sscanf(arg, "height=%d", &height) == 1;

  else if (strstr(arg, "n="))
  return sscanf(arg, "n=%d", &n) == 1;

  else if (strstr(arg, "fx="))
  return sscanf(arg, "fx=%f", &fx) == 1;

  else if (strstr(arg, "fy="))
  return sscanf(arg, "fy=%f", &fy) == 1;

  else if (strstr(arg, "radius="))
  return sscanf(arg, "radius=%f", &radius) == 1;

  else if (strstr(arg, "delta="))
  return sscanf(arg, "delta=%f", &delta) == 1;

  else if (strstr(arg, "total_time_interval="))
  return sscanf(arg, "total_time_interval=%d", &total_time_interval) == 1;

  // Return 0 if the given command-line parameter was invalid.
  return 0;
}
