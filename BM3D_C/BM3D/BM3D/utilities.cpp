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


//
// @file utilities.cpp
// @brief Utilities functions
//
// @author Marc Lebrun <marc.lebrun@cmla.ens-cachan.fr>
// 

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include "unistd.h"
#include <math.h>

#include "mt19937ar.h"
#include "utilities.h"
#include "bm3d.h"

#define YUV       0
#define YCBCR     1
#define OPP       2
#define RGB       3  
#define M_PI      3.14159265358979323846

using namespace std;

//
// @brief Load image, check the number of channels
//
// @param name : name of the image to read
// @param img : vector which will contain the image : R, G and B concatenated
// @param width, height, chnls : size of the image
//
// @return EXIT_SUCCESS if the image has been loaded, EXIT_FAILURE otherwise
//
int load_image(char* name, IplImage * &iplImage)
{
	// read input image
	cout << endl << "Read input image...";
	size_t h, w, c;
	//float *tmp = NULL;

	int bit_depth;
	iplImage = CImageUtility::loadImage(name, bit_depth);

	w = iplImage->width;
	h = iplImage->height;
	c = iplImage->nChannels;

	cout << "done." << endl;

	// Some image informations
	cout << "image name:" << name << endl;
	cout << "image size :" << endl;
	cout << " - width          = " << w << endl;
	cout << " - height         = " << h << endl;
	cout << " - nb of channels = " << c << endl;

	return EXIT_SUCCESS;
}

//
// @brief write image
//
// @param name : path+name+extension of the image
// @param img : vector which contains the image
// @param width, height, chnls : size of the image
//
// @return EXIT_SUCCESS if the image has been saved, EXIT_FAILURE otherwise
//
int save_image(char* name, IplImage * &iplImage)
{
	CImageUtility::saveImage(name, iplImage);
	return EXIT_SUCCESS;
}


// @brief Check if a number is a power of 2

bool power_of_2(const unsigned n)
{
	if (n == 0)
		return false;

	if (n == 1)
		return true;

	if (n % 2 == 0)
		return power_of_2(n / 2);
	else
		return false;
}

//
//  @brief Add boundaries by symetry
// 
//  @param img : image to symetrize
//  @param img_sym : will contain img with symetrized boundaries
//  @param width, height, chnls : size of img
//  @param N : size of the boundary
// 
//  @return none.
// 
void symetrize(float * &img, float * &img_sym, const unsigned width, const unsigned height, const unsigned chnls, const unsigned N)
{
	// Declaration
	const unsigned w = width + 2 * N;
	const unsigned h = height + 2 * N;

	for (unsigned c = 0; c < chnls; c++)
	{
		unsigned dc = c * width * height;
		unsigned dc_2 = c * w * h + N * w + N;

		// Center of the image
		for (unsigned i = 0; i < height; i++)
			for (unsigned j = 0; j < width; j++, dc++)
				img_sym[dc_2 + i * w + j] = img[dc];

		// Top and bottom
		dc_2 = c * w * h;
		for (unsigned j = 0; j < w; j++, dc_2++)
			for (unsigned i = 0; i < N; i++)
			{
				img_sym[dc_2 + i * w] = img_sym[dc_2 + (2 * N - i - 1) * w];
				img_sym[dc_2 + (h - i - 1) * w] = img_sym[dc_2 + (h - 2 * N + i) * w];
			}

		// Right and left
		dc_2 = c * w * h;
		for (unsigned i = 0; i < h; i++)
		{
			const unsigned di = dc_2 + i * w;
			for (unsigned j = 0; j < N; j++)
			{
				img_sym[di + j] = img_sym[di + 2 * N - j - 1];
				img_sym[di + w - j - 1] = img_sym[di + w - 2 * N + j];
			}
		}
	}

	return;
}

