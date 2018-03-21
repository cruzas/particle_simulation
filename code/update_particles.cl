/**
* @author Samuel A. Cruz Alegría
* @version 2017-06-04 (YYYY-MM-DD)
* Update the particle details.
* Based on: https://cg.ivd.kit.edu/downloads/GPGPU_assignment_4.pdf
*/
kernel void update_particles_kernel(global float *particle_details,
                              const int n,
                              const int width,
                              const int height,
                              const float radius,
                              const float delta,
                              const float g)
{
  int id = get_global_id(0);
  if (id < n) {
    int px_i = id*4;      // x-position index
    int py_i = id*4 + 1;  // y-position index
    int vx_i = id*4 + 2;  // vx-component index
    int vy_i = id*4 + 3;  // vy-component index

    float px = particle_details[px_i];
    float py = particle_details[py_i];
    float vx = particle_details[vx_i];
    float vy = particle_details[vy_i];

    // set new x direction based on horizontal collision with wall
    if (px + radius >= width || px - radius <= 0)
      vx = vx * -1;

    // set new y direction based on vertical collision with wall
    if (py - radius <= 0 || py + radius >= height)
      vy = vy * -1;

    particle_details[px_i] = px + (vx*delta);
    particle_details[py_i] = py - (vy*delta) - (0.5 * g * delta * delta);

    particle_details[vx_i] = vx;  // vx only as acceleration in x-direction is 0
    particle_details[vy_i] = vy + 0.5*(g)*delta;

    // correct x position in case updated position goes past wall
    if (particle_details[px_i] - radius <= 0)
      particle_details[px_i] = radius;
    else if (particle_details[px_i] + radius >= height)
      particle_details[px_i] = width - radius;

    // correct y position in case updated position goes past wall
    if (particle_details[py_i] - radius <= 0)
      particle_details[py_i] = radius;
    else if (particle_details[py_i] + radius >= height)
      particle_details[py_i] = height - radius;
  }
}
