/*
* Copyright (c) 2011, Marc Lebrun <marc.lebrun@cmla.ens-cachan.fr>
* All rights reserved.
*
* This program is free software: you can use, modify and/or
* redistribute it under the terms of the GNU General Public
* License as published by the Free Software Foundation, either
* version 3 of the License, or (at your option) any later
* version. You should have received a copy of this license along
* this program. If not, see <http://www.gnu.org/licenses/>.
*/


/**
* @file bm3d.cpp
* @brief BM3D denoising functions
*
* @author Marc Lebrun <marc.lebrun@cmla.ens-cachan.fr>
**/

#include <iostream>
#include <algorithm>
#include <math.h>

#include "bm3d.h"
#include "utilities.h"
#include "lib_transforms.h"

#define SQRT2     1.414213562373095
#define SQRT2_INV 0.7071067811865475
#define YUV       0
#define YCBCR     1
#define OPP       2
#define RGB       3
#define DCT       4
#define BIOR      5
#define HADAMARD  6

using namespace std;


void Swap(TD * td, int x, int y)
{
	TD temp = td[x];
	td[x] = td[y];
	td[y] = temp;
}

void SelectSort(TD * &td, unsigned size)
{
	unsigned minIndex;
	for (unsigned i = 0; i < size; i++)
	{
		minIndex = i;
		for (unsigned j = i + 1; j < size; j++)
		{
			if (td[minIndex].f > td[j].f)
			{
				minIndex = j;
			}
		}
		if (minIndex != i)
		{
			Swap(td, i, minIndex);
		}
	}
}


//
// @brief run BM3D process. Depending on if OpenMP is used or not,
//        and on the number of available threads, it divides the noisy
//        image in sub_images, to process them in parallel.
//
// @param sigma: value of assumed noise of the noisy image;
// @param img_noisy: noisy image;
// @param img_basic: will be the basic estimation after the 1st step
// @param img_denoised: will be the denoised final image;
// @param width, height, chnls: size of the image;
// @param useSD_h (resp. useSD_w): if true, use weight based
//        on the standard variation of the 3D group for the
//        first (resp. second) step, otherwise use the number
//        of non-zero coefficients after Hard Thresholding
//        (resp. the norm of Wiener coefficients);
// @param tau_2D_hard (resp. tau_2D_wien): 2D transform to apply
//        on every 3D group for the first (resp. second) part.
//        Allowed values are DCT and BIOR;
// @param color_space: Transformation from RGB to YUV. Allowed
//        values are RGB (do nothing), YUV, YCBCR and OPP.
//
// @return EXIT_FAILURE if color_space has not expected
//         type, otherwise return EXIT_SUCCESS.
//
IplImage * run_bm3d(IplImage * iplImage, const float sigma)
{
	const unsigned int width = iplImage->width;
	const unsigned int height = iplImage->height;
	const unsigned int chnls = iplImage->nChannels;
	const bool useSD_h = false;
	const bool useSD_w = true;
	const unsigned int tau_2D_hard = 5;
	const unsigned int tau_2D_wien = 4;
	const unsigned int color_space = 2;

	float * img = transfer_iplImage2buffer(iplImage);

	float * img_basic = new float[width * height * chnls];
	float * img_denoised = new float[width * height * chnls];

	// Parameters
	const unsigned int nHard = 7; // Half size of the search window
	const unsigned int nWien = 7; // Half size of the search window
	const unsigned int kHard = (tau_2D_hard == BIOR || sigma < 40.f ? 8 : 12); // Must be a power of 2 if tau_2D_hard == BIOR
	const unsigned int kWien = (tau_2D_wien == BIOR || sigma < 40.f ? 4 : 12); // Must be a power of 2 if tau_2D_wien == BIOR
	const unsigned int NHard = 16; // Must be a power of 2
	const unsigned int NWien = 32; // Must be a power of 2
	const unsigned int pHard = 3;
	const unsigned int pWien = 3;

	// Transformation to YUV color space
	if (color_space_transform(img, color_space, width, height, chnls, true))
	{
		return NULL;
	}

    IplImage * iplImage_ = color_space_transform(iplImage, true);

	unsigned nb_threads = 1;
	fftwf_plan* plan_2d_for_1 = new fftwf_plan[nb_threads];
	fftwf_plan* plan_2d_for_2 = new fftwf_plan[nb_threads];
	fftwf_plan* plan_2d_inv = new fftwf_plan[nb_threads];


	// In the simple case
	// Add boundaries and symetrize them
	const unsigned h_b = height + 2 * nHard;
	const unsigned w_b = width + 2 * nHard;

	// padding
    IplImage * iplImage_sym = symetrize(iplImage_, nHard);
    float * img_sym_noisy = transfer_iplImage2buffer(iplImage_sym);

	// Allocating Plan for FFTW process
	if (tau_2D_hard == DCT)
	{
		const unsigned nb_cols = ind_size(w_b - kHard + 1, nHard, pHard);
		allocate_plan_2d(&plan_2d_for_1[0], kHard, FFTW_REDFT10,
			w_b * (2 * nHard + 1) * chnls);
		allocate_plan_2d(&plan_2d_for_2[0], kHard, FFTW_REDFT10,
			w_b * pHard * chnls);
		allocate_plan_2d(&plan_2d_inv[0], kHard, FFTW_REDFT01,
			NHard * nb_cols * chnls);
	}

	// Denoising, 1st Step
	cout << "step 1...";
    IplImage * iplImage_sym_basic = bm3d_1st_step(iplImage_sym, sigma, plan_2d_for_1,
                                                  plan_2d_for_2, plan_2d_inv);
	cout << "done." << endl;

    float * img_sym_basic = transfer_iplImage2buffer(iplImage_sym_basic);

	// To avoid boundaries problem
	for (unsigned c = 0; c < chnls; c++)
	{
		const unsigned dc_b = c * w_b * h_b + nHard * w_b + nHard;
		unsigned dc = c * width * height;
		for (unsigned i = 0; i < height; i++)
			for (unsigned j = 0; j < width; j++, dc++)
			{
				img_basic[dc] = img_sym_basic[dc_b + i * w_b + j];
			}
	}
	symetrize(img_basic, img_sym_basic, width, height, chnls, nHard);


	// Allocating Plan for FFTW process
	if (tau_2D_wien == DCT)
	{
		const unsigned nb_cols = ind_size(w_b - kWien + 1, nWien, pWien);
		allocate_plan_2d(&plan_2d_for_1[0], kWien, FFTW_REDFT10,
			w_b * (2 * nWien + 1) * chnls);
		allocate_plan_2d(&plan_2d_for_2[0], kWien, FFTW_REDFT10,
			w_b * pWien * chnls);
		allocate_plan_2d(&plan_2d_inv[0], kWien, FFTW_REDFT01,
			NWien * nb_cols * chnls);
	}

	// Denoising, 2nd Step
	cout << "step 2...";
    IplImage * iplImage_sym_denoised = bm3d_2nd_step(iplImage_sym, iplImage_sym_basic,
                                                     sigma, plan_2d_for_1, plan_2d_for_2,
                                                     plan_2d_inv);
	cout << "done." << endl;
    float * img_sym_denoised = transfer_iplImage2buffer(iplImage_sym_denoised);
	// Obtention of img_denoised
	for (unsigned c = 0; c < chnls; c++)
	{
		const unsigned dc_b = c * w_b * h_b + nWien * w_b + nWien;
		unsigned dc = c * width * height;
		for (unsigned i = 0; i < height; i++)
			for (unsigned j = 0; j < width; j++, dc++)
				img_denoised[dc] = img_sym_denoised[dc_b + i * w_b + j];
	}

	// Inverse color space transform to RGB
	if (color_space_transform(img_denoised, color_space, width, height, chnls, false)
		!= EXIT_SUCCESS) return NULL;

	if (color_space_transform(img, color_space, width, height, chnls, false)
		!= EXIT_SUCCESS) return NULL;

	if (color_space_transform(img_basic, color_space, width, height, chnls, false)
		!= EXIT_SUCCESS) return NULL;

	iplImage = transfer_buffer2iplImage(img, width, height, chnls, true);
    IplImage * iplImage_basic = transfer_buffer2iplImage(img_basic, width, height, chnls, true);
	IplImage * iplImage_denoised = transfer_buffer2iplImage(img_denoised, width, height, chnls, true);

	// Free Memory
	if (tau_2D_hard == DCT || tau_2D_wien == DCT)
		for (unsigned n = 0; n < nb_threads; n++)
		{
			fftwf_destroy_plan(plan_2d_for_1[n]);
			fftwf_destroy_plan(plan_2d_for_2[n]);
			fftwf_destroy_plan(plan_2d_inv[n]);
		}
	fftwf_cleanup();

	delete[] img_denoised;
	delete[] img_basic;

	img_denoised = NULL;
	img_basic = NULL;

    return iplImage_denoised;
}

