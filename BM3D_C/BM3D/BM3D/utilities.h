#pragma once
#ifndef UTILITIES_H_INCLUDED
#define UTILITIES_H_INCLUDED

#include <vector>
#include "fftw3.h"
#include "ImgProcUtility.h"
#include <opencv2/opencv.hpp>

// Read image and check number of channels
int load_image(char* name, IplImage * &iplImage);

// Write image
int save_image(char* name, IplImage * &iplImage);

// Check if a number is a power of 2
bool power_of_2(const unsigned n);

// Add boundaries by symetry
void symetrize(float * &img, float * &img_sym, const unsigned width, const unsigned height, const unsigned chnls, const unsigned N);
IplImage * symetrize(IplImage * iplImage, const unsigned N);

// Transform the color space of the image
int color_space_transform(float * &img, const unsigned color_space, const unsigned width, const unsigned height, const unsigned chnls, const bool rgb2yuv);
IplImage * color_space_transform(IplImage * iplImage, const bool rgb2yuv);

// Look for the closest power of 2 number
int closest_power_of_2(const unsigned n);

// Estimate sigma on each channel according to the choice of the color_space
int estimate_sigma(const float sigma, float * sigma_table, const unsigned chnls, const unsigned color_space);

// Initialize a set of indices
void ind_initialize(unsigned * &ind_set, const unsigned max_size, const unsigned N, const unsigned step, unsigned &size);

// For convenience
unsigned ind_size(const unsigned max_size, const unsigned N, const unsigned step);

// Initialize a 2D fftwf_plan with some parameters
void allocate_plan_2d(fftwf_plan* plan, const unsigned N, const fftwf_r2r_kind kind, const unsigned nb);

// Initialize a 1D fftwf_plan with some parameters
void allocate_plan_1d(fftwf_plan* plan, const unsigned N, const fftwf_r2r_kind kind, const unsigned nb);

// Tabulated values of log2(2^n)
unsigned ind_log2(const unsigned N);

// Tabulated values of 2^N
unsigned ind_pow2(const unsigned N);


#endif // UTILITIES_H_INCLUDED
