// Example-based Super-resolution
// Luhong Liang, IC-ASD, ASTRI
// Setp. 7, 2011

#pragma once
#pragma warning(disable : 4996)

//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#include "HastTypeDef.h"
#ifdef VEUHDFILTER2D_EXPORTS
#include "VEUHDFilter2D.h"
#endif

// constant
#define IMAGE_DYNAMIC_RANGE_32F		255.0f
#define IMAGE_DYNAMIC_RANGE_8U		255

#ifndef ABS
#define ABS(a) ((a)<0?-(a):(a))
#endif

class CImageUtility {

public:

	//
    // Image allocation and release
    //

	//static bool safeAllocImage(int count, ...);
	static void safeReleaseImage(IplImage **iplImage1);
	static void safeReleaseImage(IplImage **iplImage1, IplImage **iplImage2);
	static void safeReleaseImage(IplImage **iplImage1, IplImage **iplImage2, IplImage **iplImage3);
	static void safeReleaseImage(IplImage **iplImage1, IplImage **iplImage2, IplImage **iplImage3, IplImage **iplImage4);
	static void safeReleaseImage(IplImage **iplImage1, IplImage **iplImage2, IplImage **iplImage3, IplImage **iplImage4, IplImage **iplImage5);
	static void safeReleaseImage(IplImage **iplImage1, IplImage **iplImage2, IplImage **iplImage3, IplImage **iplImage4, IplImage **iplImage5, IplImage **iplImage6);

    //
    // File I/O and management
    //

    // General image loading function (see below)

	// Save image of variance data types
	static bool  saveImage(char stFilename[], IplImage *iplImage, short bias = 0, float mag_factor = 1.0f, int bit_depth = 8);
#ifdef __OPENCV_OLD_CV_H__
	static bool  saveImage(char stFilename[], IplImage *iplImage, CvRect rect, short bias = 0);
#endif  // #ifdef __OPENCV_OLD_CV_H__

	// Save a 16-bit image as 8-bit one
	static bool saveImage16U(char stFilename[], IplImage *iplImage16Bit, short bias = 0, float mag_factor = 1.0f, int bit_depth = 8);

	// load/save image into BMP file
	static IplImage *loadFromBMP(char stFilename[]);
	static bool saveToBMP(char stFilename[], IplImage *iplImage);

	// Save a region in image
	static bool saveImagePatch(char stFilename[], IplImage *iplImage, int left, int top, int widht, int height, int scaling);
	static bool saveImagePatch_32f(char stFilename[], float *pData, int widht, int height, int scaling);

	// read and write matlab image matrix
	static IplImage *readImageMat(char stFilename[], int &bit_depth);
	static bool writeImageMat(IplImage *iplImage, char stFilename[]);
	static bool writeImageMat(float *pImage, int nWidth, int nHeight, char stFilename[]);

	// read and write text testing vector
	static IplImage *loadImageVector(char stFilename[], int width, int height, int channel = 1, int bit_depth = 8);
	static bool writeImageVector(char stFilename[], IplImage *iplImage, bool signed_int = false, int bit_depth = 10, int channel = 0);

	static bool writeImageBlocks_Y12(char stFilename[], IplImage *iplImage, bool signed_int = false, int bit_depth = 10, int channel = 0);
	static bool writeImageBlocks_X8(char stFilename[], IplImage *iplImage, bool signed_int = false, int bit_depth = 10, int channel = 0, int offset = 0 );

	// file management
	static bool addFileSuffix(char stSrcFilename[], char stSuffix[], char stDstFilename[]);
    static bool addFrameNum(char szSrcFile[], int num, char szDstFile[]);
	static bool getFileExt(char stFilename[], char stFileExt[]);
	static bool getFilePre(char stFilename[], char stFilePre[]);
	static char *getFilename(char stFilePath[], char stFilename[]);
	static char *getFilePath(char stFilename[], char stFilePath[]);
	static bool isVideoFile(char stFilename[]);
	static bool isImageFile(char stFilename[]);

    //
    // Color conversion related
    //

	// Get color planes in an color image (only support 8-bit)
	static bool  decColorImage(IplImage *iplColorImage, IplImage *iplPlane1, IplImage *iplPlane2, IplImage *iplPlane3);

	// Assemble images
	static IplImage *asmColorImage(IplImage *iplPlane1, IplImage *iplPlane2, IplImage *iplPlane3);
    static bool  asmColorImage(IplImage *iplPlane1, IplImage *iplPlane2, IplImage *iplPlane3, IplImage *iplColorImage);
	static IplImage *cvtToBGR(IplImage *iplPlane1, IplImage *iplPlane2, IplImage *iplPlane3, int code);
	static IplImage *assembleGreyImage(unsigned char *pImage, int nWidth, int nHeight);

	// color conversion
    static bool cvtYUVtoBGR(IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, IplImage *iplBGR, ColorSpaceName color_sp, int bit_depth);
    static bool cvtYUVtoBGR_32F(IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, IplImage *iplBGR, ColorSpaceName color_sp);
    static bool cvtYUVtoBGR_16U(IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, IplImage *iplBGR, ColorSpaceName color_sp, int bit_depth);
	static bool cvtYUVtoBGR_8U(IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, IplImage *iplBGR, ColorSpaceName color_sp);
    static bool cvtYUVtoBGR_8U_10U(IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, IplImage *iplBGR, ColorSpaceName color_sp);
	static bool cvtYUVtoBGR_8U_10U_Basic(IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, IplImage *iplBGR, ColorSpaceName color_sp);
    static bool cvtYUVtoRGB_8U(IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, IplImage *iplRGB, ColorSpaceName color_sp);
	
    static bool cvtBGRtoYUV(IplImage *iplBGR, IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, ColorSpaceName color_sp, int bit_depth);
    static bool cvtBGRtoYUV_32F(IplImage *iplBGR, IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, ColorSpaceName color_sp);
    static bool cvtBGRtoYUV_16U(IplImage *iplBGR, IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, ColorSpaceName color_sp, int bit_depth);
    static bool cvtBGRtoYUV_16U_8U(IplImage *iplBGR, IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, ColorSpaceName color_sp, int bit_depth);
	static bool cvtBGRtoYUV_8U(IplImage *iplBGR, IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, ColorSpaceName color_sp);
	static bool cvtBGRtoYUV_8U_Basic(IplImage *iplBGR, IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, ColorSpaceName color_sp);
    static bool cvtBGRtoYUV_10U_8U(IplImage *iplBGR, IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, ColorSpaceName color_sp);

    static bool cvtBGRtoGray_8U(IplImage *iplBGR, IplImage *iplGray, ColorSpaceName color_sp);
    static bool cvtGraytoBGR_8U(IplImage *iplGray, IplImage *iplBGR);

    static bool cvtBGRtoHSV_32f(IplImage *iplBGR, IplImage *iplH, IplImage *iplS, IplImage *iplV);
    static bool cvtHSVtoBGR_32f(IplImage *iplH, IplImage *iplS, IplImage *iplV, IplImage *iplBGR);

    // pixel format conversion
    static bool swapRGB(IplImage *iplImage);

    //
	// Convert image data types
    //

    // Integer (including 8U) to floating point
	static bool cvtImage8Uto32F(IplImage *iplImage8U, IplImage *iplImage32F);
    static bool cvtImage8Uto32F(unsigned char *pImage8U, IplImage *iplImage32F);
	static IplImage *cvtImage8Uto32F(IplImage *iplImage8U);

    static bool cvtImage16Uto32F(unsigned short *pImage16U, IplImage *iplImage32F, int bit_depth);
    static bool cvtImage16Uto32F(IplImage *iplImage16U, IplImage *iplImage32F, int bit_depth);

    static bool cvtImage32Sto32F(IplImage *iplImage32S, IplImage *iplImage32F, int bit_depth);
	static IplImage *cvtImage32Sto32F(IplImage *iplImage32S, int bit_depth);