float * transfer_iplImage2buffer(IplImage * iplImage) 
{
	const unsigned int width = iplImage->width;
	const unsigned int height = iplImage->height;
	const unsigned int chnls = iplImage->nChannels;

	float * vec = new float[width * height * chnls];
	if (!vec)
	{
		delete[] vec;
		vec = NULL;
	}
    if (SR_DEPTH_8U == iplImage->depth)
    {
        for (int y = 0; y < iplImage->height; y++)
        {
            unsigned char *pSrc = (unsigned char *)((char *)iplImage->imageData + y * iplImage->widthStep);
            for (int x = 0; x < iplImage->width; x++)
            {
                for (unsigned int c = 0; c < chnls; c++)
                {
                    vec[chnls * x + c + y * width * chnls] = pSrc[chnls * x + c];
                }
            }
        }
    }
    else if (SR_DEPTH_32F == iplImage->depth)
    {
        for (int y = 0; y < iplImage->height; y++)
        {
            float *pSrc = (float *)((char *)iplImage->imageData + y * iplImage->widthStep);
            for (int x = 0; x < iplImage->width; x++)
            {
                for (unsigned int c = 0; c < chnls; c++)
                {
                    vec[chnls * x + c + y * width * chnls] = pSrc[chnls * x + c];
                }
            }
        }
    }
	return vec;
}

IplImage * transfer_buffer2iplImage(float * vec, const unsigned width, const unsigned height, const unsigned chnls, const bool clip)
{
    IplImage * iplImage = CImageUtility::createImage(width, height, SR_DEPTH_32F, chnls);
    if (clip)
    {
        for (int y = 0; y < iplImage->height; y++)
        {
            float *pDst = (float *)((char *)iplImage->imageData + y * iplImage->widthStep);
            for (int x = 0; x < iplImage->width; x++)
            {
                for (unsigned int c = 0; c < chnls; c++)
                {
                    int xx = chnls * x + c + y * width * chnls;
                    //vec[xx] = (vec[xx] > 255.0f ? 255.0f : (vec[xx] < 0.0f ? 0.0f : vec[xx]));
                    //vec[xx] /= 255.0;
                    //if (vec[xx] < 0)
                    //    vec[x] = 1 + vec[xx];
                    pDst[chnls * x + c] = vec[xx];
                }
            }
        }
    }
    else
    {
        for (int y = 0; y < iplImage->height; ++y)
        {
            float * pDst = (float *)((char *)iplImage->imageData + y * iplImage->widthStep);
            for (int x = 0; x < iplImage->width; ++x)
            {
                for (unsigned int c = 0; c < chnls; ++c)
                {
                    int xx = chnls * x + c + y * width * chnls;
                    pDst[chnls * x + c] = vec[xx];
                }
            }
        }
    }

	return iplImage;
}

