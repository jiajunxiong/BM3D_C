#pragma once
#pragma warning(disable : 4996)
#pragma warning(disable : 4702) // unreachable code


// -------------------------------------------------
// OpenCV Headers
// Will not use OpenCV by default
// -------------------------------------------------
#ifdef _SR_USE_OPENCV       
#include "opencv/cv.h"
#include "opencv/cvaux.h"
#include "opencv/highgui.h"
#endif		// #ifdef _SR_USE_OPENCV


// -------------------------------------------------
// Define Data Structures Compatible with OpenCV
// -------------------------------------------------

#ifdef __OPENCV_IMGPROC_TYPES_C_H__

#define SR_DEPTH_1U    IPL_DEPTH_1U
#define SR_DEPTH_8U    IPL_DEPTH_8U
#define SR_DEPTH_16U   IPL_DEPTH_16U
#define SR_DEPTH_32F   IPL_DEPTH_32F
#define SR_DEPTH_64F   IPL_DEPTH_64F

#define SR_DEPTH_8S  IPL_DEPTH_8S
#define SR_DEPTH_16S IPL_DEPTH_16S
#define SR_DEPTH_32S IPL_DEPTH_32S


// Sub-pixel interpolation methods
enum
{
	SR_INTER_NN        = CV_INTER_NN,
    SR_INTER_LINEAR    = CV_INTER_LINEAR,
    SR_INTER_CUBIC     = CV_INTER_CUBIC,
    SR_INTER_AREA      = CV_INTER_AREA,
    SR_INTER_LANCZOS4  = CV_INTER_LANCZOS4,
    SR_INTER_LANCZOS2  = CV_INTER_LANCZOS4 + 0x80,
    SR_INTER_LANCZOS3  = CV_INTER_LANCZOS4 + 0x81
};

enum
{
    SR_BLUR_NO_SCALE = CV_BLUR_NO_SCALE,
    SR_BLUR  = CV_BLUR,
    SR_GAUSSIAN  = CV_GAUSSIAN,
    SR_MEDIAN = CV_MEDIAN,
    SR_BILATERAL = CV_BILATERAL
};

#else	// #ifdef __OPENCV_IMGPROC_TYPES_C_H__

#define SR_DEPTH_SIGN 0x80000000

#define SR_DEPTH_1U     1
#define SR_DEPTH_8U     8
#define SR_DEPTH_16U   16
#define SR_DEPTH_32F   32
#define SR_DEPTH_64F  64

#define SR_DEPTH_8S  (SR_DEPTH_SIGN| 8)
#define SR_DEPTH_16S (SR_DEPTH_SIGN|16)
#define SR_DEPTH_32S (SR_DEPTH_SIGN|32)

// Sub-pixel interpolation methods
enum
{
    SR_INTER_NN        = 0,
    SR_INTER_LINEAR    = 1,
    SR_INTER_CUBIC     = 2,
    SR_INTER_AREA      = 3,
    SR_INTER_LANCZOS4  = 4,
    SR_INTER_LANCZOS2  = SR_INTER_LANCZOS4 + 0x80,
    SR_INTER_LANCZOS3  = SR_INTER_LANCZOS4 + 0x81
};

enum
{
    SR_BLUR_NO_SCALE =0,
    SR_BLUR  =1,
    SR_GAUSSIAN  =2,
    SR_MEDIAN =3,
    SR_BILATERAL =4
};

typedef struct _IplROI
{
    int  coi; //0 - no COI (all channels are selected), 1 - 0th channel is selected ...
    int  xOffset;
    int  yOffset;
    int  width;
    int  height;
}
IplROI;

typedef struct CvPoint
{
    int x;
    int y;
}
CvPoint;

typedef struct CvRect
{
    int x;
    int y;
    int width;
    int height;
}
CvRect;

typedef struct
{
    int width;
    int height;
}
CvSize;