    static bool cvtImageIntto32F(IplImage *iplImage32S, IplImage *iplImage32F, int bit_depth);
	static IplImage *cvtImageIntto32F(IplImage *iplImage32S, int bit_depth);

    // 32F to integer (inlcuding 8U)
	static bool cvtImage32Fto8U(IplImage *iplImage32F, IplImage *iplImage8U, short bias, float mag_factor, int cvt_method = SR_DEPTHCVT_ROUND);
    static bool cvtImage32Fto8U(IplImage *iplImage32F, unsigned char *pImage8U, short bias, float mag_factor, int cvt_method = SR_DEPTHCVT_ROUND);
	static IplImage *cvtImage32Fto8U(IplImage *iplImage32F, short bias = 0, float mag_factor = 1.0f, int cvt_method = SR_DEPTHCVT_ROUND);

	static bool cvtImage32Fto16U(IplImage *iplImage32F, unsigned short *pImage16U, int bit_depth, int cvt_method = SR_DEPTHCVT_ROUND);
    static bool cvtImage32Fto16U(IplImage *iplImage32F, IplImage *iplImage16U, int bit_depth, int cvt_method = SR_DEPTHCVT_ROUND);

    static bool cvtImage32Fto32U(IplImage *iplImage32F, unsigned int *pImage32U, int bit_depth, int cvt_method = SR_DEPTHCVT_ROUND);
    static bool cvtImage32Fto32U(IplImage *iplImage32F, unsigned int *pImage32U, int widthStep, int bit_depth, int cvt_method);
    static bool cvtImage32Fto32U(IplImage *iplImage32F, IplImage *ipImage32U, int bit_depth, int cvt_method = SR_DEPTHCVT_ROUND);
	static bool cvtImage32Fto31U(IplImage *iplImage32F, IplImage *iplImage32S, int bit_depth, int cvt_method = SR_DEPTHCVT_ROUND);
	static IplImage *cvtImage32Fto31U(IplImage *iplImage32F, int bit_depth, int cvt_method = SR_DEPTHCVT_ROUND);

    static bool cvtImage32FtoUint(IplImage *iplImage32F, IplImage *iplImageUint, int bit_depth, int cvt_method = SR_DEPTHCVT_ROUND);
    static IplImage *cvtImage32FtoUint(IplImage *iplImage32F, int bit_depth, int dst_data_type, int cvt_method = SR_DEPTHCVT_ROUND);

    // conversion among integer images
	static bool cvtImage8UtoInt(IplImage *iplImage8U, IplImage *iplImage32S, int bit_depth, int cvt_method=SR_DEPTHCVT_ZEROS);
	static IplImage *cvtImage8UtoInt(IplImage *iplImage8U, int bit_depth, int cvt_method=SR_DEPTHCVT_ZEROS);
	static bool cvtImageIntto8U(IplImage *iplImage32S, int bit_depth, IplImage *iplImage8U, short bias, float mag_factor, int cvt_method=SR_DEPTHCVT_ROUND);
	static IplImage *cvtImageIntto8U(IplImage *iplImage32S, int bit_depth, short bias = 0, float mag_factor = 1.0f, int cvt_method=SR_DEPTHCVT_ROUND);

    static bool cvtImage8Uto16U(IplImage *iplImage8U, IplImage *iplImage16U, int bit_depth, int cvt_method=SR_DEPTHCVT_ZEROS);
    static bool cvtImage16Uto8U(IplImage *iplImage16U, int bit_depth, IplImage *iplImage8U, int cvt_method=SR_DEPTHCVT_ROUND);
    static bool cvtImage16Uto16U(IplImage *iplSrc16U, int src_bit_depth, IplImage *iplDst16U, int dst_bit_depth, int cvt_method=(SR_DEPTHCVT_ROUND|SR_DEPTHCVT_ZEROS));

	static IplImage *cvtImage8Uto8U(IplImage *iplImage8U, short bias, float mag_factor);
	static bool cvtImage8Uto8U(IplImage *iplImage8U, IplImage *iplDst8U, short bias, float mag_factor);

    // extract MSB/LSB of integer image
    static bool cvtImage16UtoMSB8U(IplImage *iplImage16U, IplImage *iplImage8U, int left_shift = 0);
    static IplImage *cvtImage16UtoMSB8U(IplImage *iplImage16U, int left_shift = 0);

    static bool cvtImage16UtoLSB8U(IplImage *iplImage16U, IplImage *iplImage8U, bool clipping, int left_shift = 0);
    static IplImage *cvtImage16UtoLSB8U(IplImage *iplImage16U, bool clipping, int left_shift = 0);

    // error diffusion
	static IplImage *diffusionImage32Fto8U(IplImage *iplImage32F);

    //
    // Image data fetch and set
    //

	// fetch block 
	static bool fetchBlock(IplImage *iplImage, IplImage *iplBlock, int x, int y);

	// relocate image pixel to a new grid
	static bool relocateToGrid2_32f(IplImage *iplSrcImage, IplImage *iplDstImage);		// relocate pixels to a 2x lager grid, fill zero values to "holes"

    //
    // Image processing functions
    // 

	// build image pyramid
	static bool meanPyrDown(IplImage *iplSrcImage, IplImage *iplDstImage);			// for all data types
	static bool meanPyrDownHalfPel(IplImage *iplSrcImage, IplImage *iplDstImage);