//
// @brief Run the basic process of BM3D (1st step). The result
//        is contained in img_basic. The image has boundary, which
//        are here only for block-matching and doesn't need to be
//        denoised.
//
// @param sigma: value of assumed noise of the image to denoise;
// @param img_noisy: noisy image;
// @param img_basic: will contain the denoised image after the 1st step;
// @param width, height, chnls : size of img_noisy;
// @param nHard: size of the boundary around img_noisy;
// @param useSD: if true, use weight based on the standard variation
//        of the 3D group for the first step, otherwise use the number
//        of non-zero coefficients after Hard-thresholding;
// @param tau_2D: DCT or BIOR;
// @param plan_2d_for_1, plan_2d_for_2, plan_2d_inv : for convenience. Used
//        by fftw.
//
// @return none.
//
IplImage * bm3d_1st_step(IplImage * iplImage, const float sigma, fftwf_plan *  plan_2d_for_1, fftwf_plan *  plan_2d_for_2, fftwf_plan *  plan_2d_inv)
{
    // iplImage with padding, width = width + boundary, height = height + boundary
    const unsigned int width = iplImage->width;
    const unsigned int height = iplImage->height;
    const unsigned int chnls = iplImage->nChannels;
    const unsigned int tau_2D = 5;
    const unsigned int nHard = 7;
    const unsigned int kHard = (tau_2D == BIOR || sigma < 40.f ? 8 : 12);
    const unsigned int NHard = 16;
    const unsigned int pHard = 3;
    const bool useSD = false;
    const unsigned int color_space = 2;

    float * img_noisy = transfer_iplImage2buffer(iplImage);
    float * img_basic = new float[width * height * chnls];
	// Estimatation of sigma on each channel
	float * sigma_table = new float[chnls];
	if (!sigma_table)
	{
		CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_1st_step!\n");
		delete[] sigma_table;
		sigma_table = NULL;
	}
	if (estimate_sigma(sigma, sigma_table, chnls, color_space))
	{
		return NULL;
	}

	// Parameters initialization
	// Threshold for Hard Thresholding
	const float    lambdaHard3D = 2.7f;

	// threshold used to determinate similarity between patches
	const float    tauMatch = (chnls == 1 ? 3.f : 1.f) * (sigma_table[0] < 35.0f ? 2500 : 5000);

	// Initialization for convenience
	unsigned int * row_ind;
	unsigned int row_ind_size;
	ind_initialize(row_ind, height - kHard + 1, nHard, pHard, row_ind_size);
	unsigned int * column_ind;
	unsigned int column_ind_size;
	ind_initialize(column_ind, width - kHard + 1, nHard, pHard, column_ind_size);

	const unsigned int kHard_2 = kHard * kHard;

	float * group_3D_table;
	float * wx_r_table;

	float * hadamard_tmp = new float[NHard];
	float * kaiser_window = new float[kHard_2];
	float * coef_norm = new float[kHard_2];
	float * coef_norm_inv = new float[kHard_2];
	if (!hadamard_tmp || !kaiser_window || !coef_norm || !coef_norm_inv)
	{
		if (!hadamard_tmp)
		{
			delete[] hadamard_tmp;
			hadamard_tmp = NULL;
		}
		if (!kaiser_window)
		{
			delete[] kaiser_window;
			kaiser_window = NULL;
		}
		if (!coef_norm)
		{
			delete[] kaiser_window;
			kaiser_window = NULL;
		}
		if (!coef_norm_inv)
		{
			delete[] coef_norm_inv;
			coef_norm_inv = NULL;
		}
		CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_1st_step!\n");
		return NULL;
	}

	// Check allocation memory
	preProcess(kaiser_window, coef_norm, coef_norm_inv, kHard);

	// Preprocessing of Bior table
	float * lpd = new float[10];
	float * hpd = new float[10];
	float * lpr = new float[10];
	float * hpr = new float[10];
	if (!lpd || !hpd || !lpr || !hpr)
	{
		if (!lpd)
		{
			delete[] lpd;
			lpd = NULL;
		}
		if (!hpd)
		{
			delete[] hpd;
			hpd = NULL;
		}
		if (!lpr)
		{
			delete[] lpr;
			lpr = NULL;
		}
		if (!hpr)
		{
			delete[] hpr;
			hpr = NULL;
		}
		CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_1st_step!\n");
		return NULL;
	}
	bior15_coef(lpd, hpd, lpr, hpr);

	// For aggregation part
	float * denominator = new float[width * height * chnls]();
	float * numerator = new float[width * height * chnls]();
	if (!denominator || !numerator)
	{
		if (!denominator)
		{
			delete[] denominator;
			denominator = NULL;
		}
		if (!numerator)
		{
			delete[] numerator;
			numerator = NULL;
		}
		CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_1st_step!\n");
		return NULL;
	}

	// Precompute Bloc-Matching
	unsigned int ** patch_table;
	unsigned int * patch_table_size;
	precompute_BM(patch_table, patch_table_size, img_noisy, width, height, kHard, NHard, nHard, pHard, tauMatch);
	// nHard -- window size, NHard -- max number of similar patches


	float * table_2D = new float[(2 * nHard + 1) * width * chnls * kHard_2]();
	if (!table_2D)
	{
		CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_1st_step!\n");
		delete[] table_2D;
		table_2D = NULL;
	}

	// Loop on i_r
	for (unsigned int ind_i = 0; ind_i < row_ind_size; ind_i++)
	{
		const unsigned int i_r = row_ind[ind_i];

		// Update of table_2D
		if (tau_2D == DCT)
			dct_2d_process(table_2D, img_noisy, plan_2d_for_1, plan_2d_for_2, nHard,
			width, height, chnls, kHard, i_r, pHard, coef_norm,
			row_ind[0], row_ind[row_ind_size - 1]);
		else if (tau_2D == BIOR)
			bior_2d_process(table_2D, img_noisy, nHard, width, height, chnls,
			kHard, i_r, pHard, row_ind[0], row_ind[row_ind_size - 1], lpd, hpd);

		wx_r_table = new float[chnls * column_ind_size];
		if (!wx_r_table)
		{
			CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_1st_step!\n");
			delete[] wx_r_table;
			wx_r_table = NULL;
		}

		unsigned int sum_nSx_r = 0;
		for (unsigned int ind_j = 0; ind_j < column_ind_size; ind_j++)
		{
			const unsigned int j_r = column_ind[ind_j];
			const unsigned int k_r = i_r * width + j_r;
			sum_nSx_r += patch_table_size[k_r];
		}
		unsigned int group_3D_table_size = chnls * sum_nSx_r * kHard_2;
		group_3D_table = new float[group_3D_table_size];
		if (!group_3D_table)
		{
			CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_1st_step!\n");
			delete[] group_3D_table;
			group_3D_table = NULL;
		}
		unsigned int sum = 0;

		// Loop on j_r
		for (unsigned int ind_j = 0; ind_j < column_ind_size; ind_j++)
		{
			// Initialization
			const unsigned int j_r = column_ind[ind_j];
			const unsigned int k_r = i_r * width + j_r;

			// Number of similar patches
			const unsigned int nSx_r = patch_table_size[k_r];

			// Build of the 3D group
			float * group_3D = new float[chnls * nSx_r * kHard_2]();
			float * group_3D_table_ = new float[chnls * nSx_r * kHard_2];
			if (!group_3D || !group_3D_table_)
			{
				if (!group_3D)
				{
					delete[] group_3D;
					group_3D = NULL;
				}
				if (!group_3D_table_)
				{
					delete[] group_3D_table_;
					group_3D_table_ = NULL;
				}
				CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_1st_step!\n");
				return NULL;
			}
			for (unsigned int c = 0; c < chnls; c++)
				for (unsigned int n = 0; n < nSx_r; n++)
				{
					const unsigned int ind = patch_table[k_r][n] + (nHard - i_r) * width;
					for (unsigned int k = 0; k < kHard_2; k++)
						group_3D[n + k * nSx_r + c * kHard_2 * nSx_r] =
						table_2D[k + ind * kHard_2 + c * kHard_2 * (2 * nHard + 1) * width];
				}

			// HT filtering of the 3D group
			float * weight_table = new float[chnls];
			if (!weight_table)
			{
				CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_1st_step!\n");
				delete[] weight_table;
				weight_table = NULL;
			}
			ht_filtering_hadamard(group_3D, hadamard_tmp, nSx_r, kHard, chnls, sigma_table,
				lambdaHard3D, weight_table, !useSD);

			// 3D weighting using Standard Deviation
			if (useSD)
				sd_weighting(group_3D, nSx_r, kHard, chnls, weight_table);

			// Save the 3D group. The DCT 2D inverse will be done after.
			for (unsigned int c = 0; c < chnls; c++)
				for (unsigned int n = 0; n < nSx_r; n++)
					for (unsigned int k = 0; k < kHard_2; k++)
					{
						unsigned int index = k + n * kHard_2 + c * nSx_r * kHard_2;
						group_3D_table_[index] = group_3D[n + k * nSx_r + c * kHard_2 * nSx_r];
					}
			unsigned int sz = chnls * nSx_r * kHard_2;
			for (unsigned int i = 0; i < sz; i++)
			{
				group_3D_table[i + sum] = group_3D_table_[i];
			}
			sum += sz;

			// Save weighting
			for (unsigned int c = 0; c < chnls; c++)
			{
				wx_r_table[chnls * ind_j + c] = weight_table[c];
			}

			delete[] group_3D_table_;
			delete[] weight_table;
			delete[] group_3D;

			group_3D_table_ = NULL;
			weight_table = NULL;
			group_3D = NULL;

		} // End of loop on j_r

		//  Apply 2D inverse transform
		if (tau_2D == DCT)
		{
			dct_2d_inverse(group_3D_table, kHard, NHard * chnls * column_ind_size, group_3D_table_size,
				coef_norm_inv, plan_2d_inv);
		}

		else if (tau_2D == BIOR)
		{
			bior_2d_inverse(group_3D_table, kHard, lpr, hpr, group_3D_table_size);
		}

		// Registration of the weighted estimation
		unsigned int dec = 0;
		for (unsigned int ind_j = 0; ind_j < column_ind_size; ind_j++)
		{
			const unsigned int j_r = column_ind[ind_j];
			const unsigned int k_r = i_r * width + j_r;
			const unsigned int nSx_r = patch_table_size[k_r];

			for (unsigned int c = 0; c < chnls; c++)
			{
				for (unsigned int n = 0; n < nSx_r; n++)
				{
					const unsigned int k = patch_table[k_r][n] + c * width * height;
					for (unsigned int p = 0; p < kHard; p++)
						for (unsigned int q = 0; q < kHard; q++)
						{
							const unsigned int ind = k + p * width + q;
							numerator[ind] += kaiser_window[p * kHard + q]
								* wx_r_table[c + ind_j * chnls]
								* group_3D_table[p * kHard + q + n * kHard_2
								+ c * kHard_2 * nSx_r + dec];
							denominator[ind] += kaiser_window[p * kHard + q]
								* wx_r_table[c + ind_j * chnls];
						}
				}
			}
			dec += nSx_r * chnls * kHard_2;
		}
		delete[] group_3D_table;
		delete[] wx_r_table;

		group_3D_table = NULL;
		wx_r_table = NULL;

	} // End of loop on i_r

	delete[] table_2D;
	delete[] hpr;
	delete[] lpr;
	delete[] hpd;
	delete[] lpd;
	delete[] coef_norm_inv;
	delete[] coef_norm;
	delete[] kaiser_window;
	delete[] hadamard_tmp;
	delete[] sigma_table;

	table_2D = NULL;
	hpr = NULL;
	lpr = NULL;
	hpd = NULL;
	lpd = NULL;
	coef_norm_inv = NULL;
	coef_norm = NULL;
	kaiser_window = NULL;
	hadamard_tmp = NULL;
	sigma_table = NULL;

	// Final reconstruction
	for (unsigned int k = 0; k < width * height * chnls; k++)
	{
		img_basic[k] = numerator[k] / denominator[k];
		// Foreback black-blocking problem
		if (denominator[k] == 0.0)
		{
			img_basic[k] = img_noisy[k];
		}
	}
	delete[] numerator;
	delete[] denominator;

	numerator = NULL;
	denominator = NULL;

    IplImage * iplImage_basic = transfer_buffer2iplImage(img_basic, width, height, chnls, false);
    return iplImage_basic;
}

