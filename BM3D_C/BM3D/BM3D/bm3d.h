#pragma once
#ifndef BM3D_H_INCLUDED
#define BM3D_H_INCLUDED

#include "fftw3.h"

#include "ImgProcUtility.h"

struct TD
{
	float f;
	unsigned u;
	TD() {}
	TD(float _f, unsigned _u) : f(_f), u(_u) {}
};

// Main function
IplImage * run_bm3d(IplImage * iplImage, const float sigma);

float * transfer_iplImage2buffer(IplImage * iplImage);

IplImage * transfer_buffer2iplImage(float * vec, const unsigned width, const unsigned height, const unsigned chnls, const bool clip);

IplImage * bm3d_1st_step(IplImage * iplImage, const float sigma, fftwf_plan *  plan_2d_for_1, fftwf_plan *  plan_2d_for_2, fftwf_plan *  plan_2d_inv);

IplImage * bm3d_2nd_step(IplImage * iplImage, IplImage * iplImage_basic, const float sigma, fftwf_plan *  plan_2d_for_1, fftwf_plan *  plan_2d_for_2, fftwf_plan *  plan_2d_inv);

// Process 2D dct of a group of patches
void dct_2d_process(
    float * DCT_table_2D,
    float * const img,
    fftwf_plan * plan_1,
    fftwf_plan * plan_2,
    const unsigned nHW,
    const unsigned width,
    const unsigned height,
    const unsigned chnls,
    const unsigned kHW,
    const unsigned i_r,
    const unsigned step,
    float * const coef_norm,
    const unsigned i_min,
    const unsigned i_max
);

// Process 2D bior1.5 transform of a group of patches
void bior_2d_process(
    float * bior_table_2D,
    float * const img,  
    const unsigned nHW,
    const unsigned width,
    const unsigned height,
    const unsigned chnls,
    const unsigned kHW,
    const unsigned i_r,
    const unsigned step,
    const unsigned i_min,
    const unsigned i_max,
    float * lpd,
    float * hpd
);

void dct_2d_inverse(
	float * group_3D_table,
    const unsigned kHW,
    const unsigned N,	
    const unsigned group_3D_table_size,
    float * const coef_norm_inv,
    fftwf_plan * plan
);

void bior_2d_inverse(
	float * group_3D_table,
    const unsigned kHW,  
    float * const lpr,  
    float * const hpr,	
    const unsigned group_3D_table_size
);

// HT filtering using Welsh-Hadamard transform (do only
// third dimension transform, Hard Thresholding
// and inverse Hadamard transform)
void ht_filtering_hadamard(
    float * group_3D,  
    float * tmp,  
    const unsigned nSx_r, 
    const unsigned kHard, 
    const unsigned chnls,
    float * const sigma_table,
    const float lambdaThr3D,
    float * weight_table,
    const bool doWeight
);

// Wiener filtering using Welsh-Hadamard transform
void wiener_filtering_hadamard(
    float * group_3D_img,
    float * group_3D_est,
    float * tmp, 
    const unsigned nSx_r,   
    const unsigned kWien,
    const unsigned chnls,
    float * const sigma_table,
    float * weight_table,
    const bool doWeight
);

// Compute weighting using Standard Deviation
void sd_weighting(
    float * const group_3D, 
    const unsigned nSx_r, 
    const unsigned kHW,
    const unsigned chnls,   
    float * weight_table
);

// Preprocess coefficients of the Kaiser window and normalization coef for the DCT
void preProcess(
    float * kaiserWindow,
    float * coef_norm,
    float * coef_norm_inv,
    const unsigned kHW
);

void precompute_BM(
	unsigned ** &patch_table,	
    unsigned * &patch_table_size,
    float * const &img,   
    const unsigned width,
    const unsigned height,  
    const unsigned kHW,  
    const unsigned NHW,  
    const unsigned n,   
    const unsigned pHW, 
    const float    tauMatch
);

#endif // BM3D_H_INCLUDED