IplImage * symetrize(IplImage * iplImage, const unsigned N)
{
    const unsigned int width = iplImage->width;
    const unsigned int height = iplImage->height;
    const unsigned int chnls = iplImage->nChannels;
    const unsigned int w = width + 2 * N;
    const unsigned int h = height + 2 * N;

    float * img = transfer_iplImage2buffer(iplImage);
    float * img_sym = new float[w * h * chnls];
    if (!img_sym)
    {
        delete[] img_sym;
        img_sym = NULL;
    }
    for (unsigned c = 0; c < chnls; c++)
    {
        unsigned dc = c * width * height;
        unsigned dc_2 = c * w * h + N * w + N;

        // Center of the image
        for (unsigned i = 0; i < height; i++)
            for (unsigned j = 0; j < width; j++, dc++)
                img_sym[dc_2 + i * w + j] = img[dc];

        // Top and bottom
        dc_2 = c * w * h;
        for (unsigned j = 0; j < w; j++, dc_2++)
            for (unsigned i = 0; i < N; i++)
            {
                img_sym[dc_2 + i * w] = img_sym[dc_2 + (2 * N - i - 1) * w];
                img_sym[dc_2 + (h - i - 1) * w] = img_sym[dc_2 + (h - 2 * N + i) * w];
            }

        // Right and left
        dc_2 = c * w * h;
        for (unsigned i = 0; i < h; i++)
        {
            const unsigned di = dc_2 + i * w;
            for (unsigned j = 0; j < N; j++)
            {
                img_sym[di + j] = img_sym[di + 2 * N - j - 1];
                img_sym[di + w - j - 1] = img_sym[di + w - 2 * N + j];
            }
        }
    }

    IplImage *iplImage_sym = transfer_buffer2iplImage(img_sym, w, h, 3, false);
    save_image("sym.bmp", iplImage_sym);
    delete[] img_sym;
    img_sym = NULL;

    return iplImage_sym;
}