//
// @brief Run the final process of BM3D (2nd step). The result
//        is contained in img_denoised. The image has boundary, which
//        are here only for block-matching and doesn't need to be
//        denoised.
//
// @param sigma: value of assumed noise of the image to denoise;
// @param img_noisy: noisy image;
// @param img_basic: contains the denoised image after the 1st step;
// @param img_denoised: will contain the final estimate of the denoised
//        image after the second step;
// @param width, height, chnls : size of img_noisy;
// @param nWien: size of the boundary around img_noisy;
// @param useSD: if true, use weight based on the standard variation
//        of the 3D group for the second step, otherwise use the norm
//        of Wiener coefficients of the 3D group;
// @param tau_2D: DCT or BIOR.
//
// @return none.
//
IplImage * bm3d_2nd_step(IplImage * iplImage, IplImage * iplImage_basic, const float sigma, fftwf_plan *  plan_2d_for_1, fftwf_plan *  plan_2d_for_2, fftwf_plan *  plan_2d_inv)
{
    float * img_basic = transfer_iplImage2buffer(iplImage_basic);
    float * img_noisy = transfer_iplImage2buffer(iplImage);
    const unsigned int width = iplImage->width;
    const unsigned int height = iplImage->height;
    const unsigned int chnls = iplImage->nChannels;

    float * img_denoised = new float[width * height * chnls];

    const unsigned int tau_2D = 4;
    const unsigned int nWien = 7;
    const unsigned int kWien = (tau_2D == BIOR || sigma < 40.f ? 4 : 12);
    const unsigned int NWien = 32;
    const unsigned int pWien = 3;
    const bool useSD = true;
    const unsigned int color_space = 2;
	// Estimatation of sigma on each channel
	float * sigma_table = new float[chnls];
	if (!sigma_table)
	{
		CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_2nd_step!\n");
		delete[] sigma_table;
		sigma_table = NULL;
	}
	if (estimate_sigma(sigma, sigma_table, chnls, color_space))
	{
		return NULL;
	}

	// Parameters initialization
	// threshold used to determinate similarity between patches
	const float tauMatch = (sigma_table[0] < 35.0f ? 400.0f : 3500);

	// Initialization for convenience
	unsigned int * row_ind;
	unsigned int row_ind_size;
	ind_initialize(row_ind, height - kWien + 1, nWien, pWien, row_ind_size);
	unsigned int * column_ind;
	unsigned int column_ind_size;
	ind_initialize(column_ind, width - kWien + 1, nWien, pWien, column_ind_size);
	const unsigned int kWien_2 = kWien * kWien;

	float * group_3D_table;
	float * wx_r_table;

	float * tmp = new float[NWien];
	float * kaiser_window = new float[kWien_2];
	float * coef_norm = new float[kWien_2];
	float * coef_norm_inv = new float[kWien_2];
	if (!tmp || !kaiser_window || !coef_norm || !coef_norm_inv)
	{
		if (!tmp)
		{
			delete[] tmp;
			tmp = NULL;
		}
		if (!kaiser_window)
		{
			delete[] kaiser_window;
			kaiser_window = NULL;
		}
		if (!coef_norm)
		{
			delete[] coef_norm;
			coef_norm = NULL;
		}
		if (!coef_norm_inv)
		{
			delete[] coef_norm_inv;
			coef_norm_inv = NULL;
		}
		CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_2nd_step!\n");
		return NULL;
	}

	preProcess(kaiser_window, coef_norm, coef_norm_inv, kWien);

	// For aggregation part
	float * denominator = new float[width * height * chnls]();
	float * numerator = new float[width * height * chnls]();
	if (!denominator || !numerator)
	{
		if (!denominator)
		{
			delete[] denominator;
			denominator = NULL;
		}
		if (!numerator)
		{
			delete[] numerator;
			numerator = NULL;
		}
		CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_2nd_step!\n");
		return NULL;
	}

	// Precompute Bloc-Matching
	unsigned int ** patch_table;
	unsigned int * patch_table_size;
	precompute_BM(patch_table, patch_table_size, img_basic, width, height, kWien, NWien, nWien, pWien, tauMatch);

	// Preprocessing of Bior table
	float * lpd = new float[10];
	float * hpd = new float[10];
	float * lpr = new float[10];
	float * hpr = new float[10];
	if (!lpd || !hpd || !lpr || !hpr)
	{
		if (!lpd)
		{
			delete[] lpd;
			lpd = NULL;
		}
		if (!hpd)
		{
			delete[] hpd;
			hpd = NULL;
		}
		if (!lpr)
		{
			delete[] lpr;
			lpr = NULL;
		}
		if (!hpr)
		{
			delete[] hpr;
			hpr = NULL;
		}
		CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_2nd_step!\n");
		return NULL;
	}
	bior15_coef(lpd, hpd, lpr, hpr);


	float * table_2D_img = new float[(2 * nWien + 1) * width * chnls * kWien_2]();
	float * table_2D_est = new float[(2 * nWien + 1) * width * chnls * kWien_2]();
	if (!table_2D_img || !table_2D_est)
	{
		if (!table_2D_img)
		{
			delete[] table_2D_img;
			table_2D_img;
		}
		if (!table_2D_est)
		{
			delete[] table_2D_est;
			table_2D_est = NULL;
		}
		CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_2nd_step!\n");
		return NULL;
	}

	// Loop on i_r
	for (unsigned int ind_i = 0; ind_i < row_ind_size; ind_i++)
	{
		const unsigned int i_r = row_ind[ind_i];

		// Update of DCT_table_2D
		if (tau_2D == DCT)
		{
			dct_2d_process(table_2D_img, img_noisy, plan_2d_for_1, plan_2d_for_2,
				nWien, width, height, chnls, kWien, i_r, pWien, coef_norm,
				row_ind[0], row_ind[row_ind_size - 1]);
			dct_2d_process(table_2D_est, img_basic, plan_2d_for_1, plan_2d_for_2,
				nWien, width, height, chnls, kWien, i_r, pWien, coef_norm,
				row_ind[0], row_ind[row_ind_size - 1]);
		}
		else if (tau_2D == BIOR)
		{
			bior_2d_process(table_2D_img, img_noisy, nWien, width, height,
				chnls, kWien, i_r, pWien, row_ind[0], row_ind[row_ind_size - 1], lpd, hpd);
			bior_2d_process(table_2D_est, img_basic, nWien, width, height,
				chnls, kWien, i_r, pWien, row_ind[0], row_ind[row_ind_size - 1], lpd, hpd);
		}

		wx_r_table = new float[chnls * column_ind_size];
		if (!wx_r_table)
		{
			CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_2nd_step!\n");
			delete[] wx_r_table;
			wx_r_table = NULL;
		}

		unsigned int sum_nSx_r = 0;
		for (unsigned int ind_j = 0; ind_j < column_ind_size; ind_j++)
		{
			const unsigned int j_r = column_ind[ind_j];
			const unsigned int k_r = i_r * width + j_r;
			sum_nSx_r += patch_table_size[k_r];
		}
		unsigned int group_3D_table_size = chnls * sum_nSx_r * kWien_2;
		group_3D_table = new float[group_3D_table_size];
		if (!group_3D_table)
		{
			CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_2nd_step!\n");
			delete[] group_3D_table;
			group_3D_table = NULL;
		}
		unsigned int sum = 0;

		// Loop on j_r
		for (unsigned int ind_j = 0; ind_j < column_ind_size; ind_j++)
		{
			// Initialization
			const unsigned int j_r = column_ind[ind_j];
			const unsigned int k_r = i_r * width + j_r;

			// Number of similar patches
			const unsigned int nSx_r = patch_table_size[k_r];

			// Build of the 3D group
			float * group_3D_est = new float[chnls * nSx_r * kWien_2]();
			float * group_3D_img = new float[chnls * nSx_r * kWien_2]();
			float * group_3D_table_ = new float[chnls * nSx_r * kWien_2];
			if (!group_3D_est || !group_3D_img || !group_3D_table_)
			{
				if (!group_3D_est)
				{
					delete[] group_3D_est;
					group_3D_est = NULL;
				}
				if (!group_3D_img)
				{
					delete[] group_3D_img;
					group_3D_img = NULL;
				}
				if (!group_3D_table_)
				{
					delete[] group_3D_table_;
					group_3D_table_ = NULL;
				}
				CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_2nd_step!\n");
				return NULL;
			}
			for (unsigned int c = 0; c < chnls; c++)
				for (unsigned int n = 0; n < nSx_r; n++)
				{
					const unsigned int ind = patch_table[k_r][n] + (nWien - i_r) * width;
					for (unsigned int k = 0; k < kWien_2; k++)
					{
						group_3D_est[n + k * nSx_r + c * kWien_2 * nSx_r] =
							table_2D_est[k + ind * kWien_2 + c * kWien_2 * (2 * nWien + 1) * width];
						group_3D_img[n + k * nSx_r + c * kWien_2 * nSx_r] =
							table_2D_img[k + ind * kWien_2 + c * kWien_2 * (2 * nWien + 1) * width];
					}
				}

			// Wiener filtering of the 3D group
			float * weight_table = new float[chnls];
			if (!weight_table)
			{
				CImageUtility::showErrMsg("Fail to allocate buffer in bm3d_2nd_step!\n");
				delete[] weight_table;
				weight_table = NULL;
			}
			wiener_filtering_hadamard(group_3D_img, group_3D_est, tmp, nSx_r, kWien,
				chnls, sigma_table, weight_table, !useSD);

			// 3D weighting using Standard Deviation
			if (useSD)
			{
				sd_weighting(group_3D_est, nSx_r, kWien, chnls, weight_table);
			}

			// Save the 3D group. The DCT 2D inverse will be done after.
			for (unsigned int c = 0; c < chnls; c++)
			{
				for (unsigned int n = 0; n < nSx_r; n++)
				{
					for (unsigned int k = 0; k < kWien_2; k++)
					{
						unsigned int index = k + n * kWien_2 + c * kWien_2 * nSx_r;
						group_3D_table_[index] = group_3D_est[n + k * nSx_r + c * kWien_2 * nSx_r];
					}
				}
			}

			unsigned int sz = chnls * nSx_r * kWien_2;
			for (unsigned int i = 0; i < sz; i++)
			{
				group_3D_table[i + sum] = group_3D_table_[i];
			}
			sum += sz;

			// Save weighting
			for (unsigned int c = 0; c < chnls; c++)
			{
				wx_r_table[chnls * ind_j + c] = weight_table[c];
			}

			delete[] group_3D_table_;
			delete[] weight_table;
			delete[] group_3D_img;
			delete[] group_3D_est;

			group_3D_table_ = NULL;
			weight_table = NULL;
			group_3D_img = NULL;
			group_3D_est = NULL;
		} // End of loop on j_r

		//  Apply 2D dct inverse
		if (tau_2D == DCT)
		{
			dct_2d_inverse(group_3D_table, kWien, NWien * chnls * column_ind_size, group_3D_table_size,
				coef_norm_inv, plan_2d_inv);
		}
		else if (tau_2D == BIOR)
		{
			bior_2d_inverse(group_3D_table, kWien, lpr, hpr, group_3D_table_size);
		}

		// Registration of the weighted estimation
		unsigned int dec = 0;
		for (unsigned int ind_j = 0; ind_j < column_ind_size; ind_j++)
		{
			const unsigned int j_r = column_ind[ind_j];
			const unsigned int k_r = i_r * width + j_r;
			const unsigned int nSx_r = patch_table_size[k_r];
			for (unsigned int c = 0; c < chnls; c++)
			{
				for (unsigned int n = 0; n < nSx_r; n++)
				{
					const unsigned int k = patch_table[k_r][n] + c * width * height;
					for (unsigned int p = 0; p < kWien; p++)
						for (unsigned int q = 0; q < kWien; q++)
						{
							const unsigned int ind = k + p * width + q;
							numerator[ind] += kaiser_window[p * kWien + q]
								* wx_r_table[c + ind_j * chnls]
								* group_3D_table[p * kWien + q + n * kWien_2
								+ c * kWien_2 * nSx_r + dec];
							denominator[ind] += kaiser_window[p * kWien + q]
								* wx_r_table[c + ind_j * chnls];
						}
				}
			}
			dec += nSx_r * chnls * kWien_2;
		}

		delete[] group_3D_table;
		delete[] wx_r_table;

		group_3D_table = NULL;
		wx_r_table = NULL;

	} // End of loop on i_r

	delete[] table_2D_img;
	delete[] table_2D_est;
	delete[] hpr;
	delete[] lpr;
	delete[] hpd;
	delete[] lpd;
	delete[] coef_norm_inv;
	delete[] coef_norm;
	delete[] kaiser_window;
	delete[] tmp;
	delete[] sigma_table;

	table_2D_img = NULL;
	table_2D_est = NULL;
	hpr = NULL;
	lpr = NULL;
	hpd = NULL;
	lpd = NULL;
	coef_norm_inv = NULL;
	coef_norm = NULL;
	kaiser_window = NULL;
	tmp = NULL;
	sigma_table = NULL;

	// Final reconstruction
	for (unsigned int k = 0; k < width * height * chnls; k++)
	{
		img_denoised[k] = numerator[k] / denominator[k];
		if (denominator[k] == 0.0)
		{
			img_denoised[k] = img_noisy[k];
		}
	}
	delete[] numerator;
	delete[] denominator;

	numerator = NULL;
	denominator = NULL;

    IplImage * iplImage_denoised = transfer_buffer2iplImage(img_denoised, width, height, chnls, false);
    return iplImage_denoised;
}

