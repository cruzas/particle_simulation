/**
* @author Samuel A. Cruz Alegría
* @version 2017-06-04 (YYYY-MM-DD)
*/
#ifndef PARTICLES_H_INCLUDED
#define PARTICLES_H_INCLUDED

extern int animate_particles();
extern int init_params(int argc, char *argv[]);
extern int init_particles();
extern void print_pd_all();
extern int process_arg(char *arg);
extern int set_up_opencl_prelim();

#endif // PARTICLES_H_INCLUDED