//
// @brief Transform the color space of the image
//
// @param img: image to transform
// @param color_space: choice between OPP, YUV, YCbCr, RGB
// @param width, height, chnls: size of img
// @param rgb2yuv: if true, transform the color space
//        from RGB to YUV, otherwise do the inverse
//
// @return EXIT_FAILURE if color_space has not expected
//         type, otherwise return EXIT_SUCCESS.
//
int color_space_transform(float * &img, const unsigned color_space, const unsigned width, const unsigned height, const unsigned chnls, const bool rgb2yuv)
{
	if (chnls == 1 || color_space == RGB)
		return EXIT_SUCCESS;

	// Declarations
	float * tmp = new float[chnls * width * height];
	if (!tmp)
	{
		delete[] tmp;
		tmp = NULL;
	}
	const unsigned int red = 0;
	const unsigned int green = width * height;
	const unsigned int blue = width * height * 2;

	// Transformations depending on the mode
	if (color_space == YUV)
	{
		if (rgb2yuv)
		{
			for (unsigned int k = 0; k < width * height; k++)
			{
				// Y
				tmp[k + red] = 0.299f   * img[k + red] + 0.587f   * img[k + green] + 0.114f   * img[k + blue];
				// U
				tmp[k + green] = -0.14713f * img[k + red] - 0.28886f * img[k + green] + 0.436f   * img[k + blue];
				// V
				tmp[k + blue] = 0.615f   * img[k + red] - 0.51498f * img[k + green] - 0.10001f * img[k + blue];
			}
		}
		else
		{
			for (unsigned int k = 0; k < width * height; k++)
			{
				// Red   channel
				tmp[k + red] = img[k + red] + 1.13983f * img[k + blue];
				// Green channel
				tmp[k + green] = img[k + red] - 0.39465f * img[k + green] - 0.5806f * img[k + blue];
				// Blue  channel
				tmp[k + blue] = img[k + red] + 2.03211f * img[k + green];
			}
		}
	}
	else if (color_space == YCBCR)
	{
		if (rgb2yuv)
		{
			for (unsigned int k = 0; k < width * height; k++)
			{
				// Y
				tmp[k + red] = 0.299f * img[k + red] + 0.587f * img[k + green] + 0.114f * img[k + blue];
				// U
				tmp[k + green] = -0.169f * img[k + red] - 0.331f * img[k + green] + 0.500f * img[k + blue];
				// V
				tmp[k + blue] = 0.500f * img[k + red] - 0.419f * img[k + green] - 0.081f * img[k + blue];
			}
		}
		else
		{
			for (unsigned int k = 0; k < width * height; k++)
			{
				// Red   channel
				tmp[k + red] = 1.000f * img[k + red] + 0.000f * img[k + green] + 1.402f * img[k + blue];
				// Green channel
				tmp[k + green] = 1.000f * img[k + red] - 0.344f * img[k + green] - 0.714f * img[k + blue];
				// Blue  channel
				tmp[k + blue] = 1.000f * img[k + red] + 1.772f * img[k + green] + 0.000f * img[k + blue];
			}
		}
	}
	else if (color_space == OPP)
	{
		if (rgb2yuv)
		{
			for (unsigned int k = 0; k < width * height; k++)
			{
				// Y
				tmp[k + red] = 0.333f * img[3 * k + 2] + 0.333f * img[3 * k + 1] + 0.333f * img[3 * k];
				// U
				tmp[k + green] = 0.500f * img[3 * k + 2] + 0.000f * img[3 * k + 1] - 0.500f * img[3 * k];
				// V
				tmp[k + blue] = 0.250f * img[3 * k + 2] - 0.500f * img[3 * k + 1] + 0.250f * img[3 * k];
			}
		}
		else
		{
			for (unsigned int k = 0; k < width * height; k++)
			{	
				// Red   channel
				tmp[3 * k + 2] = 1.0f * img[k + red] + 1.0f * img[k + green] + 0.666f * img[k + blue];
				// Green channel
				tmp[3 * k + 1] = 1.0f * img[k + red] + 0.0f * img[k + green] - 1.333f * img[k + blue];
				// Blue  channel
				tmp[3 * k] = 1.0f * img[k + red] - 1.0f * img[k + green] + 0.666f * img[k + blue];
			}
		}
	}
	else
	{
		cout << "Wrong type of transform. Must be OPP, YUV, or YCbCr!!" << endl;
		return EXIT_FAILURE;
	}

	for (unsigned int k = 0; k < width * height * chnls; k++)
	{
		img[k] = tmp[k];
	}
	delete[] tmp;
	tmp = NULL;

	return EXIT_SUCCESS;
}

IplImage * color_space_transform(IplImage * iplImage, const bool rgb2yuv)
{
    const unsigned int width = iplImage->width;
    const unsigned int height = iplImage->height;
    const unsigned int chnls = iplImage->nChannels;

    const unsigned int red = 0;
    const unsigned int green = width * height;
    const unsigned int blue = width * height * 2;

    float * tmp = new float[chnls * width * height];
    if (!tmp)
    {
        delete[] tmp;
        tmp = NULL;
    }
    float * img = transfer_iplImage2buffer(iplImage);
    if (rgb2yuv)
    {
        for (unsigned int k = 0; k < width * height; k++)
        {
            // Y
            tmp[k + red] = 0.333f * img[3 * k + 2] + 0.333f * img[3 * k + 1] + 0.333f * img[3 * k];
            // U
            tmp[k + green] = 0.500f * img[3 * k + 2] + 0.000f * img[3 * k + 1] - 0.500f * img[3 * k];
            // V
            tmp[k + blue] = 0.250f * img[3 * k + 2] - 0.500f * img[3 * k + 1] + 0.250f * img[3 * k];
        }
    }
    else
    {
        for (unsigned int k = 0; k < width * height; k++)
        {
            // Red   channel
            tmp[3 * k + 2] = 1.0f * img[k + red] + 1.0f * img[k + green] + 0.666f * img[k + blue];
            // Green channel
            tmp[3 * k + 1] = 1.0f * img[k + red] + 0.0f * img[k + green] - 1.333f * img[k + blue];
            // Blue  channel
            tmp[3 * k] = 1.0f * img[k + red] - 1.0f * img[k + green] + 0.666f * img[k + blue];
        }
    }

    //FILE *pFile_2 = fopen("img_ipl.txt", "w");
    //for (unsigned int i = 0; i < width * height; ++i)
    //{
    //    fprintf(pFile_2, "%f, %f, %f\n", tmp[3 * i], tmp[3 * i + 1], tmp[3 * i + 2]);
    //}
    //fclose(pFile_2);

    IplImage * iplImage_ = transfer_buffer2iplImage(tmp, width, height, chnls, false);
    delete[] tmp;
    tmp = NULL;

    return iplImage_;
}