//
// @brief Precompute a 2D DCT transform on all patches contained in
//        a part of the image.
//
// @param DCT_table_2D : will contain the 2d DCT transform for all
//        chosen patches;
// @param img : image on which the 2d DCT will be processed;
// @param plan_1, plan_2 : for convenience. Used by fftw;
// @param nHW : size of the boundary around img;
// @param width, height, chnls: size of img;
// @param kHW : size of patches (kHW x kHW);
// @param i_r: current index of the reference patches;
// @param step: space in pixels between two references patches;
// @param coef_norm : normalization coefficients of the 2D DCT;
// @param i_min (resp. i_max) : minimum (resp. maximum) value
//        for i_r. In this case the whole 2d transform is applied
//        on every patches. Otherwise the precomputed 2d DCT is re-used
//        without processing it.
//
void dct_2d_process(float * DCT_table_2D, float * const img, fftwf_plan * plan_1, fftwf_plan * plan_2, const unsigned int nHW,
	const unsigned int width, const unsigned int height, const unsigned int chnls, const unsigned int kHW, const unsigned int i_r,
	const unsigned int step, float * const coef_norm, const unsigned int i_min, const unsigned int i_max)
{
	// Declarations
	const unsigned int kHW_2 = kHW * kHW;
	const unsigned int size = chnls * kHW_2 * width * (2 * nHW + 1);

	// If i_r == ns, then we have to process all DCT
	if (i_r == i_min || i_r == i_max)
	{
		// Allocating Memory
		float* vec = (float*)fftwf_malloc(size * sizeof(float));
		float* dct = (float*)fftwf_malloc(size * sizeof(float));

		for (unsigned int c = 0; c < chnls; c++)
		{
			const unsigned int dc = c * width * height;
			const unsigned int dc_p = c * kHW_2 * width * (2 * nHW + 1);
			for (unsigned int i = 0; i < 2 * nHW + 1; i++)
				for (unsigned int j = 0; j < width - kHW; j++)
					for (unsigned int p = 0; p < kHW; p++)
						for (unsigned int q = 0; q < kHW; q++)
							vec[p * kHW + q + dc_p + (i * width + j) * kHW_2] =
							img[dc + (i_r + i - nHW + p) * width + j + q];
		}

		// Process of all DCTs
		fftwf_execute_r2r(*plan_1, vec, dct);
		fftwf_free(vec);

		// Getting the result
		for (unsigned int c = 0; c < chnls; c++)
		{
			const unsigned int dc = c * kHW_2 * width * (2 * nHW + 1);
			const unsigned int dc_p = c * kHW_2 * width * (2 * nHW + 1);
			for (unsigned int i = 0; i < 2 * nHW + 1; i++)
				for (unsigned int j = 0; j < width - kHW; j++)
					for (unsigned int k = 0; k < kHW_2; k++)
						DCT_table_2D[dc + (i * width + j) * kHW_2 + k] =
						dct[dc_p + (i * width + j) * kHW_2 + k] * coef_norm[k];
		}
		fftwf_free(dct);
	}
	else
	{
		const unsigned int ds = step * width * kHW_2;

		// Re-use of DCT already processed
		for (unsigned int c = 0; c < chnls; c++)
		{
			unsigned int dc = c * width * (2 * nHW + 1) * kHW_2;
			for (unsigned int i = 0; i < 2 * nHW + 1 - step; i++)
				for (unsigned int j = 0; j < width - kHW; j++)
					for (unsigned int k = 0; k < kHW_2; k++)
						DCT_table_2D[k + (i * width + j) * kHW_2 + dc] =
						DCT_table_2D[k + (i * width + j) * kHW_2 + dc + ds];
		}

		// Compute the new DCT
		float* vec = (float*)fftwf_malloc(chnls * kHW_2 * step * width * sizeof(float));
		float* dct = (float*)fftwf_malloc(chnls * kHW_2 * step * width * sizeof(float));

		for (unsigned int c = 0; c < chnls; c++)
		{
			const unsigned int dc = c * width * height;
			const unsigned int dc_p = c * kHW_2 * width * step;
			for (unsigned int i = 0; i < step; i++)
				for (unsigned int j = 0; j < width - kHW; j++)
					for (unsigned int p = 0; p < kHW; p++)
						for (unsigned int q = 0; q < kHW; q++)
							vec[p * kHW + q + dc_p + (i * width + j) * kHW_2] =
							img[(p + i + 2 * nHW + 1 - step + i_r - nHW)
							* width + j + q + dc];
		}

		// Process of all DCTs
		fftwf_execute_r2r(*plan_2, vec, dct);
		fftwf_free(vec);

		// Getting the result
		for (unsigned int c = 0; c < chnls; c++)
		{
			const unsigned int dc = c * kHW_2 * width * (2 * nHW + 1);
			const unsigned int dc_p = c * kHW_2 * width * step;
			for (unsigned int i = 0; i < step; i++)
				for (unsigned int j = 0; j < width - kHW; j++)
					for (unsigned int k = 0; k < kHW_2; k++)
						DCT_table_2D[dc + ((i + 2 * nHW + 1 - step) * width + j) * kHW_2 + k] =
						dct[dc_p + (i * width + j) * kHW_2 + k] * coef_norm[k];
		}
		fftwf_free(dct);
	}
}

