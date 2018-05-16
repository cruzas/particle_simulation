#include <stdlib.h>

#ifndef SIM
#define SIM

const float eps = 1.0;

// initialize particles, position and velocity
/*void init(size_t n, float *part_pos, float *part_vel,
        float *part_acc, float *part_mass,
        float scale_x, float scale_y,
        float center_x, float center_y,
        float scale_mass);*/

//void init(size_t n, float *pd,
//          float scale_x, float scale_y,
//          float center_x, float center_y,
//          float scale_mass);

void init(size_t n, float *pd,
          float scale_x, float scale_y, float scale_z,
          float center_x, float center_y,  float center_z,
          float scale_mass);

//void init(size_t n, float *pd,float scale_x, float scale_y, float scale_z, float center_x, float center_y,  float center_z, float scale_mass);

// update acceleration
//void update_acc(size_t n, float *part_pos, float *part_vel,float *part_acc, float *part_mass, float grav_const);

void update_acc(size_t n, float *pd, float grav_const, float delta_t);

// update velocity
//void update_vel(size_t n, float *part_vel, float *part_acc, float delta_t);
void update_vel(size_t n, float *pd, float delta_t);

// update particles
//void update_pos(size_t n, float *part_pos, float *part_vel, float delta_t);
void update_pos(size_t n, float *pd, float delta_t);



#endif