//
// @brief Look for the closest power of 2 number
//
// @param n: number
//
// @return the closest power of 2 lower or equal to n
//
int closest_power_of_2(const unsigned n)
{
	unsigned r = 1;
	while (r * 2 <= n)
		r *= 2;

	return r;
}

//
// @brief Estimate sigma on each channel according to
//        the choice of the color_space.
//
// @param sigma: estimated standard deviation of the noise;
// @param sigma_Y : noise on the first channel;
// @param sigma_U : (if chnls > 1) noise on the second channel;
// @param sigma_V : (if chnls > 1) noise on the third channel;
// @param chnls : number of channels of the image;
// @param color_space : choice between OPP, YUV, YCbCr. If not
//        then we assume that we're still in RGB space.
//
// @return EXIT_FAILURE if color_space has not expected
//         type, otherwise return EXIT_SUCCESS.
//
int estimate_sigma(const float sigma, float * sigma_table, const unsigned chnls, const unsigned color_space)
{
	if (chnls == 1)
		sigma_table[0] = sigma;
	else
	{
		if (color_space == YUV)
		{
			// Y
			sigma_table[0] = sqrtf(0.299f * 0.299f + 0.587f * 0.587f + 0.114f * 0.114f) * sigma;
			// U
			sigma_table[1] = sqrtf(0.14713f * 0.14713f + 0.28886f * 0.28886f + 0.436f * 0.436f) * sigma;
			// V
			sigma_table[2] = sqrtf(0.615f * 0.615f + 0.51498f * 0.51498f + 0.10001f * 0.10001f) * sigma;
		}
		else if (color_space == YCBCR)
		{
			// Y
			sigma_table[0] = sqrtf(0.299f * 0.299f + 0.587f * 0.587f + 0.114f * 0.114f) * sigma;
			// U
			sigma_table[1] = sqrtf(0.169f * 0.169f + 0.331f * 0.331f + 0.500f * 0.500f) * sigma;
			// V
			sigma_table[2] = sqrtf(0.500f * 0.500f + 0.419f * 0.419f + 0.081f * 0.081f) * sigma;
		}
		else if (color_space == OPP)
		{
			// Y
			sigma_table[0] = sqrtf(0.333f * 0.333f + 0.333f * 0.333f + 0.333f * 0.333f) * sigma;
			// U
			sigma_table[1] = sqrtf(0.5f * 0.5f + 0.0f * 0.0f + 0.5f * 0.5f) * sigma;
			// V
			sigma_table[2] = sqrtf(0.25f * 0.25f + 0.5f * 0.5f + 0.25f * 0.25f) * sigma;
		}
		else if (color_space == RGB)
		{
			// Y
			sigma_table[0] = sigma;
			// U
			sigma_table[1] = sigma;
			// V
			sigma_table[2] = sigma;
		}
		else
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
//
// @brief Initialize a set of indices.
//
// @param ind_set: will contain the set of indices;
// @param max_size: indices can't go over this size;
// @param N : boundary;
// @param step: step between two indices.
//
// @return none.
//
void ind_initialize(unsigned * &ind_set, const unsigned max_size, const unsigned N, const unsigned step, unsigned &size)
{
	/// Recoding
	if ((max_size - 2 * N) % step == 0)
	{
		size = (max_size - 2 * N) / step;
	}
	else
	{
		size = (max_size - 2 * N) / step + 1;
	}

	unsigned ind = N;
	if (N > 1)
	{
		size += 1;
		ind_set = new unsigned[size];
		for (unsigned int i = 0; i < size - 1; i++)
		{
			ind_set[i] = ind;
			ind += step;
		}
		ind_set[size - 1] = max_size - N - 1;
	}
	else
	{
		ind_set = new unsigned[size];
		for (unsigned int i = 0; i < size; i++)
		{
			ind_set[i] = ind;
			ind += step;
		}
	}
}

//
// @brief For convenience. Estimate the size of the ind_set vector built
//        with the function ind_initialize().
//
// @return size of ind_set vector built in ind_initialize().
//
unsigned ind_size(const unsigned max_size, const unsigned N, const unsigned step)
{
	unsigned ind = N;
	unsigned k = 0;
	while (ind < max_size - N)
	{
		k++;
		ind += step;
	}
	if (ind - step < max_size - N - 1)
		k++;

	return k;
}

//
// @brief Initialize a 2D fftwf_plan with some parameters
//
// @param plan: fftwf_plan to allocate;
// @param N: size of the patch to apply the 2D transform;
// @param kind: forward or backward;
// @param nb: number of 2D transform which will be processed.
//
// @return none.
//
void allocate_plan_2d(fftwf_plan* plan, const unsigned N, const fftwf_r2r_kind kind, const unsigned nb)
{
	int            nb_table[2] = { N, N };
	int            nembed[2] = { N, N };
	fftwf_r2r_kind kind_table[2] = { kind, kind };

	float* vec = (float*)fftwf_malloc(N * N * nb * sizeof(float));
	(*plan) = fftwf_plan_many_r2r(2, nb_table, nb, vec, nembed, 1, N * N, vec,
		nembed, 1, N * N, kind_table, FFTW_ESTIMATE);

	fftwf_free(vec);
}

//
// @brief Initialize a 1D fftwf_plan with some parameters
//
// @param plan: fftwf_plan to allocate;
// @param N: size of the vector to apply the 1D transform;
// @param kind: forward or backward;
// @param nb: number of 1D transform which will be processed.
//
// @return none.
//
void allocate_plan_1d(fftwf_plan* plan, const unsigned N, const fftwf_r2r_kind kind, const unsigned nb)
{
	int nb_table[1] = { N };
	int nembed[1] = { N * nb };
	fftwf_r2r_kind kind_table[1] = { kind };

	float* vec = (float*)fftwf_malloc(N * nb * sizeof(float));
	(*plan) = fftwf_plan_many_r2r(1, nb_table, nb, vec, nembed, 1, N, vec,
		nembed, 1, N, kind_table, FFTW_ESTIMATE);
	fftwf_free(vec);
}

//
// @brief tabulated values of log2(N), where N = 2 ^ n.
//
// @param N : must be a power of 2 smaller than 64
//
// @return n = log2(N)
//
unsigned ind_log2(const unsigned N)
{
	return (N == 1 ? 0 :
		(N == 2 ? 1 :
		(N == 4 ? 2 :
		(N == 8 ? 3 :
		(N == 16 ? 4 :
		(N == 32 ? 5 : 6))))));
}

//
// @brief tabulated values of log2(N), where N = 2 ^ n.
//
// @param N : must be a power of 2 smaller than 64
//
// @return n = 2 ^ N
//
unsigned ind_pow2(const unsigned N)
{
	return (N == 0 ? 1 :
		(N == 1 ? 2 :
		(N == 2 ? 4 :
		(N == 3 ? 8 :
		(N == 4 ? 16 :
		(N == 5 ? 32 : 64))))));
}