//
// @brief Precompute a 2D bior1.5 transform on all patches contained in
//        a part of the image.
//
// @param bior_table_2D : will contain the 2d bior1.5 transform for all
//        chosen patches;
// @param img : image on which the 2d transform will be processed;
// @param nHW : size of the boundary around img;
// @param width, height, chnls: size of img;
// @param kHW : size of patches (kHW x kHW). MUST BE A POWER OF 2 !!!
// @param i_r: current index of the reference patches;
// @param step: space in pixels between two references patches;
// @param i_min (resp. i_max) : minimum (resp. maximum) value
//        for i_r. In this case the whole 2d transform is applied
//        on every patches. Otherwise the precomputed 2d DCT is re-used
//        without processing it;
// @param lpd : low pass filter of the forward bior1.5 2d transform;
// @param hpd : high pass filter of the forward bior1.5 2d transform.

void bior_2d_process(float * bior_table_2D, float * const img, const unsigned int nHW, const unsigned int width, const unsigned int height,
	const unsigned int chnls, const unsigned int kHW, const unsigned int i_r, const unsigned int step, const unsigned int i_min,
	const unsigned int i_max, float * lpd, float * hpd)
{
	// Declarations
	const unsigned int kHW_2 = kHW * kHW;

	// If i_r == ns, then we have to process all Bior1.5 transforms
	if (i_r == i_min || i_r == i_max)
	{
		for (unsigned int c = 0; c < chnls; c++)
		{
			const unsigned int dc = c * width * height;
			const unsigned int dc_p = c * kHW_2 * width * (2 * nHW + 1);
			for (unsigned int i = 0; i < 2 * nHW + 1; i++)
				for (unsigned int j = 0; j < width - kHW; j++)
				{
					bior_2d_forward(img, bior_table_2D, kHW, dc +
						(i_r + i - nHW) * width + j, width,
						dc_p + (i * width + j) * kHW_2, lpd, hpd);
				}
		}
	}
	else
	{
		const unsigned int ds = step * width * kHW_2;

		// Re-use of Bior1.5 already processed
		for (unsigned int c = 0; c < chnls; c++)
		{
			unsigned int dc = c * width * (2 * nHW + 1) * kHW_2;
			for (unsigned int i = 0; i < 2 * nHW + 1 - step; i++)
				for (unsigned int j = 0; j < width - kHW; j++)
					for (unsigned int k = 0; k < kHW_2; k++)
						bior_table_2D[k + (i * width + j) * kHW_2 + dc] =
						bior_table_2D[k + (i * width + j) * kHW_2 + dc + ds];
		}

		// Compute the new Bior
		for (unsigned int c = 0; c < chnls; c++)
		{
			const unsigned int dc = c * width * height;
			const unsigned int dc_p = c * kHW_2 * width * (2 * nHW + 1);
			for (unsigned int i = 0; i < step; i++)
				for (unsigned int j = 0; j < width - kHW; j++)
				{
					bior_2d_forward(img, bior_table_2D, kHW, dc +
						(i + 2 * nHW + 1 - step + i_r - nHW) * width + j,
						width, dc_p + ((i + 2 * nHW + 1 - step)
						* width + j) * kHW_2, lpd, hpd);
				}
		}
	}
}

//
// @brief HT filtering using Welsh-Hadamard transform (do only third
//        dimension transform, Hard Thresholding and inverse transform).
//
// @param group_3D : contains the 3D block for a reference patch;
// @param tmp: allocated vector used in Hadamard transform for convenience;
// @param nSx_r : number of similar patches to a reference one;
// @param kHW : size of patches (kHW x kHW);
// @param chnls : number of channels of the image;
// @param sigma_table : contains value of noise for each channel;
// @param lambdaHard3D : value of thresholding;
// @param weight_table: the weighting of this 3D group for each channel;
// @param doWeight: if true process the weighting, do nothing
//        otherwise.
//
// @return none.
//
void ht_filtering_hadamard(float * group_3D, float * tmp, const unsigned int nSx_r, const unsigned int kHard, const unsigned int chnls,
	float * const sigma_table, const float lambdaHard3D, float * weight_table, const bool doWeight)
{
	// Declarations
	const unsigned int kHard_2 = kHard * kHard;
	for (unsigned int c = 0; c < chnls; c++)
		weight_table[c] = 0.0f;
	const float coef_norm = sqrtf((float)nSx_r);
	const float coef = 1.0f / (float)nSx_r;

	// Process the Welsh-Hadamard transform on the 3rd dimension
	for (unsigned int n = 0; n < kHard_2 * chnls; n++)
		hadamard_transform(group_3D, tmp, nSx_r, n * nSx_r);

	// Hard Thresholding
	for (unsigned int c = 0; c < chnls; c++)
	{
		const unsigned int dc = c * nSx_r * kHard_2;
		const float T = lambdaHard3D * sigma_table[c] * coef_norm;
		for (unsigned int k = 0; k < kHard_2 * nSx_r; k++)
		{
			if (fabs(group_3D[k + dc]) > T)
				weight_table[c]++;
			else
				group_3D[k + dc] = 0.0f;
		}
	}

	// Process of the Welsh-Hadamard inverse transform
	for (unsigned int n = 0; n < kHard_2 * chnls; n++)
		hadamard_transform(group_3D, tmp, nSx_r, n * nSx_r);

	/// Recoding group_3D.size() = chnls * nSx_r * kHard * kHard; 
	//for (unsigned int k = 0; k < group_3D.size(); k++)
	unsigned int group_3D_size = chnls * nSx_r * kHard * kHard;
	for (unsigned int k = 0; k < group_3D_size; k++)
		group_3D[k] *= coef;

	// Weight for aggregation
	if (doWeight)
		for (unsigned int c = 0; c < chnls; c++)
			weight_table[c] = (weight_table[c] > 0.0f ? 1.0f / (float)
			(sigma_table[c] * sigma_table[c] * weight_table[c]) : 1.0f);
}

//
// @brief Wiener filtering using Hadamard transform.
//
// @param group_3D_img : contains the 3D block built on img_noisy;
// @param group_3D_est : contains the 3D block built on img_basic;
// @param tmp: allocated vector used in hadamard transform for convenience;
// @param nSx_r : number of similar patches to a reference one;
// @param kWien : size of patches (kWien x kWien);
// @param chnls : number of channels of the image;
// @param sigma_table : contains value of noise for each channel;
// @param weight_table: the weighting of this 3D group for each channel;
// @param doWeight: if true process the weighting, do nothing
//        otherwise.
//
// @return none.
//
void wiener_filtering_hadamard(float * group_3D_img, float * group_3D_est, float * tmp, const unsigned int nSx_r, const unsigned int kWien,
	const unsigned int chnls, float * const sigma_table, float * weight_table, const bool doWeight)
{
	// Declarations
	const unsigned int kWien_2 = kWien * kWien;
	const float coef = 1.0f / (float)nSx_r;

	for (unsigned int c = 0; c < chnls; c++)
		weight_table[c] = 0.0f;

	// Process the Welsh-Hadamard transform on the 3rd dimension
	for (unsigned int n = 0; n < kWien_2 * chnls; n++)
	{
		hadamard_transform(group_3D_img, tmp, nSx_r, n * nSx_r);
		hadamard_transform(group_3D_est, tmp, nSx_r, n * nSx_r);
	}

	// Wiener Filtering
	for (unsigned int c = 0; c < chnls; c++)
	{
		const unsigned int dc = c * nSx_r * kWien_2;
		for (unsigned int k = 0; k < kWien_2 * nSx_r; k++)
		{
			float value = group_3D_est[dc + k] * group_3D_est[dc + k] * coef;
			value /= (value + sigma_table[c] * sigma_table[c]);
			group_3D_est[k + dc] = group_3D_img[k + dc] * value * coef;
			weight_table[c] += value;
		}
	}

	// Process of the Welsh-Hadamard inverse transform
	for (unsigned int n = 0; n < kWien_2 * chnls; n++)
		hadamard_transform(group_3D_est, tmp, nSx_r, n * nSx_r);

	// Weight for aggregation
	if (doWeight)
		for (unsigned int c = 0; c < chnls; c++)
			weight_table[c] = (weight_table[c] > 0.0f ? 1.0f / (float)
			(sigma_table[c] * sigma_table[c] * weight_table[c]) : 1.0f);
}

