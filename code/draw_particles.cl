/**
* @author Samuel A. Cruz Alegría
* @version 2017-06-04 (YYYY-MM-DD)
* Set the necessary pixels to draw the ball-shaped particles.
*/

typedef unsigned char guchar;

kernel void draw_particles_kernel(global guchar * pixels,
                              global float *particle_details,
                              const int n,
                              const int width,
                              const int height,
                              const int row_stride,
                              const int n_channels,
                              const float radius,
                              const float delta,
                              const float dim_factor,
                              const float g)
{
  int id = get_global_id(0);
  if (id < n) {
    int px_i = id*4;      // x-position index
    int py_i = id*4 + 1;  // y-position index

    float px = particle_details[px_i];
    float py = particle_details[py_i];

    // draw filled circle
    // based on: https://stackoverflow.com/questions/1201200/fast-algorithm-for-drawing-filled-circles/1201227
    for(int y = -radius; y <= radius; ++y) {
      for(int x = -radius; x <= radius; ++x) {
        if(x*x + y*y <= radius*radius) {
          int yind = (int)(py+y) * row_stride;  // y index
          int xind = (int)(px+x) * n_channels;  // x index
          int pixind = yind + xind; // pixel index

          for (int c = 0; c < n_channels; ++c)
            pixels[pixind + c] = 255;
          
        }
      }
    }
  }
}
