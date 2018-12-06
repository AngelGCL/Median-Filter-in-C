#ifndef INCLUDED_FILTER_H
#define INCLUDED_FILTER_H
/*
Based on Rosseta Stone median filter for PPM image files
This Structure is used to manage the RGB values of each pixel
*/
/* RGB values */
typedef struct { unsigned char r, g, b; } RGB;

RGB *filtImage(int width, int height, const RGB *image, int n);

#endif