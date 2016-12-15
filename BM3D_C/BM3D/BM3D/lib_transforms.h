#pragma once
#ifndef LIB_TRANSFORMS_INCLUDED
#define LIB_TRANSFORMS_INCLUDED

#include<vector>

// Compute a Bior1.5 2D
void bior_2d_forward(float * const input, float * output, const unsigned N, const unsigned d_i, const unsigned r_i, const unsigned d_o,
	float * const lpd, float * const hpd);

// Compute a Bior1.5 2D inverse
void bior_2d_inverse(float * signal, const unsigned N, const unsigned d_s, float * const lpr, float * const hpr);

// Precompute the Bior1.5 coefficients
void bior15_coef(float * lp1, float * hp1, float * lp2, float * hp2);

// Apply Walsh-Hadamard transform (non normalized) on a vector of size N = 2^n
void hadamard_transform(float * vec, float * tmp, const unsigned N, const unsigned d) ;

// Process the log2 of N
unsigned log2(const unsigned N);

// Obtain index for periodic extension
void per_ext_ind(unsigned int * ind_per, const unsigned N, const unsigned L);

#endif // LIB_TRANSFORMS_INCLUDED