//
// @brief Apply 2D dct inverse to a lot of patches.
//
// @param group_3D_table: contains a huge number of patches;
// @param kHW : size of patch;
// @param coef_norm_inv: contains normalization coefficients;
// @param plan : for convenience. Used by fftw.
//
// @return none.
//
void dct_2d_inverse(float * group_3D_table, const unsigned int kHW, const unsigned int N, const unsigned int group_3D_table_size,
	float * const coef_norm_inv, fftwf_plan * plan)
{
	// Declarations
	const unsigned int kHW_2 = kHW * kHW;
	const unsigned int size = kHW_2 * N;
	const unsigned int Ns = group_3D_table_size / kHW_2;

	// Allocate Memory
	float* vec = (float*)fftwf_malloc(size * sizeof(float));
	float* dct = (float*)fftwf_malloc(size * sizeof(float));

	// Normalization
	for (unsigned int n = 0; n < Ns; n++)
		for (unsigned int k = 0; k < kHW_2; k++)
			dct[k + n * kHW_2] = group_3D_table[k + n * kHW_2] * coef_norm_inv[k];

	// 2D dct inverse
	fftwf_execute_r2r(*plan, dct, vec);
	fftwf_free(dct);

	// Getting the result + normalization
	const float coef = 1.0f / (float)(kHW * 2);
	for (unsigned int k = 0; k < group_3D_table_size; k++)
		group_3D_table[k] = coef * vec[k];

	// Free Memory
	fftwf_free(vec);
}

void bior_2d_inverse(
	float * group_3D_table,
    const unsigned int kHW,
    float * const lpr,
    float * const hpr,
    const unsigned int group_3D_table_size
	){
	// Declarations
	const unsigned int kHW_2 = kHW * kHW;
	const unsigned int N = group_3D_table_size / kHW_2;

	// Bior process
	for (unsigned int n = 0; n < N; n++)
		bior_2d_inverse(group_3D_table, kHW, n * kHW_2, lpr, hpr);
}

//
// @brief Preprocess
//
// @param kaiser_window[kHW * kHW]: Will contain values of a Kaiser Window;
// @param coef_norm: Will contain values used to normalize the 2D DCT;
// @param coef_norm_inv: Will contain values used to normalize the 2D DCT;
// @param bior1_5_for: will contain coefficients for the bior1.5 forward transform
// @param bior1_5_inv: will contain coefficients for the bior1.5 inverse transform
// @param kHW: size of patches (need to be 8 or 12).
//
// @return none.
//
void preProcess(float * kaiserWindow, float * coef_norm, float * coef_norm_inv, const unsigned int kHW)
{
	// Kaiser Window coefficients
	if (kHW == 8)
	{
		// First quarter of the matrix
		kaiserWindow[0 + kHW * 0] = 0.1924f; kaiserWindow[0 + kHW * 1] = 0.2989f; kaiserWindow[0 + kHW * 2] = 0.3846f; kaiserWindow[0 + kHW * 3] = 0.4325f;
		kaiserWindow[1 + kHW * 0] = 0.2989f; kaiserWindow[1 + kHW * 1] = 0.4642f; kaiserWindow[1 + kHW * 2] = 0.5974f; kaiserWindow[1 + kHW * 3] = 0.6717f;
		kaiserWindow[2 + kHW * 0] = 0.3846f; kaiserWindow[2 + kHW * 1] = 0.5974f; kaiserWindow[2 + kHW * 2] = 0.7688f; kaiserWindow[2 + kHW * 3] = 0.8644f;
		kaiserWindow[3 + kHW * 0] = 0.4325f; kaiserWindow[3 + kHW * 1] = 0.6717f; kaiserWindow[3 + kHW * 2] = 0.8644f; kaiserWindow[3 + kHW * 3] = 0.9718f;

		// Completing the rest of the matrix by symmetry
		for (unsigned int i = 0; i < kHW / 2; i++)
			for (unsigned int j = kHW / 2; j < kHW; j++)
				kaiserWindow[i + kHW * j] = kaiserWindow[i + kHW * (kHW - j - 1)];

		for (unsigned int i = kHW / 2; i < kHW; i++)
			for (unsigned int j = 0; j < kHW; j++)
				kaiserWindow[i + kHW * j] = kaiserWindow[kHW - i - 1 + kHW * j];
	}
	else if (kHW == 12)
	{
		// First quarter of the matrix
		kaiserWindow[0 + kHW * 0] = 0.1924f; kaiserWindow[0 + kHW * 1] = 0.2615f; kaiserWindow[0 + kHW * 2] = 0.3251f; kaiserWindow[0 + kHW * 3] = 0.3782f;  kaiserWindow[0 + kHW * 4] = 0.4163f;  kaiserWindow[0 + kHW * 5] = 0.4362f;
		kaiserWindow[1 + kHW * 0] = 0.2615f; kaiserWindow[1 + kHW * 1] = 0.3554f; kaiserWindow[1 + kHW * 2] = 0.4419f; kaiserWindow[1 + kHW * 3] = 0.5139f;  kaiserWindow[1 + kHW * 4] = 0.5657f;  kaiserWindow[1 + kHW * 5] = 0.5927f;
		kaiserWindow[2 + kHW * 0] = 0.3251f; kaiserWindow[2 + kHW * 1] = 0.4419f; kaiserWindow[2 + kHW * 2] = 0.5494f; kaiserWindow[2 + kHW * 3] = 0.6390f;  kaiserWindow[2 + kHW * 4] = 0.7033f;  kaiserWindow[2 + kHW * 5] = 0.7369f;
		kaiserWindow[3 + kHW * 0] = 0.3782f; kaiserWindow[3 + kHW * 1] = 0.5139f; kaiserWindow[3 + kHW * 2] = 0.6390f; kaiserWindow[3 + kHW * 3] = 0.7433f;  kaiserWindow[3 + kHW * 4] = 0.8181f;  kaiserWindow[3 + kHW * 5] = 0.8572f;
		kaiserWindow[4 + kHW * 0] = 0.4163f; kaiserWindow[4 + kHW * 1] = 0.5657f; kaiserWindow[4 + kHW * 2] = 0.7033f; kaiserWindow[4 + kHW * 3] = 0.8181f;  kaiserWindow[4 + kHW * 4] = 0.9005f;  kaiserWindow[4 + kHW * 5] = 0.9435f;
		kaiserWindow[5 + kHW * 0] = 0.4362f; kaiserWindow[5 + kHW * 1] = 0.5927f; kaiserWindow[5 + kHW * 2] = 0.7369f; kaiserWindow[5 + kHW * 3] = 0.8572f;  kaiserWindow[5 + kHW * 4] = 0.9435f;  kaiserWindow[5 + kHW * 5] = 0.9885f;

		// Completing the rest of the matrix by symmetry
		for (unsigned int i = 0; i < kHW / 2; i++)
			for (unsigned int j = kHW / 2; j < kHW; j++)
				kaiserWindow[i + kHW * j] = kaiserWindow[i + kHW * (kHW - j - 1)];

		for (unsigned int i = kHW / 2; i < kHW; i++)
			for (unsigned int j = 0; j < kHW; j++)
				kaiserWindow[i + kHW * j] = kaiserWindow[kHW - i - 1 + kHW * j];
	}
	else
		for (unsigned int k = 0; k < kHW * kHW; k++)
			kaiserWindow[k] = 1.0f;

	// Coefficient of normalization for DCT II and DCT II inverse
	const float coef = 0.5f / ((float)(kHW));
	for (unsigned int i = 0; i < kHW; i++)
		for (unsigned int j = 0; j < kHW; j++)
		{
			if (i == 0 && j == 0)
			{
				coef_norm[i * kHW + j] = 0.5f * coef;
				coef_norm_inv[i * kHW + j] = 2.0f;
			}
			else if (i * j == 0)
			{
				coef_norm[i * kHW + j] = (float)SQRT2_INV * coef;
				coef_norm_inv[i * kHW + j] = (float)SQRT2;
			}
			else
			{
				coef_norm[i * kHW + j] = 1.0f * coef;
				coef_norm_inv[i * kHW + j] = 1.0f;
			}
		}
}