	static bool meanPyrDown_8u(IplImage *iplSrcImage, IplImage *iplDstImage);		// for 8-bit unsigned
	static bool meanPyrDownHalfPel_8u(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool meanPyrDown_32f(IplImage *iplSrcImage, IplImage *iplDstImage);		// for single pecision floating point
	static bool meanPyrDown_32f(IplImage *iplSrcImage, IplImage *iplDstImage, int ds_level);
	static bool meanPyrDownHalfPel_32f(IplImage *iplSrcImage, IplImage *iplDstImage);

	static bool intpPyrDownHalfPel_32f(IplImage *iplSrcImage, IplImage *iplDstImage);

	static bool intpBicubic_32f(IplImage *iplSrcImage, IplImage *iplDstImage, bool clipping = false);
	static bool upscaleDyadicBicuLift_32f(IplImage *iplSrcImage, IplImage *iplDstImage, bool clipping = false);

	static bool pyrDownSquare9_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool pyrDownEven_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool pyrDownEven_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool pyrDownDiamod5_32f(IplImage *iplSrcImage, IplImage *iplDstImage);

	// interlace and de-interlace
	static bool interlaceY_8U(IplImage *iplSrcImage, IplImage *iplDstImage, bool even);
	static bool interlaceX(IplImage *iplSrcImage, IplImage *iplDstImage, bool even);
    static bool interlaceX_8U(IplImage *iplSrcImage, IplImage *iplDstImage, bool even);
    static bool interlaceX_16U(IplImage *iplSrcImage, IplImage *iplDstImage, bool even);
    static bool interlaceX_Int(IplImage *iplSrcImage, IplImage *iplDstImage, bool even);
    static bool interlaceX_32f(IplImage *iplSrcImage, IplImage *iplDstImage, bool even);
    static bool cvt422to444_linear(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool cvt422to444_linear_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool cvt422to444_linear_16U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool cvt422to444_linear_Int(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool cvt422to444_linear_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool cvt422to444_cubic_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool cvt422to444_lincub_8U(IplImage *iplSrcImage, IplImage *iplDstImage);

	// scaling
	static bool scaling(IplImage *iplSrcY, IplImage *iplSrcU, IplImage *iplSrcV, IplImage *iplDstY, IplImage *iplDstU, IplImage *iplDstV);

	// image shifting and interpolation
	static bool shift_bilinear_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float x, float y, bool clipping = false);
	static bool shift_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float x, float y, bool clipping = false) {
		return shift_bilinear_32f(iplSrcImage, iplDstImage, x, y, clipping);
	}
	static bool shift_bilinear_32f(IplImage *iplImage, float x, float y);

	static bool shiftHalfPel_bicubic_32f(IplImage *iplImage, int horizontal, int vertical, bool clip);
	static bool shiftHalfPel_bicubic_int(IplImage *iplImage, int horizontal, int vertical, int clip_low, int clip_high);
	static bool shiftHalfPel_bicubic_int_s1(IplImage *iplImage, int clip_low, int clip_high);
	static bool shiftHalfPel_bicubic_8U_10bit_s5(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);

	// image resize (fixed magnification factor)
	static bool resize_4to3_bilinear_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool resize_4to3_bilinear_odd_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_4to3_bilinear_int(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
	static bool resize_4to3_bilinear_int_s1(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
    static bool resize_3to4_bilinear_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_3to4_bicubic_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_3to4_bicubic_int(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
	static bool resize_3to4_bicubic_int_s1(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);

    static bool resize_2to3_bilinear_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_2to3_bicubic_32f(IplImage *iplSrcImage, IplImage *iplDstImage);

	static bool resize_3to2_bilinear_32f(IplImage *iplSrcImage, IplImage *iplDstImage);

	static bool resize_16to9_bilinear_int(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
	static bool resize_16to9_bilinear_int_s1(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
	static bool resize_16to9_bilinear_8U_s1(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_9to16_nn_int_s1(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_9to16_bicubic_int(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
	static bool resize_9to16_bicubic_int_s1(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
	static bool resize_9to16_bicubic_8U_10bit_s1(IplImage *iplSrcImage, IplImage *iplDstImage);

	static bool resize_15to8_bilinear_int(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
	static bool resize_15to8_bilinear_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_8to15_bicubic_int(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
	static bool resize_8to15_bicubic_8U_10bit(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_8to15_bicubic_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_8to15_cublin_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_8to15_cublin_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float clip_low, float clip_high);
	static bool resize_8to15_bilinear_8U(IplImage *iplSrcImage, IplImage *iplDstImage);

	static bool resize_2to1_bilinear_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_2to1_bilinear_int(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_2to1_bilinear_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool resize_2to1_bilinear_16U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_2to1_bicubic_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_2to1_lanczos2_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_2to1_lanczos3_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_2to1_enc4_8U(IplImage *iplSrcImage, IplImage *iplDstImage, unsigned char opt_code);

	static bool inter_1to2_bicubic_8U_10bit(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
	static bool inter_1to2_bicubic_8U_10bit_s5(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
	static bool inter_1to2_bicubic_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool inter_1to2_bicubic_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool inter_1to2_bicubic_odd_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool inter_1to2_bicubic_odd_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool inter_1to2_cublin_odd_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool inter_1to2_bicubic_odd_rgb_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool inter_1to2_lanczos3_odd_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool inter_1to2_cublin_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool inter_1to2_bilinear_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool inter_1to2_bilinear_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool inter_1to2_bilinear_odd_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool inter_1to2_bilinear_odd_Int(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool inter_1to2_bilinear_odd_8U(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool inter_1to4_bilinear_odd_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool inter_1to4_H264_odd_32f(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool inter_1to2_hybrid_odd_32f(IplImage *iplSrcImage, IplImage *iplDstImage, int degree);

    static bool inter_2to1_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool inter_2to1_odd_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool inter_2to1_odd_Int(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool inter_2to1_odd_8U(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool inter_4to1_odd_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    
	static bool resize_1to2_bicubic_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_1to2_bicubic_int(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
	static bool resize_1to2_bicubic_int_s5(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
	static bool resize_1to2_bicubic_8U_int(IplImage *iplSrcImage, IplImage *iplDstImage, int bit_depth, int clip_low, int clip_high);
	static bool resize_1to2_bicubic_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_1to2_bicubic_8U_s1(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_1to2_bicubic_32f(IplImage *iplSrcImage, IplImage *iplDstImage, bool clip);
	static bool resize_1to2_bilinear_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_1to2_bilinear_int(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
    static bool resize_1to2_bilinear_16U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_1to2_bilinear_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool resize_1to2_cublin_8U(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool resize_nn_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool resize_nn_16U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool resize_nn_8U(IplImage *iplSrcImage, IplImage *iplDstImage);

	static bool intp_chroma_4to15_bilinear_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool intp_chroma_4to15_bicubic_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool intp_chroma_4to15_cublin_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool intp_chroma_1to4_bilinear_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool intp_chroma_1to4_bicubic_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool intp_chroma_1to4_cublin_8U(IplImage *iplSrcImage, IplImage *iplDstImage);

	static bool resize_16to15_bicubic_int(IplImage *iplSrcImage, IplImage *iplDstImage, int clip_low, int clip_high);
	static bool resize_16to15_bicubic_int_8U(IplImage *iplSrcImage, IplImage *iplDstImage, int bit_depth);
	static bool resize_16to15_bicubic_8U_10U(IplImage *iplSrcImage, IplImage *iplDstImage);

	static bool resize_16to15_lanczos3_8U_10U(IplImage *iplSrcImage, IplImage *iplDstImage);

	static bool resize_8to9_bicubic_8U_10U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_8to9_bicubic_8U(IplImage *iplSrcImage, IplImage *iplDstImage);

	static bool resize_4to9_bicubic_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool resize_4to9_cublin_8U(IplImage *iplSrcImage, IplImage *iplDstImage);

    //lanczos
    static bool resizeLanczos_32f(IplImage *iplSrc, IplImage *iplDst, int a);
    static bool resizeLanczosArb_32f(IplImage *iplSrc, IplImage *iplDst, int a);
    static bool resizeLanczos2_4to3_32f(IplImage *iplSrc, IplImage *iplDst);
    static bool resizeLanczos2_2to3_32f(IplImage *iplSrc, IplImage *iplDst);
    static bool resizeLanczos2_1to3_32f(IplImage *iplSrc, IplImage *iplDst);
    static bool resizeLanczos2_1to2_32f(IplImage *iplSrc, IplImage *iplDst);
    static bool resizeLanczos3_4to3_32f(IplImage *iplSrc, IplImage *iplDst);
    static bool resizeLanczos3_2to3_32f(IplImage *iplSrc, IplImage *iplDst);
    static bool resizeLanczos3_1to3_32f(IplImage *iplSrc, IplImage *iplDst);
    static bool resizeLanczos3_1to2_32f(IplImage *iplSrc, IplImage *iplDst);

	// bilateral interpolation
	static bool inter1to2BLBicubic_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float fSigmaRange = 25);
	static bool inter1to2BLBicubic_32f(IplImage *iplSrcImage, IplImage *iplRangeImage, IplImage *iplDstImage, float fSigmaRange);

    // bilateral resize
    static bool resize_1to2_BLbicu_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float fSigmaRange = 25);

	// interpolation for pyramid
	static bool bilinearPyrUp_32f(IplImage *iplSrcImage, IplImage *iplDstImage);		// for single pecision floating point

    // stretch
    static bool stretchX_8to15_cubic_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool stretchY_8to15_linear_8U(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool stretchX_1to2_cubic_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool stretchY_1to2_linear_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool stretchY_1to2_linear_32f(IplImage *iplSrcImage, IplImage *iplDstImage);

	// padding & unpadding
	static IplImage *padding(IplImage *iplImage, int left, int right, int top, int bottom, int type=0);
    static bool padding(IplImage *iplImage, IplImage *iplPadded, int left, int right, int top, int bottom, int type=0);

	static IplImage *padding_R_0(IplImage *iplImage, int left, int right, int top, int bottom);        // left right: padding; top bottom: zero
	static IplImage *padding_R_128(IplImage *iplImage, int left, int right, int top, int bottom);        // left right: padding; top bottom: 128
	static bool padding_R_128(IplImage *iplImage, IplImage *iplPadded, int left, int right, int top, int bottom);
	static IplImage *padding_0_0(IplImage *iplImage, int left, int right, int top, int bottom);
	static bool padding_0_0(IplImage *iplImage, IplImage *iplPadded, int left, int right, int top, int bottom);    // padding zeros
	static IplImage *padding_128_128(IplImage *iplImage, int left, int right, int top, int bottom);
	static bool padding_128_128(IplImage *iplImage, IplImage *iplPadded, int left, int right, int top, int bottom);    // padding 128

	static IplImage *unpadding(IplImage *iplImage, int left, int right, int top, int bottom);
	static bool unpadding(IplImage *iplImage, IplImage *iplDstImage, int left, int top);
	static bool unpadding_8U_10U(IplImage *iplImage, IplImage *iplDstImage, int left, int top);

	// pixel operations
	static bool subImage_32f(IplImage *iplSrc1, IplImage *iplSrc2, IplImage *iplDst);
	static bool addImage_32f(IplImage *iplSrc1, IplImage *iplSrc2, IplImage *iplDst);
    static bool addImage_32f(IplImage *iplSrc1, IplImage *iplDst, float bright_shift);
    static bool addImagePos_32f(IplImage *iplSrc1, IplImage *iplDst, float bright_shift);
    static bool absImage_32f(IplImage *iplSrc, IplImage *iplDst);
	static bool mulImage_32f(IplImage *iplSrc, float fFactor);
    static bool mulImage_8U(IplImage *iplSrc, float fFactor);
	static bool mulImage_32f(IplImage *iplSrc1, IplImage *iplSrc2, IplImage *iplDst);
	static float maxInImage_32f(IplImage *iplSrc);
    static int maxInImage_32s(IplImage *iplSrc);
	static float meanImage(IplImage *iplSrc);
	static float minInImage_32f(IplImage *iplSrc);
    static int minInImage_32s(IplImage *iplSrc);
    static bool getMaxMinVal(IplImage *iplImage, unsigned char &maxval, unsigned char &minval);
    static bool getMaxMinVal(IplImage *iplImage, unsigned short &maxval, unsigned short &minval);
    static bool getMaxMinVal(IplImage *iplImage, int &maxval, int &minval);
    static bool getMaxMinVal(IplImage *iplImage, float &maxval, float &minval);
	static bool cvtToLogDomain_32f(IplImage *iplImage);
	static bool cvtFromLogDomain_32f(IplImage *iplImage);
	static bool clip_32f(IplImage *iplImage, float min_v=0.0f, float max_v=255.0f);
    static bool binarize_32f(IplImage *iplImage, float threshold, float val0=0.0f, float val1=255.0f);
	static bool setValue(IplImage *iplImage, unsigned char val);
    static bool setValue(IplImage *iplImage, int val);
    static bool setValue(IplImage *iplImage, float val);

    // general 2D filters and some filter banks
    static bool filter2D_32f(IplImage *iplImage, IplImage *iplDstImage, float pFilter[], int wnd_width, int wnd_height, float factor);
    static bool kernelEpsGaussian(int wnd_rad, float sigma, float lamda, float gx, float gy, float *pFilter);
    static bool kernelDirStep(int wnd_rad, float alpha, float gx, float gy, float *pFilter);

	// smooth filters
	static bool gaussian_32f(IplImage *iplImage, IplImage *iplDstImage, int wnd_size, float sigma);
    static bool gaussian1_32f(IplImage *iplImage, IplImage *iplDstImage, int wnd_size, float sigma);
    static bool gaussian3x3_32f(IplImage *iplImage, IplImage *iplDstImage, float sigma);
    static bool gaussian3x3_o1_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float sigma);
    static bool gaussian5x5_32f(IplImage *iplImage, IplImage *iplDstImage, float sigma);
    static bool gaussian5x5_o1_32f(IplImage *iplImage, IplImage *iplDstImage, float sigma);
    static bool gaussian7x7_32f(IplImage *iplImage, IplImage *iplDstImage, float sigma);
    static bool gaussian7x7_o1_32f(IplImage *iplImage, IplImage *iplDstImage, float sigma);
    static bool gaussianRGB5x5_32f(IplImage *iplImage, IplImage *iplDstImage, float sigma);
	static bool gaussian9x9_32f(IplImage *iplImage, IplImage *iplDstImage, float sigma);
	static bool gaussian_int(IplImage *iplImage, IplImage *iplDstImage, int wnd_size, float sigma, int filter_bit_depth);
	static bool gaussian7x7_10_int(IplImage *iplImage, IplImage *iplDstImage, int filter_bit_depth);
	static bool gaussian7x7_10n15_int_s1(IplImage *iplImage, IplImage *iplDstImage, bool is10);
	static bool gaussian7x7_15_int_s4(IplImage *iplImage, IplImage *iplDstImage);
	static bool gaussian5x5_10_int_s1(IplImage *iplImage, IplImage *iplDstImage);
	static bool gaussian5x5_10_Int_s4(IplImage *iplImage, IplImage *iplDstImage);
    static bool gaussian5x5_10_8U_10U_s4(IplImage *iplImage, IplImage *iplDstImage);
	static bool gaussian5x5_10_int_s5(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool gaussian5x5_15_int_s1(IplImage *iplImage, IplImage *iplDstImage);
	static bool gaussian5x5_15_int_s5(IplImage *iplImage, IplImage *iplDstImage);
	static bool gaussian5x5_15_8U_10U_s5(IplImage *iplImage, IplImage *iplDstImage);
    static bool gaussian5x5_30_8U_10U(IplImage *iplImage, IplImage *iplDstImage);
	static bool gaussian5x5_10n15_int_s1(IplImage *iplImage, IplImage *iplDstImage, bool is10);
	static bool gaussian5x5_08_int_s4(IplImage *iplImage, IplImage *iplDstImage);
	static bool gaussian5x5_08_int_s5(IplImage *iplImage, IplImage *iplDstImage);
	static bool gaussian5x5_08_8U_10U_s5(IplImage *iplImage, IplImage *iplDstImage);
	static bool gaussian3x3_05_int(IplImage *iplImage, IplImage *iplDstImage, int filter_bit_depth);
	static bool gaussian3x3_05_int_s1(IplImage *iplImage, IplImage *iplDstImage);
	static bool gaussian3x3_05_int_s4(IplImage *iplImage, IplImage *iplDstImage);
	static bool gaussian5x5_05_int_s5(IplImage *iplImage, IplImage *iplDstImage);
	static bool gaussian5x5_05_8U_10U_s5(IplImage *iplImage, IplImage *iplDstImage);
	static bool gaussian3x3_08_int_s1(IplImage *iplImage, IplImage *iplDstImage);

	static bool adaptGaussian_32f(IplImage *iplImage, IplImage *iplDstImage, IplImage *iplPrior, int wnd_size, float pSigmaList16[], int filter_num);
    static bool adaptGaussian7x7_32f(IplImage *iplImage, IplImage *iplDstImage, IplImage *iplPrior, float pSigmaList16[], int filter_num);
	static bool adaptGaussian7x7_Int_s4(IplImage *iplImage, IplImage *iplDstImage, IplImage *iplTexStruct, int filter_mode);
	static bool adaptGaussian7x7_Int_s5(IplImage *iplImage, IplImage *iplDstImage, IplImage *iplTexStruct, int filter_mode);
    static bool adaptGaussian7x7_Int_s5a(IplImage *iplImage, IplImage *iplDstImage, IplImage *iplTexStruct, int filter_mode);
	static bool adaptGaussian3x3_05_Int_s4(IplImage *iplImage, IplImage *iplDstImage, IplImage *iplMask);
	static bool adaptBox3x3_05_Int_s4(IplImage *iplImage, IplImage *iplDstImage, IplImage *iplMask);
    static bool adaptBox3x3_05_Int_s4a(IplImage *iplImage, IplImage *iplDstImage, IplImage *iplMask);

    static bool boxing3x3_32f(IplImage *iplImage, IplImage *iplDstImage, float factor = 1.0f);
	static bool boxing5x5_32f(IplImage *iplImage, IplImage *iplDstImage, float factor = 1.0f);
    static bool boxing5x5_o1_32f(IplImage *iplImage, IplImage *iplDstImage, float factor = 1.0f);
    static bool boxing7x7_32f(IplImage *iplImage, IplImage *iplDstImage, float factor = 1.0f);
    static bool boxing9x9_32f(IplImage *iplImage, IplImage *iplDstImage, float factor = 1.0f);

    static bool neighbormean5x5_32f(IplImage *iplImage, IplImage *iplDstImage);

    static bool filtLoG_32f(IplImage *iplImage, IplImage *iplDstImage, int wnd_size, float sigma, int lap_option);

	// bilateral filters
	static bool bilatFilter_32f(IplImage *iplSrcImage, IplImage *iplDstImage, int nR, float fSigmaSpatial, float fSigmaRange);
    static bool bilatFilter5x5_1_25_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool bilatFilter5x5_box_25_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool bilatFilter5x5_1_25_int(IplImage *iplSrcImage, int bit_depth, IplImage *iplDstImage, int sp_depth=10, int ra_depth=10);
	static bool bilatFilter5x5_1_25_8U_10bit(IplImage *iplSrc8U, IplImage *iplDstInt);
	static bool bilatFilter5x5_1_25_8U_10bit_s1(IplImage *iplSrc8U, IplImage *iplDstInt);
	static bool bilatFilter5x5_1_25_8U_10bit_s5(IplImage *iplSrc8U, IplImage *iplDstInt);
	static bool bilatFilter5x5_D5_12D5_8U_10bit(IplImage *iplSrc8U, IplImage *iplDstInt);
	static bool bilatFilter5x5_D7_17D5_8U_10bit(IplImage *iplSrc8U, IplImage *iplDstInt);

	static bool bilatFilterW_32f(IplImage *iplSrcImage, IplImage *iplFilterRegion, IplImage *iplDstImage, int nR, float fSigmaSpatial, float fSigmaRange);
	static bool bilatFilter_32f(IplImage *iplSrcImage, IplImage *iplInitUpscale, IplImage *iplDstImage,  int nR, float fSigmaSpatial, float fSigmaRange);
	static bool bilatFilter_32f(IplImage *iplSrcImage, IplImage *iplInitUpscale, IplImage *iplEdge, IplImage *iplDstImage,  int nR, float fSigmaSpatial, float fSigmaRange);
	static IplImage *bilatFilter_32f(IplImage *iplImage, int nR, float fSigmaSpatial, float fSigmaRange);
	//static bool bilatFilterAdapt_32f(IplImage *iplSrcImage, IplImage *iplDstImage, int nR, float fSigmaSpatial);
	//static bool bilatFilterEnc_32f(IplImage *iplSrcImage, IplImage *iplDstImage, int nR, float fSigmaSpatial, float fSigmaRange);

	// fast bilateral filters
	static bool bilatFilterXY_32f(IplImage *iplInitImage, IplImage *iplRangeImage, IplImage *iplDstImage, float fSigmaRange);
	static bool bilatFilterXY_32f(IplImage *iplInitImage, IplImage *iplRangeImage, IplImage *iplEdgeImage, IplImage *iplDstImage, float fSigmaRange);

    // non-local mean filter and adaptive NLMF
    static bool nlmFilter11x5_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float sigma);
    static bool nlmFilter15x7_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float sigma);

    static bool nlmFilterSP11x5_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float sigma_ra, float sigma_sp);

    static bool adaNLMFilter11x5_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float sensitivity, float hvsimpact, float adaptfactor, float sigma);

	// edge detection
	static bool sobel_32f(IplImage *iplSrcImage, IplImage *iplDstImage, int direction);
    static bool sobel_o1_32f(IplImage *iplSrcImage, IplImage *iplDstImage, int direction);
	static bool sobel_int(IplImage *iplSrcImage, IplImage *iplDstImage, int direction, int clip_low, int clip_high);
	static bool sobel_8U_int(IplImage *iplSrcImage, IplImage *iplDstImage, int direction);
    static bool sobel_8U(IplImage *iplSrcImage, IplImage *iplDstImage, int direction);
    static bool sobel_o1_8U(IplImage *iplSrcImage, IplImage *iplDstImage, int direction);
	static bool sobel_8U_int_s5(IplImage *iplSrcImage, IplImage *iplDstImage, int direction);
	static bool laplace_32f(IplImage *iplSrcImage, IplImage *iplDstImage, int option = 1);
	static bool sobel_laplace_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool sobel_laplace_o1_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool sobel_laplace_abs_2_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool sobel_laplace_int(IplImage *iplSrcImage, int src_depth, IplImage *iplDstImage, int dst_depth);
	static bool sobel_laplace_int_s5(IplImage *iplSrcImage, int src_depth, IplImage *iplDstImage, int dst_depth);
	static bool sobel_laplace_8U(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool sobel_max3x3_32f(IplImage *iplSrcImage, IplImage *iplDstImage, int direction=3, float factor=1.0f);
    static bool sobel_max3x3_abs_2_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool sobel_max5x5_32f(IplImage *iplSrcImage, IplImage *iplDstImage, int direction=3, float factor=1.0f);

    static bool sobel_laplace_max3x3_abs_2_32f(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool sobel5x5_32f(IplImage *iplImage, IplImage *iplDstImage, int direction);

    static bool sobel5x5ds_32f(IplImage *iplImage, IplImage *iplDstImage);
	
	static bool prewitt3x3max_int(IplImage *iplSrcImage, int src_depth, IplImage *iplDstImage, int dst_depth);
	static bool prewitt3x3max_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
	static bool prewitt3x3max_8U_s5(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool prewitt3x3max_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
	
	static bool prewitt5x5_int(IplImage *iplSrcImage, int src_depth, IplImage *iplDstImage, int dst_depth);
    static bool prewitt5x5_32f(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool filtLoG_RGB5x5_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float sigma);

    // sharpening
    static bool usm_linear_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float lamda);
    static bool usm_cubic_32f(IplImage *iplSrcImage, IplImage *iplDstImage, int type, float lamda);
    static bool usm_rational_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float lamda, float g0 = 400.0f);

    static bool sharpen_lap8_8U(IplImage *iplSrcImage, IplImage *iplDstImage, int strength);
    static bool sharpen_lap8a_8U(IplImage *iplSrcImage, IplImage *iplDstImage, int strength, int th0, int th1);
    static bool sharpen_lap8a_8U(IplImage *iplSrcImage, IplImage *iplDstImage, int strength, int thr0, int thr1, int thr2, int thr3);
    static bool sharpen_lap8b_8U(IplImage *iplSrcImage, IplImage *iplDstImage, int strength, int thr0, int thr2);

	// error diffusion
	static IplImage *errDiffuseFS(IplImage *iplImage, float dim, int threshold=128);
	static bool errDiffuseFS(IplImage *iplImage, IplImage *iplBin, float dim, int threshold=128);

    // morphology operations
    static bool rmIsoBulb9x9_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float thw, int con_num);
    static bool rmIsoBulb11x11_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float thw, int con_num);
    static bool rmIsoBulb13x13_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float thw, int con_num);
    static bool rmIsoBulb13x13_o1_32f(IplImage *iplSrcImage, IplImage *iplDstImage, IplImage *iplBulb, float thw, int con_num);

    static bool rmIsoBulbFillHole3x3_8U(IplImage *iplSrcImage, IplImage *iplDstImage, int min_th=16);

    // 2D filter
    static bool filter2Doddxodd_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float *pFilt, int size); //Added by Cao Shuang
    static bool filter2Doddxodd_o1_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float *pFilt, int size);

    static bool filter2Devenxeven_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float *pFilt, int size); //Added by Cao Shuang
    static bool filter2Devenxeven_o1_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float *pFilt, int size);

    static bool filter2D3x3_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float *pFilt);  //Added by cao shuang
    static bool filter2D3x3_o1_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float *pFilt);

    static bool filter2D5x5_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float *pFilt);
    static bool filter2D5x5_o1_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float *pFilt);
    static bool filter2D5x5_acc1_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float *pFilt);

    static bool filter2D7x7_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float *pFilt);  //Added by cao shuang
    static bool filter2D7x7_o1_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float *pFilt);

    static bool filter2D9x9_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float *pFilt);
    static bool filter2D9x9_o1_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float *pFilt);
    static bool filter2D9x9_ReLU_32f(IplImage *iplSrcImage, IplImage *iplDstImage, float *pFilt, float bias);

protected:
    static bool inline getMonoEdgePointList(IplImage *iplLabel, CvRect roi, int label, CvPoint pt_start, unsigned char pMark[], 
                                            CvPoint pPoints[], int &point_num, int &start_index);

public:
    static IplImage *markEdges_32f(IplImage *iplEdge, float edge_th, int &edge_num, int connectivity = 8, int min_pix_num = 0);
    static IplImage *visMarkedEdges(IplImage *iplLabel, int edge_num);
    static IplImage *extrEdge_32f(IplImage *iplLabel, int edge_id0, int edge_id1, int &pixel_num);
    static bool inflatLabel3x3_32f(IplImage *iplLabel, IplImage *iplInfLabel);
    static bool rmGrateLines_32f(IplImage *ipoLabel, int wnd_rad, int threshold = 2);
    static bool getEdgeOrient(IplImage *iplLabel, IplImage *iplDirMap, IplImage *iplLikelihood, int wnd_rad);
    static bool smoothEdgeOrient(IplImage *iplDirMap, IplImage *iplDstMap, IplImage *iplLabel, int wnd_rad, float sigma);

    static bool dilateMax15x15_32f(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool mean3x3_32f(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool min3x3_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool min3x3_o1_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool min3x3_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool min3x3_int(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool min5x5_32f(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool max3x3_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool max3x3_o1_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool max3x3_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool max3x3_o1_8U(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool max3x3_int(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool max5x5_32f(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool maxbox3x3_8U(IplImage *iplSrcImage, IplImage *iplDstImage, int max_weight_quarters);
    static bool maxbox3x3_int(IplImage *iplSrcImage, IplImage *iplDstImage, int max_weight_quarters);

    static bool contrast3x3(IplImage *iplSrcImage, IplImage *DstImage);
    static bool contrast3x3_o1_8U(IplImage *iplSrcImage, IplImage *DstImage);
    static bool contrast3x3_o1_32f(IplImage *iplSrcImage, IplImage *DstImage);

    static bool contrast5x5_32f(IplImage *iplSrcImage, IplImage *DstImage);
    static bool contrast5x5_o1_32f(IplImage *iplPadded, IplImage *DstImage);

    // specially designed filters
    static IplImage *detIsoBulb5x5_32f(IplImage *iplSrcImage, bool normalize);
    static IplImage *detIsoBulb5x5_o1_32f(IplImage *iplSrcImage, IplImage *iplLocVar);
    static bool detIsoBulb5x5_32f(IplImage *iplSrcImage, IplImage *iplDstImage, bool normalize);
    static bool detIsoBulb5x5_o1_32f(IplImage *iplSrcImage, IplImage *iplDstImage, IplImage *iplLocVar);

	// image statistics (whole image)
	static bool calcMeanDev(IplImage *iplImage, float &mean, float &dev, int bit_depth = 12);
	static float rms(IplImage *iplImage1, IplImage *iplImage2);

	// image feature extraction
	static IplImage *extrStructMap_32f(IplImage *iplImage, int wnd_size = 5, int ds_level = 0);
	static IplImage *extrStructMap5x5_32f(IplImage *iplImage, int ds_level = 0);
    static IplImage *extrStructMap5x5_o1_32f(IplImage *iplImage, int ds_level = 0);
	static IplImage *extrStructMap_MS_32f(IplImage *iplImage, int wnd_size = 5);
	static bool extrStructMap_32f(IplImage *iplImage, IplImage *iplStruct, int wnd_size = 5, int ds_level = 0);
	static bool extrStructMap_32f(IplImage *iplImage, IplImage *iplRegularity, IplImage *iplCorner, int wnd_size = 5, int ds_level = 0);
	static bool extrStructMap5x5_32f(IplImage *iplImage, IplImage *iplStruct, int ds_level = 0);
    static bool extrStructMap5x5_o1_32f(IplImage *iplImage, IplImage *iplStruct, int ds_level = 0);
    static bool extrStructMap5x5_o2_32f(IplImage *iplImage, IplImage *iplStruct);
    static bool extrStructMap5x5_o2_32f(IplImage *iplImage, IplImage *iplRegular, IplImage *iplCorner);
    static bool extrStructMap5x5_o3_32f(IplImage *iplImage, IplImage *iplStruct, IplImage *iplCorner = NULL);
    static bool extrStructMap5x5_o3a_32f(IplImage *iplImage, IplImage *iplStruct, IplImage *iplGrad, IplImage *iplCorner, bool downsample);
	static bool extrStructMap_int(IplImage *iplImage, IplImage *iplStruct, int wnd_size = 5, int bit_depth = 12);
	static bool extrStructMap5x5_8U(IplImage *iplImage, IplImage *iplStruct);
	static bool extrStructMap5x5_8U_s5(IplImage *iplImage, IplImage *iplStruct);
    static bool extrStructMap5x5_8U_v2(IplImage *iplImage, IplImage *iplStruct, IplImage *iplGM, int atten_opt);        // for VEUHD

    static bool extrCornerMap_32f(IplImage *iplImage, IplImage *iplCorner, int method=1);

	static bool extrMADLowMap_32f(IplImage *iplImage, IplImage *iplMAD, int wnd_size = 5);

	static bool extrSumMap5x5_32f(IplImage *iplImage);
	static bool extrMeanMap5x5_32f(IplImage *iplImage);

    static IplImage *extrLocVarMap5x5_32f(IplImage *iplImage);
    static bool extrLocVarMap5x5_32f(IplImage *iplImage, IplImage *iplLocVar);
    static bool extrLocVarMap5x5min_32f(IplImage *iplImage, IplImage *iplLocVar);

    static IplImage *extrLocVarMap3x3_32f(IplImage *iplImage);
    static bool extrLocVarMap3x3_32f(IplImage *iplImage, IplImage *iplLocVar);

    static bool extrLocVar3x3_32f(IplImage *iplImage, IplImage *iplLocVar);

	static IplImage *extrMADMap_32f(IplImage *iplImage, int wnd_size);
	static IplImage *extrMADMap5x5_32f(IplImage *iplImage);
    static IplImage *extrMADMap5x5_o1_32f(IplImage *iplImage);
    static bool extrMADMap5x5_o1_32f(IplImage *iplImage, IplImage *iplMADMap);
    static bool extrMADMap5x5_o1_32f(IplImage *iplImage, IplImage *iplMADMap, IplImage *iplMean);

    static bool extrSingularBinMap5x5_32f(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool extrSingularBinMap5x5_32f(IplImage *iplSrcImage, IplImage *iplSin, IplImage *iplBin, IplImage *iplMaxMin, IplImage *iplMAD);
    static bool extrSingularBinMap5x5_8U_v2(IplImage *iplSrcImage, IplImage *iplBin, IplImage *iplMaxMin);
    static bool extrSingularBinMap5x5_8U_v2a(IplImage *iplSrcImage, IplImage *iplBin, IplImage *iplMaxMin);
    static bool extrSingularBinMap5x5_32f_v3(IplImage *iplSrcImage, IplImage *iplBin, IplImage *iplMaxMin);

	static IplImage *extrMADMap_int(IplImage *iplImage, int wnd_size, int bit_depth);
	static bool extrMADMap_int(IplImage *iplImage, IplImage *iplMADMap, int wnd_size, int bit_depth);

	static IplImage *extrMADMap5x5_int(IplImage *iplImage, int src_depth, int dst_depth);
	static bool extrMADMap5x5_Ito6U(IplImage *iplImage, IplImage *iplMADMap, int bit_depth);
	static bool extrMADMap5x5_Intto8U(IplImage *iplImage, IplImage *iplMADMap, int bit_depth);
	static bool extrMADMap5x5_8Uto8U(IplImage *iplImage, IplImage *iplMADMap);
	static bool extrMADMap5x5_8Uto8U_s5(IplImage *iplImage, IplImage *iplMADMap);
	static bool extrMADMap5x5_10Uto8U_s5(IplImage *iplImage, IplImage *iplMADMap);
	static IplImage *extrMADMap5x5_8Uto8U_s5(IplImage *iplImage);
	static IplImage *extrMADMap5x5_10Uto8U_s5(IplImage *iplImage);
	static bool extrMADMap5x5_8Uto6U(IplImage *iplImage, IplImage *iplMADMap);
	static bool extrMADMap5x5_8Uto3U(IplImage *iplImage, IplImage *iplMADMap);
	static bool extrMADMap5x5_8Uto3U_s5(IplImage *iplImage, IplImage *iplMADMap);

	static bool extrStrongGradPrior_32f(IplImage *iplImage, IplImage *iplEdge, float threshold = 64.0f);
	static bool extrBlurMap_32f(IplImage *iplImage, IplImage *iplBlur, int wnd_size = 9);

	static bool extrGradPriorMap_32f(IplImage *iplImage, IplImage *iplRecImage, IplImage *iplMap, float threshold = 0.125f);	// not used

	static bool extrGradSharpMap_32f(IplImage *iplImage, IplImage *iplMap, int d=3);

    static bool filtHolo5x5_int(IplImage *iplImage, IplImage *iplHolo);
    static IplImage *filtHolo5x5_int(IplImage *iplImage);

	//darkband detection
	static bool detDarkband_8U(IplImage *iplImage, int TH_dark, int TH_contrast, int TH_max_tb, int TH_max_lr, 
                                                int &top, int &bottom, int &left, int &right, char *szDebugFilePre=NULL);

	static bool detDarkband_32f(IplImage *iplImage, int TH_dark, int TH_contrast, int TH_max_tb, int TH_max_lr, 
                                                int &top, int &bottom, int &left, int &right);


	// synthesis
	static bool synGaussNoise_32f(IplImage *iplImage, float var = 1.0f, int seed = 17);
	static IplImage *synGaussNoise_32f(int width, int height, float var = 1.0f, int seed = 17);

	// visualization for debugging and tuning
	static bool visWeight(char stFilename[], IplImage *iplWeight, float fFactor = -1.0f, IplImage *iplLabel = NULL, int bit_depth = 8);
	static bool visWeightSum(char stFilename[], IplImage *iplSum, IplImage *iplWeight, int bit_depth = 8);
	static bool visIntRecImage(char stFilename[], IplImage *iplBuild, IplImage *iplWeight);
	static bool visPatch(char stFilename[], float *pfPatch, int width, int height, int scale = 4);

	static bool cmpImages(IplImage *iplImage1, IplImage *iplImage2, int max_error = 32, float flt_th = 0.0f, 
						  int top=0, int bottom=0, int left=0, int right=0);

	static float getImagePixel(IplImage *iplImage, int x, int y);

    static float addWatermark(IplImage *iplImage, int wm_content = 0, int wm_style = 0);

	// -------------------------------------------------------
	// Functions to build a stand-alone dll w/o OpenCV
	// -------------------------------------------------------

	// create and release image
	static IplImage *createImage( int width, int height, int depth, int channels );
    static IplImage *createImage( CvSize size, int depth, int channels );
    static IplImage *createImage(IplImage *iplImage);
	static void releaseImage(IplImage **pImg);

	// load and save image
	static bool saveImage8U(char stFilename[], IplImage *iplImage, int quality = 100);
	static IplImage *loadImage(char stFilename[], int &bit_depth);

	// basic operations
	static bool copy(IplImage *iplSrc, IplImage *iplDst);
	static bool copy(unsigned char *pSrc, IplImage *iplDst);
	static bool copy(IplImage *iplSrc, unsigned char *pDst);
	static IplImage *clone(IplImage *iplImage);
    static IplImage *clone(float *pImageData, int width, int height);
	static bool setZero(IplImage *iplImage);
    static bool setValue3(IplImage *iplImage, float R, float G=0.0f, float B=0.0f);
    static IplImage *getSubImage(IplImage *iplImage, CvRect rect);
    static bool stitchSubImage(IplImage *iplTarImage, int x, int y, IplImage *iplSubImage, CvRect rect, int method);

    static IplImage *stitchSbS(IplImage *iplLeft, IplImage *iplRight, int r, int g, int b, int thickness=0);
    static bool stitchSbS(IplImage *iplTarImage, IplImage *iplLeft, IplImage *iplRight, int r, int g, int b, int thickness=0);

	// image process functions
	static bool resize(IplImage *iplSrcImage, IplImage *iplDstImage, int inter_type = SR_INTER_CUBIC);
	static bool smooth(IplImage *iplSrcImage, IplImage *iplDstImage, int smoothtype=0, int size1=3, int size2=0, double sigma1=0, double sigma2=0);

    static bool cannyEdge_32f(IplImage *iplImage, IplImage *iplDstImage, float highThresh = 0.5f);
	// Coded by Kayton Cheung (Jul. 29, 2013)
	static bool cannyedge_MatLab_32f(IplImage *iplImage, IplImage *iplDstImage, float highThresh = 0.5f);
	static bool gaussian1D_32f(IplImage *iplImage, IplImage *iplDstImage, int wnd_rad, float sigma, int direction);
	static bool gradient1D_32f(IplImage *iplImage, IplImage *iplDstImage, int wnd_rad, float sigma, int direction);
	static bool findLocalMaxima(IplImage *iplSrcImage1, IplImage *iplSrcImage2, IplImage *iplSrcImage3, IplImage *iplDstImage1, IplImage *iplDstImage2, IplImage *iplDstImage3, IplImage *iplDstImage4, float lowThresh, float highThresh, int direction);
	static bool findConnectedComponents(IplImage *iplidxWeakSum, IplImage *iplImgCannyEdge, int ir, int  ic, int conn, int level, IplImage *iplBreakPoint, bool &has_breakpoint);

    // -------------------------------------------------------
    // Special funcations to support VEUHD DLL
    // -------------------------------------------------------
#ifdef _ASTRI_ICDD_VEUHD4K_DLL
    static VEUHDImage *createVEUHDImage( int width, int height, int depth, int colorfmt );
    static void releaseVEUHDImage(VEUHDImage **ppImg);
    static bool copy(VEUHDImage *vpSrcImage, VEUHDImage*vpDstImage);
    static IplImage *cvtVImgTo32F(VEUHDImage *vpImage, int plane);
    static VEUHDImage *cvt32FToVImg(IplImage *iplY, IplImage *iplU, IplImage *iplV, int bit_depth);
    static bool cvt32FToVImg(IplImage *iplY, IplImage *iplU, IplImage *iplV, VEUHDImage *vpImage);
#endif

	// -------------------------------------------------------
	// SIMD implementations 
    // Each function has a counterpart function without
    // suffix "_SIMD" and does not check the input arguments.
	// -------------------------------------------------------
#ifdef __SR_USE_SIMD
public: // change to pretected after tuning

    static bool cvtImage8Uto32F_SIMD(IplImage *iplImage8U, IplImage *iplImage32F);
    static bool cvtImage8Uto32F_SIMD(unsigned char *pImage8U, IplImage *iplImage32F);
    static bool cvtImage32Fto8U_SIMD(IplImage *iplImage32F, IplImage *iplImage8U, short bias, float mag_factor, int cvt_method);
    static bool cvtImage32Fto8U_SIMD(IplImage *iplImage32F, unsigned char *pImage8U, short bias, float mag_factor, int cvt_method);

    static bool cvtBGRtoYUV_32f_SIMD(IplImage *iplBGR, IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, ColorSpaceName color_sp);
    static bool cvtYUVtoBGR_32f_SIMD(IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, IplImage *iplBGR, ColorSpaceName color_sp);

    //lanczos
    static bool resizeLanczos2_4to3_32f_SIMD(IplImage *iplSrc, IplImage *iplDst);
    static bool resizeLanczos2_2to3_32f_SIMD(IplImage *iplSrc, IplImage *iplDst);
    static bool resizeLanczos2_1to3_32f_SIMD(IplImage *iplSrc, IplImage *iplDst);
    static bool resizeLanczos2_1to2_32f_SIMD(IplImage *iplSrc, IplImage *iplDst);
    static bool resizeLanczos3_4to3_32f_SIMD(IplImage *iplSrc, IplImage *iplDst);
    static bool resizeLanczos3_2to3_32f_SIMD(IplImage *iplSrc, IplImage *iplDst);
    static bool resizeLanczos3_1to3_32f_SIMD(IplImage *iplSrc, IplImage *iplDst);
    static bool resizeLanczos3_1to2_32f_SIMD(IplImage *iplSrc, IplImage *iplDst);

    static bool extrStructMap5x5_o3a_32f_SIMD(IplImage *iplImage, IplImage *iplStruct, IplImage *iplGrad, IplImage *iplCorner);
    static bool extrSingularBinMap5x5_v3_32f_SIMD(IplImage *iplSrcImage, IplImage *iplBin, IplImage *iplMaxMin);
    static bool extrSingularBinMap5x5_v3_32f_MMX(IplImage *iplSrcImage, IplImage *iplBin, IplImage *iplMaxMin);

    static bool min3x3_o1_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage);
    static bool max3x3_o1_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool sobel_o1_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage, int direction);
    static bool sobel_laplace_o1_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage);

    static bool gaussian3x3_o1_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage, float sigma);
    static bool gaussian5x5_o1_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage, float sigma);
    static bool gaussian7x7_o1_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage, float sigma);
    static bool boxing5x5_o1_32f_SIMD(IplImage *iplImage, IplImage *iplDstImage, float factor);
    static bool contrast5x5_o1_32f_SIMD(IplImage *iplPadded, IplImage *DstImage);

    static bool adaNLMFilter11x5_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage, 
		                                  float sensitivity, float hvsimpact, float adaptfactor, float sigma);

    static bool isAligned64Bytes(IplImage *iplImage) {
        return ((iplImage->widthStep % 64) == 0);
    }
    static bool isOpenCVAligned64Bytes(IplImage *iplImage);

#endif      // #ifdef __SR_USE_SIMD

    // some alternative version of SIMD implementations
#ifdef __SR_USE_SIMD
    bool extrSingularBinMap5x5_32f_v3_MMX(IplImage *iplSrcImage, IplImage *iplBin, IplImage *iplMaxMin);
#endif      // #ifdef __SR_USE_SIMD

    // -------------------------------------------------------
    // Other functions
    // -------------------------------------------------------

public:
	static inline int clip_0_255(int a) {
		a = a < 0 ? 0 : a;
		a = a > 255 ? 255 : a;
		return a;
	}

	static inline long long clip_0_255(long long a) {
		a = a < 0 ? 0 : a;
		a = a > 255 ? 255 : a;
		return a;
	}

	static inline int clip_0_256(int a) {
		a = a < 0 ? 0 : a;
		a = a > 256 ? 256 : a;
		return a;
	}

	static inline double clip_0_256(double a) {
		a = a < 0.0f ? 0.0f : a;
		a = a > 256.0f ? 256.0f : a;
		return a;
	}

	static inline float clip_0_255(float a) {
		a = a < 0.0f ? 0.0f : a;
		a = a > 255.0f ? 255.0f : a;
		return a;
	}

	static inline double clip_0_255(double a) {
		a = a < 0.0 ? 0.0 : a;
		a = a > 255.0 ? 255.0 : a;
		return a;
	}

    static inline int round_clip_0_255(float a) {
        a += 0.5f;
		a = a < 0.0f ? 0.0f : a;
		a = a > 255.0f ? 255.0f : a;
		return int(a);
	}

	static inline float clip_0_1(float a) {
		a = a < 0.0f ? 0.0f : a;
		a = a > 1.0f ? 1.0f : a;
		return a;
	}

	static inline int clip_int(int data, int up, int down) {
		if (data > up) 
			return up;
		else if (data < down)
			return down;
		else
			return data;
	}

	static inline unsigned char rndclp_9Uto8U(int a) {
        a = a < 0 ? 0 : a;									// clipping
		a = ((a&0x00000001)==0) ? (a>>1) : ((a>>1)+1);		// rounding
		a = a > 255 ? 255 : a;
		return (unsigned char)a;
	}

	static inline unsigned char rndclp_Unsigned(int a) {
        a = a < 0 ? 0 : a;									// clipping
		a = ((a&0x00000001)==0) ? (a>>1) : ((a>>1)+1);		// rounding
		return (unsigned char)a;
	}

    static inline int get_aligned_size(int size, int align_unit) {
        return ((size + align_unit - 1) / align_unit)  * align_unit;
    }

	static inline int div_Int24_8U(unsigned int N, unsigned int D);
	static inline int div_Int12_5U(unsigned int N, unsigned int D);

    static bool isValidYUV(IplImage *iplY, IplImage *iplU, IplImage *iplV);

	// for message output
#ifdef VEUHDFILTER2D_EXPORTS
    protected:
        // internal data for callback function
		// internal messages to show vis callback function
		// 0 -- no message, 1 -- error message only, 2 -- basic messages, 3 -- all messages
        static int m_nShowMsg;
        static VEUHDCore::VEUHD4K_CALLBACK_FUNC m_pCallback;	// pointer to the callback function. set NULL to disable the callback function
        static VEUHDCore::VEUHDCoreMsg *m_pCallbackMsg;			    // pointer to the message data buffer allocated by DLL caller and used by the callback function

public:
    static void setCallback(VEUHDCore::VEUHD4K_CALLBACK_FUNC pCallback, VEUHDCore::VEUHDCoreMsg *pCallbackMsg, int show_msg) {
		m_pCallback = pCallback;
		m_pCallbackMsg = pCallbackMsg;
		m_nShowMsg = show_msg;
	}
#endif      // #ifdef VEUHDFILTER2D_EXPORTS

#ifdef _SUPPORT_ASTRI_VEUHD_DLL
protected:
	// internal messages to show vis callback function
	// 0 -- no message, 1 -- error message only, 2 -- basic messages, 3 -- all messages
	static int m_nShowMsg;			
	static _VEUHD_CALL_BACK_FUNC m_pCallback;	// pointer to the callback function. set NULL to disable the callback function
	static _VEUHD_CALL_BACK_MSG *m_pCallbackMsg;			// pointer to the message data buffer allocated by DLL caller and used by the callback function

public:
	static void setCallback(_VEUHD_CALL_BACK_FUNC pCallback, _VEUHD_CALL_BACK_MSG *pCallbackMsg, int show_msg) {
		m_pCallback = pCallback;
		m_pCallbackMsg = pCallbackMsg;
		m_nShowMsg = show_msg;
	}
#endif

#ifndef __OPENCV_IMGPROC_TYPES_C_H__
    public:
        static CvSize cvSize(int width, int height) { 
            CvSize size; 
            size.width = width; size.height = height;
            return size;
        }
#endif      // #ifndef __OPENCV_IMGPROC_TYPES_C_H__

public:
	static void showMessage(char *format, ...);
	static void showMessage1(char *format, ...);		// for internal use
	static void showErrMsg(char *format, ...);

};