// definition of data structure
typedef struct _IplImage
{
    int  nSize;                    // sizeof(IplImage)
    int  ID;                        //version (=0)
    int  nChannels;           //Most of OpenCV functions support 1,2,3 or 4 channels 
    int  alphaChannel;      //Ignored by OpenCV 
    int  depth;                 //Pixel depth in bits: SR_DEPTH_8U, SR_DEPTH_8S, SR_DEPTH_16S,
                                    //SR_DEPTH_32S, SR_DEPTH_32F and IPL_DEPTH_64F are supported.  
    char colorModel[4];     //Ignored by OpenCV 
    char channelSeq[4];     //ditto 
    int  dataOrder;         //0 - interleaved color channels, 1 - separate color channels.
                                    //createImage can only create interleaved images 
    int  origin;            //0 - top-left origin,
                               // 1 - bottom-left origin (Windows bitmaps style).  
    int  align;             //Alignment of image rows (4 or 8).
                               //OpenCV ignores it and uses widthStep instead.    
    int  width;             //Image width in pixels.                           
    int  height;            //Image height in pixels.                          
    struct _IplROI *roi;    //Image ROI. If NULL, the whole image is selected. 
    struct _IplImage *maskROI;      //Must be NULL. 
    void  *imageId;                 //"           " 
    void *tileInfo;					// struct _IplTileInfo *tileInfo;  //"           "  <------ NOTE:  not used in our implementation
    int  imageSize;         //Image data size in bytes
                               //(==image->height*image->widthStep
                               //in case of interleaved data)
    char *imageData;        //Pointer to aligned image data.         

    int  widthStep;         //Size of aligned image row in bytes.    
    int  BorderMode[4];     //Ignored by OpenCV.                     
    int  BorderConst[4];    //Ditto.                                 
    char *imageDataOrigin;  //Pointer to very origin of image data
                               //(not necessarily aligned) - needed for correct deallocation 
}
IplImage;

#endif	// #ifdef __OPENCV_IMGPROC_TYPES_C_H__	// film grain noise reduction


// -----------------------------------------------
// definiations for filter-based system
// -----------------------------------------------

