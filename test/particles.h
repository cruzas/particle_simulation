/* Author: Samuel A. Cruz Alegría */
#ifndef PARTICLES_H_INCLUDED
#define PARTICLES_H_INCLUDED

const float eps = 1.0;

extern void update_particle_details();

extern int init_params(int argc, char *argv[]);
extern void init_particles();
extern int process_arg(char *arg);


#endif // PARTICLES_H_INCLUDED
