#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <string.h>

#include "bm3d.h"
#include "utilities.h"
#include "ImgProcUtility.h"

#define YUV       0
#define YCBCR     1
#define OPP       2
#define RGB       3
#define DCT       4
#define BIOR      5
#define HADAMARD  6
#define NONE      7

using namespace std;

//
// @file   main.cpp
// @brief  Main executable file. Do not use lib_fftw to
//         process DCT.
//
// @author MARC LEBRUN  <marc.lebrun@cmla.ens-cachan.fr>
//


int main(int argc, char **argv)
{
	argv[0] = "BM3D";
	argv[1] = "test.jpg";
	argv[2] = "10";
	argv[3] = "test_denoised.bmp";
    //! Load image
	IplImage * iplImage = NULL;
	if (load_image(argv[1], iplImage) != EXIT_SUCCESS)
        return EXIT_FAILURE;

	float fSigma = (float)atof(argv[2]);

	//! Add noise
	cout << endl << "Denoise parameter [sigma = " << fSigma << "] ...\n";

	//IplImage * iplImage_basic = NULL;
	IplImage * iplImage_denoised = NULL;
	iplImage_denoised = run_bm3d(iplImage, fSigma);


	cout << endl << "Save images...\n";
	//if (save_image(argv[3], iplImage_denoised) != EXIT_SUCCESS)
	//	return EXIT_FAILURE;
    CImageUtility::saveImage(argv[3], iplImage_denoised, 0, 1, 8);

    //system("pause");
	return 0;
}
