/**
* @author Samuel A. Cruz Alegría
* @version 2017-06-04 (YYYY-MM-DD)
* Set the necessary pixels to draw the trace of the particles.
*/

typedef unsigned char guchar;

kernel void trace_particles_kernel(global guchar * pixels,
                                const int width,
                                const int height,
                                const unsigned int row_stride,
                                const unsigned int n_channels,
                                const float dim_factor)
{
        int x = get_global_id(0); // x position in pixels
        int y = get_global_id(1); // y position in pixels

        if (x < width && y < height) {
          pixels += y*row_stride + x*n_channels;
          for(int i = 0; i < n_channels; ++i) {
            pixels[i] *= dim_factor;
          }
        }
}