typedef enum {
    VEUHD_FILTER_NONE = 0x0000,

    // for  2K to 4K engine

    RESIZE_BICUBIC = 0x01,      // SR_BICUBIC
    RESIZE_BILINEAR = 0x02,     // SR_BILINEAR
    RESIZE_BLBICU = 0x03,       // SR_BIBICU (bilateral bicubic)
    RESIZE_NN = 0x04,           // SR_NN
    RESIZE_LANCZOS2 = 0x05,     // SR_LANCZOS2
    RESIZE_LANCZOS3 = 0x06,     // SR_LANCZOS3
    RESIZE_CEI = 0x21,          // resize using directional interpolation
    RESIZE_CER_RUSM = 0x31,       // SR_CER_RUSM

    INTER_BILINEAR = 0x40,       // bicubic bilienar
    INTER_BILINEAR_ODD = 0x41,   // bicubic bilienar, from odd lines/columns ( x o x o)
    INTER_BICUBIC = 0x42,        // bicubic interpolation
    INTER_BICUBIC_ODD = 0x43,    // bicubic interpolation, from odd lines/columns ( x o x o)
    INTER_EGI = 0x50,            // SR_EGINTP
    INTER_EGI_ODD = 0x51,        // EGI from odd lines/columns ( x o x o)
    INTER_EGIAD = 0x52,          // SR_EGIAD
	INTER_CEI = 0x53,            // SR_CEI
	INTER_CEI_RUSM = 0x54,       // SR_CEI_RUSM
	INTER_CEI_RUSM_2to3 = 0x55,  // SR_CEI_RUSM_2to3
    
    // Patch searching based methods
    UPSCALE_MESR_LATEST = 0x80,  // latest patch searching based method
    UPSCALE_MESR45 = 0x81,       // patch searching based method, 4x4 to 5x5
    UPSCALE_MESR48 = 0x82,       // patch searching based method, 4x4 to 8x8 (latest version)
    // SiSS for HAST
    UPSCALE_HAST_LASTEST = 0x90,        // HAST latest version
    UPSCALE_SISS48 = 0x91,              // SiSS based method for HAST, the basic algorithm of HAST FPGA (latest version)
    UPSCALE_SISS48PLUS = 0x93,          // SiSS based method for HAST with local search
    // SiSS for VEUHD
    UPSCALE_VEUHD_SISS = 0x98,          // lastest SISS based version for VEUHD
    UPSCALE_SISS68 = 0x99,              // an earlier version of SiSS based method for VEUHD
    UPSCALE_SISS4K68 = 0x9a,            // SiSS based method for VEUHD
    UPSCALE_SISS4K68SEL = 0x9b,         // Selective SiSS based method
    // Fast Algorithm for VEUHD
    UPSCALE_VEUHD_LATEST = 0xa0,        // VEUHD latest version
    UPSCALE_VEUHD_V1 = 0xa1,            // VEUHD version v1
    UPSCALE_VEUHD_V3 = 0xa3,            // VEUHD version v3
    UPSCALE_VEUHD_CEI = 0xa8,           // VEUHD 1.5 (CEI based VEUHD)
    // Super-SR
    UPSCALE_VEUHD2_CNN_TXOx2 = 0xb1,    // Original PAMI 2015 algoirthm by Prof. Xiaoou Tang
    UPSCALE_VEUHD2_SUPERSR0 = 0xb2,      // CA + CEI + CNN (using Prof. Tang's CNN model)

    VEUHD_FILTER_LAST_UPSCALE = 0x00FF,

    // Enhancement in luminance
    VEUHD_FILTER_LUMA_FIRST = 0x0100,

    VEUHD_FILTER_CONTRAST_SPE = 0x0101,         // style-preserving contrast enhancement
    VEUHD_FILTER_CONTRAST_STRETCH = 0x0102,     // adaptive contrast stretching
    VEUHD_FILTER_CONTRAST_DYNAMIC = 0x0103,     // dynamic contrast
	VEUHD_FILTER_CONTRAST_DYNAMIC_MF_INIT = 0x0104,     // dynamic contrast
	VEUHD_FILTER_CONTRAST_DYNAMIC_MF = 0x0105,  // dynamic contrast
    VEUHD_FILTER_TONE_MAP_ADHE = 0x0106,        // adaptive histogram equalization based tone mapping
    VEUHD_FILTER_GAMMA = 0x0107,                // Gamma correction

    VEUHD_FILTER_USM = 0x0181,                  // unsharp masking

    VEUHD_FILTER_LUMA_LAST = 0x01FF,

    // Enhancement in chroma
    VEUHD_FILTER_CHROMA_FIRST = 0x0200,

    VEUHD_FILTER_COLOR_SPE = 0x0201,            // style-preserving color enhancement
    VEUHD_FILTER_COLOR_STPE = 0x0202,           // skin-color-tone-preserving color enhancement
    VEUHD_FILTER_COLOR_AWB = 0x0203,            // basic AWB (auto white balance)
    VEUHD_FILTER_COLOR_SATADJ = 0x0204,         // saturation enhancement

    VEUHD_FILTER_CHROMA_LAST = 0x02FF,

    // noise & artifacts reduction
    VEUHD_FILTER_NLMF = 0x0301,                 // adaptive non-local mean filter
    VEUHD_FILTER_RMJAGGY = 0x0302,              // jaggy removal
    VEUHD_FILTER_BLF = 0x0303,                  // bilateral filter
    VEUHD_FILTER_ADBLF = 0x0304,                // adaptive bilateral filter
    VEUHD_FILTER_FGTRANS = 0x0305,              // artifacts to film grain
    VEUHD_FILTER_DEMOS = 0x0306,                // mosquito removal

    VEUHD_FILTER_LAST_DENOISE = 0x03FF,

    // GEO filters
    VEUHD_FILTER_LAST_GEO = 0x06FF,

    // other accessory filters
    VEUHD_FILTER_ADDDB = 0x700,
    VEUHD_FILTER_LAST_ACC = 0x7FF,
    
    VEUHD_FILTER_LAST_2D = 0x07FF,

    // "3D" filters

    VEUHD_FILTER_FIRST_ST = 0x0800,

    VEUHD_FILTER_NLMF_ST = 0x0801,     // spatial-temporal NLFM

    VEUHD_FILTER_LAST_ST = 0x08FF

} VEUHDFilterName;