//
// @brief Precompute Bloc Matching (distance inter-patches)
//
// @param patch_table: for each patch in the image, will contain
// all coordonnate of its similar patches
// @param img: noisy image on which the distance is computed
// @param width, height: size of img
// @param kHW: size of patch
// @param NHW: maximum similar patches wanted
// @param nHW: size of the boundary of img
// @param tauMatch: threshold used to determinate similarity between
//        patches
//
// @return none.
//
void precompute_BM(unsigned int ** &patch_table, unsigned int * &patch_table_size, float * const &img, const unsigned int width,
	const unsigned int height, const unsigned int kHW, const unsigned int NHW, const unsigned int nHW, const unsigned int pHW,
	const float tauMatch)
{
	// Declarations
	const unsigned int Ns = 2 * nHW + 1;
	const float threshold = tauMatch * kHW * kHW;
	float * diff_table = new float[width * height];	

	float ** sum_table = new float*[(nHW + 1) * Ns];
	for (unsigned int i = 0; i < (nHW + 1) * Ns; ++i)
	{
		sum_table[i] = new float[width * height];
	}
	for (unsigned int j = 0; j < (nHW + 1) * Ns; ++j)
	{
		for (unsigned int i = 0; i < width*height; ++i)
		{
			sum_table[j][i] = 2 * threshold;
		}
	}
	patch_table = new unsigned int *[width*height];
	patch_table_size = new unsigned int[width*height];

	unsigned int * row_ind;
	unsigned int row_ind_size;
	ind_initialize(row_ind, height - kHW + 1, nHW, pHW, row_ind_size);
	unsigned int * column_ind;
	unsigned int column_ind_size;
	ind_initialize(column_ind, width - kHW + 1, nHW, pHW, column_ind_size);
	// For each possible distance, precompute inter-patches distance
	for (unsigned int di = 0; di <= nHW; di++)	
		for (unsigned int dj = 0; dj < Ns; dj++)	// Ns*2+1
		{
			const int dk = (int)(di * width + dj) - (int)nHW; 
			const unsigned int ddk = di * Ns + dj;	

			// Process the image containing the square distance between pixels
			for (unsigned int i = nHW; i < height - nHW; i++)	
			{
				unsigned int k = i * width + nHW;
				for (unsigned int j = nHW; j < width - nHW; j++, k++)
					diff_table[k] = (img[k + dk] - img[k]) * (img[k + dk] - img[k]);
			}

			// Compute the sum for each patches, using the method of the integral images
			const unsigned int dn = nHW * width + nHW; 
			// 1st patch, top left corner
			float value = 0.0f;
			for (unsigned int p = 0; p < kHW; p++)
			{
				unsigned int pq = p * width + dn;
				for (unsigned int q = 0; q < kHW; q++, pq++)
					value += diff_table[pq];
			}
			sum_table[ddk][dn] = value;

			// 1st row, top
			for (unsigned int j = nHW + 1; j < width - nHW; j++)
			{
				const unsigned int ind = nHW * width + j - 1;
				float sum = sum_table[ddk][ind];
				for (unsigned int p = 0; p < kHW; p++)
					sum += diff_table[ind + p * width + kHW] - diff_table[ind + p * width];
				sum_table[ddk][ind + 1] = sum;
			}

			// General case
			for (unsigned int i = nHW + 1; i < height - nHW; i++)
			{
				const unsigned int ind = (i - 1) * width + nHW;
				float sum = sum_table[ddk][ind];
				// 1st column, left
				for (unsigned int q = 0; q < kHW; q++)
					sum += diff_table[ind + kHW * width + q] - diff_table[ind + q];
				sum_table[ddk][ind + width] = sum;

				// Other columns
				unsigned int k = i * width + nHW + 1;
				unsigned int pq = (i + kHW - 1) * width + kHW - 1 + nHW + 1;
				for (unsigned int j = nHW + 1; j < width - nHW; j++, k++, pq++)
				{
					sum_table[ddk][k] =
						sum_table[ddk][k - 1]
						+ sum_table[ddk][k - width]
						- sum_table[ddk][k - 1 - width]
						+ diff_table[pq]
						- diff_table[pq - kHW]
						- diff_table[pq - kHW * width]
						+ diff_table[pq - kHW - kHW * width];
				}

			}
		}
	delete[] diff_table;
	diff_table = NULL;
	// Precompute Bloc Matching
	TD * table_distance;

	for (unsigned int ind_i = 0; ind_i < row_ind_size; ind_i++)
	{
		for (unsigned int ind_j = 0; ind_j < column_ind_size; ind_j++)
		{
			// Initialization
			const unsigned int k_r = row_ind[ind_i] * width + column_ind[ind_j];

			unsigned int table_distance_size = 0;
			unsigned int * table_distance_size_arr = new unsigned int[4 * nHW + 2];
			if (!table_distance_size_arr)
			{
				delete[] table_distance_size_arr;
				table_distance_size_arr = NULL;
			}
			for (int dj = -(int)nHW; dj <= (int)nHW; dj++)
			{
				table_distance_size_arr[2 * (dj + nHW)] = table_distance_size;
				for (int di = 0; di <= (int)nHW; di++)
				{
					if (sum_table[dj + nHW + di * Ns][k_r] < threshold)
					{
						table_distance_size++;
					}
				}
				table_distance_size_arr[2 * (dj + nHW) + 1] = table_distance_size;
				for (int di = -(int)nHW; di < 0; di++)
				{
					if (sum_table[-dj + nHW + (-di) * Ns][k_r] < threshold)
					{
						table_distance_size++;
					}
				}
			}
			table_distance = new TD[table_distance_size];
			if (!table_distance)
			{
				delete[] table_distance;
				table_distance = NULL;
			}
			// Threshold distances in order to keep similar patches
			for (int dj = -(int)nHW; dj <= (int)nHW; dj++)
			{
				int i = 0;
				int j = 0;
				for (int di = 0; di <= (int)nHW; di++)
				{
					if (sum_table[dj + nHW + di * Ns][k_r] < threshold)
					{
						float f = sum_table[dj + nHW + di * Ns][k_r];
						unsigned int u = k_r + di * width + dj;
						table_distance[i + table_distance_size_arr[2 * (dj + nHW)]] = TD(f, u);
						i++;

					}
				}

				for (int di = -(int)nHW; di < 0; di++)
				{
					if (sum_table[-dj + nHW + (-di) * Ns][k_r] < threshold)
					{
						float f = sum_table[-dj + nHW + (-di) * Ns][k_r + di * width + dj];
						unsigned int u = k_r + di * width + dj;
						table_distance[j + table_distance_size_arr[2 * (dj + nHW) + 1]] = TD(f, u);
						j++;
					}
				}
			}

			// We need a power of 2 for the number of similar patches,
			// because of the Welsh-Hadamard transform on the third dimension.
			// We assume that NHW is already a power of 2
			const unsigned int nSx_r = (NHW > table_distance_size ?
				closest_power_of_2(table_distance_size) : NHW); // nPatcWidth
			if (nSx_r == 1)
			{
				patch_table[k_r] = new unsigned int[nSx_r + 1];
				patch_table_size[k_r] = nSx_r + 1;
			}
			else
			{
				patch_table[k_r] = new unsigned int[nSx_r];
				patch_table_size[k_r] = nSx_r;
			}

			// Sort patches according to their distance to the reference one
			SelectSort(table_distance, table_distance_size);

			// Keep a maximum of NHW similar patches
			for (unsigned n = 0; n < nSx_r; n++) 
			{
				patch_table[k_r][n] = table_distance[n].u;
			}

			// To avoid problem
			if (nSx_r == 1) 
			{
				patch_table[k_r][nSx_r] = table_distance[0].u;
			}
			delete[] table_distance;
			delete[] table_distance_size_arr;

			table_distance = NULL;
			table_distance_size_arr = NULL;
		}
	}
	for (unsigned int i = 0; i < (nHW + 1) * Ns; ++i)
	{
		delete[] sum_table[i];
		sum_table[i] = NULL;
	}
	delete[] sum_table;
	sum_table = NULL;
}

//
// @brief Process of a weight dependent on the standard
//        deviation, used during the weighted aggregation.
//
// @param group_3D : 3D group
// @param nSx_r : number of similar patches in the 3D group
// @param kHW: size of patches
// @param chnls: number of channels in the image
// @param weight_table: will contain the weighting for each
//        channel.
//
// @return none.
//
void sd_weighting(float * const group_3D, const unsigned nSx_r, const unsigned kHW, const unsigned chnls, float * weight_table)
{
	const unsigned N = nSx_r * kHW * kHW;

	for (unsigned c = 0; c < chnls; c++)
	{
		// Initialization
		float mean = 0.0f;
		float std = 0.0f;

		// Compute the sum and the square sum
		for (unsigned k = 0; k < N; k++)
		{
			mean += group_3D[k];
			std += group_3D[k] * group_3D[k];
		}

		// Sample standard deviation (Bessel's correction)
		float res = (std - mean * mean / (float)N) / (float)(N - 1);

		// Return the weight as used in the aggregation
		weight_table[c] = (res > 0.0f ? 1.0f / sqrtf(res) : 0.0f);
	}
}






