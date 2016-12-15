
#pragma once
#pragma warning(disable : 4996)
#pragma warning(disable : 4702) // unreachable code

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <time.h>
// LIBC.LIB Single thread static library, retail version 
// LIBCMT.LIB Multithread static library, retail version 
// MSVCRT.LIB Import library for MSVCRT.DLL, retail version 

#include <stdarg.h>
#include <math.h>
#include <float.h>
#include <vadefs.h>

#include "ComTypeDef.h"

// options

#ifdef _ASTRI_ICD_ASD_HDTO4K_DLL  // Customized HAST Engine DLL for BesTV (Phase1), NOT USED in this version
#include "CvtHDto4K.h"
#define _SUPPORT_ASTRI_VEUHD_DLL
#define _VEUHD_CALL_BACK_FUNC ASDHDto4K_CALLBACK_FUNC
#define _VEUHD_CALL_BACK_MSG ASDHDto4KMsg
#endif

#ifdef _ASTRI_ICDD_VEUHD4K_DLL    // Customized VEUHD Engine DLL for BesTV (Phase 2) and other Customers
#include "VEUHD4K.h"
#define _SUPPORT_ASTRI_VEUHD_DLL
#define _VEUHD_CALL_BACK_FUNC VEUHD4K_CALLBACK_FUNC
#define _VEUHD_CALL_BACK_MSG VEUHD4KMsg
#endif

//#define _USE_FIXED_POINT_SIM

#define MAX_FRAME_NUM_IN_FILTER     128

#define PI 3.1415926f
//#define _SR_USE_OPENCV

// -------------------------------------------------
// Licence control
// -------------------------------------------------
#ifdef _ADD_EXPIRE_TIME
#define LICENCE_EXPIRE_YEAR     2018
#define LICENCE_EXPIRE_MONTH    12
#define LICENCE_EXPIRE_DATE     31  // will be expired on this day
#endif      // #ifdef _ADD_EXPIRE_TIME

// -------------------------------------------------
// Define Macros
// -------------------------------------------------

#ifndef MIN
#define MIN(a, b)	((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef round
#define round(x) (((x) > 0) ? floor((x) + 0.5) : ceil((x) - 0.5))
#endif

// -------------------------------------------------
// SIMD
// -------------------------------------------------
#ifdef __SR_USE_SIMD
#define _SIMD_ALIGN_BYTES   64
#endif      // #ifdef __SR_USE_SIMD

// -------------------------------------------------
// Define Data Structures and Constants
// -------------------------------------------------
// constant
#define MAX_MULTIFRAME_NUM		5

// const
#define IC_MODULE_NONE  0x00000000
#define IC_MODULE_IPU	0x00000100
#define IC_MODULE_PP	0x00000200
#define IC_MODULE_SiSS	0x00000400
#define IC_MODULE_VESR	0x00000400      // share the code with SiSS, since they will not appear together
#define IC_MODULE_IBP	0x00000800
#define IC_MODULE_DB	0x00001000
#define IC_MODULE_ENC	0x00002000
#define IC_MODULE_DCU	0x00004000
#define IC_MODULE_TOP	0x00008000
#define IC_MODULE_ALL	0x0000FFFF
#define IC_MODULE_CA    0x00000001

// Data structure

typedef struct {
	//unsigned short	bfType;		// remove this field to implement the data structure in an unpack mode
	unsigned int		bfSize;
	unsigned short      bfReserved1;
	unsigned short      bfReserved2;
	unsigned int		bfOffBits;
} BMPFileHeader;

typedef struct {
	unsigned int       biSize;
	unsigned int       biWidth;
	unsigned int       biHeight;
	unsigned short     biPlanes;
	unsigned short     biBitCount;
	unsigned int       biCompression;
	unsigned int       biSizeImage;
	unsigned int       biXPelsPerMeter;
	unsigned int       biYPelsPerMeter;
	unsigned int       biClrUsed;
	unsigned int       biClrImportant;
} BMPInfoHeader;

typedef struct {
	unsigned char    rgbBlue;
	unsigned char    rgbGreen;
	unsigned char    rgbRed;
	unsigned char    rgbReserved;
} RGBQuad;

typedef enum {
	SR_BICUBIC,
    SR_LANCZOS2,
    SR_LANCZOS3,
    SR_CUBUSM,              // bicubic + (deblur) USM
	SR_EGINTP,
    SR_CEI,              // Dr. Hung's EGI
    SR_EGIAD,            // adaptive EGI (bicubic for non-edge regions)
    SR_EGI_AdENH,
	SR_BIBICU,			// bilateral-bicubic interpolation
	SR_BILINEAR,		// bilinear interpolation
    SR_NN,		            // nearest neighbourhood
	SR_EXPB_ME,
	SR_EXPB_MFME,		// multi-frame SR using ME
	SR_DEFAULT_SR_FLOAT,	// latest floating point version of SR (denoise on, de-ringing on, color enc on)
	SR_DEFAULT_SR_INT,	// latest fixed point version of SR (denoise on, de-ringing on, color enc on)
	SR_NONE,        // by pass the superresolution/upscaling in Y
	SR_B8001,		//VEUHD8K Solution Basic 1
	SR_B8002,		//VEUHD8K Solution Basic 2
	SR_G8001,		//VEUHD8K Solution Golden
	SR_G8002,		//VEUHD8K Solution Golden
	SR_G8101,		//VEUHD8K Solution Golden
	SR_G8102,		//VEUHD8K Solution Golden
    ALL_NONE        // do nothing except for file format conversion
} SuperResMethod;

typedef enum {
	PRESCALE_BICUBIC,	// pre-scaling using bicubic
	PRESCALE_BIBICU,	// pre-scaling using bicubic plus bilateral filter
	POSTSCALE_BICUBIC,	// post-scaling using bicubic
	POSTSCALE_BIBICU,	// post-scaling using bicubic pluse bilateral filter
	SCALE_NONE			// no scaling
} ScalingMethod;

typedef enum {
	// for single frame
	ME_FULLSEARCH,
	ME_FAST_NAIVE,
	ME_FAST_HEX9,
    ME_EGI,
    ME_HyEGI,
    ME_AdEGI,
	ME_SISS,
    ME_SISS_SEL,        // selective SiSS
	ME_SISS_PLUS,
	// for multi-frame
	ME_FULL_QUAR, // Nov18
	ME_FULL_HALF,
	ME_NAIVE_QUAR,		
	ME_NAIVE_HALF,
	MF_SIMILAR,
	MF_SHIFT
} SearchMethod;

typedef enum {
	PATCH_SHAPE_1MODE,
	PATCH_SHAPE_10MODES
} PatchShape;

typedef enum {
	MATCH_METHOD_MSE,
	MATCH_METHOD_SAD,
	MATCH_METHOD_MAX
} MatchMethod;

typedef enum {
	WEIGHT_WHOLE_DIRECT,		// use patch matching reconstruction only
	WEIGHT_WHOLE_FUSION,		// fusion of patch matching reconstruction and EGI interpolation
	WEIGHT_LOCATION_GAUSSIAN,
	WEIGHT_DIST_MAP,
	WEIGHT_HYBRID				// distmap + siss
} ReconstructWeigth;

typedef enum {
	PREFILTER_NONE,
    PREFILTER_GAUSSIAN,
	PREFILTER_BILATERAL,
    PREFILTER_ADPTBL,
    PREFILTER_NLMF,                     // non-local mean filter
    PREFILTER_AdNLMF,                // adaptive non-local mean filter
    PREFILTER_BL_MOSQUITO,      // selective BL to remove MOSQUITO
    FGN_SYN_PRE_GAUSSIAN,       // removal file grain noise using BL and add back using Gaussian model BEFORE SR
	FGN_SYN_POST_GAUSSIAN,		// removal film grain noise using BL and then add back after SR
	PREFILTER_ABF
} PreFilterMethod;

typedef enum {
	EGME_NONE,
	EGME_LAP3x3
} EdgeGuideMEMethod;

typedef enum {
	DECOMP_BILATERAL,
	DECOMP_NONE
} ImageDecompMethod;

typedef enum {
	TEX_UP_CUBIC,
	TEX_UP_PYRAMID,
	TEX_UP_EGINTP
} TextureUpMethod;

typedef enum {
	BP_NONE,				// no back-projection
	BP_TRADITIONAL,			// traditional backprojection using interpolation
	BP_DUAL1,				// two-phase backprojection in two seperate loops
	BP_DUAL2,				// two-phase backprojection in a single loop
	BP_NLIBP,				// non-local means back-projection
	BP_BLIBP,				// bilateral back-projection
	BP_EDBIBP,				// bilateral iterative back-projection with embedded deblur
	BP_TEDBIBP,				// iterative back-projection with tensor-based embedded deblur
	BP_AdIBPDB,				// improved BP_TEDBIBP, using adaptive projection filters
    BP_FGadIBPDB,			// BP_AdIBPDB + film grain noise synthesis in IBP
    BP_AdBLIBPDB,           // similar to BP_AdIBPDB, but use adaptive bilateral filter as projection filter in IBP and DB
    BP_FastIBPDB,           // fast IBP and deblur
    BP_FPBPDB,              // forward projection + backprojection + deblur
    BP_FPBPUSM,             // forward projection + backprojection + usm (rational unsharp masking)
    BP_SVOIBP,              // SVO-IBP described in paper "a fast single-frame super-resolution using subjective visibility optimization"
} BackProjectionMethod;

typedef enum {
	CA_TENSOR,              // regularity (tensor) + corner + flat detection (local variance) + post-process
	CA_MTENSOR,             // regularity (tensor, multi-scale) + corner + flat detection (local variance)
    CA_JND,                 // just noticeable distortion (JND) by Yang (luminance + contrast)
    CA_JNDTENSOR,           // combination of CA_TENSOR and CA_JND
    CA_CCTV,                // content analysis for surveillance apps (strong edge masking)
	CA_NONE
} ContentAnalysisMethod;

typedef enum {
	SFMF_AVERAGE,			// average
	SFMF_WINNER_ALL,		// winner takes all
	SFMF_DEFAULT
} SFMFFusionMethod;

typedef enum {
    CHROMA_FMT_RGB8,         // 8-bit RGB 4:4:4, for VEUHD PCIE card simulation only
    CHROMA_FMT_RGB444,
	CHROMA_FMT_YUV444,
	CHROMA_FMT_YUV420,
	CHROMA_FMT_YUV420L,		// 4:2:0-like format, resample from 4:4:4 by interlacing
	CHROMA_FMT_YUV422,
	CHROMA_FMT_HD2SD4,		// 4:2:2 for 1080p input, 4:4:4 for SD input
} ChromaFormat;

typedef enum {
	CHROMA_NTSC = 0x01,
	CHROMA_BT601 = 0x10,
    CHROMA_BT624 = 0x11,
	CHROMA_BT709 = 0x20,
    CHROMA_YCbCr = 0x30,       // YCbCr in OpenCV
    CHROMA_YUV = 0x40,         // YUV in OpenCV
    CHROMA_YUVMS = 0x41,    // YUV recommended by Miscrosoft, similar to FFMPEG
	CHROMA_YCC = 0x50,		// OpenCV for JPEG, aka YCC
} ColorSpaceName;

typedef enum {
	CHROMA_RESIZE_BILINEAR,
	CHROMA_RESIZE_BICUBIC,
	CHROMA_RESIZE_BICUBLIN
} ChromaResizeMethod;

typedef enum {
    DEMO_MODE_None = 0,
    DEMO_MODE_SbS = 1,
    DEMO_MODE_HH = 2,
    DEMO_MODE_ROI = 3
} DemoModeMethod;

typedef enum {
    WATERMARK_None = 0,
    WATERMARK_ASTRI = 1,
    WATERMARK_Trial = 2,
    WATERMARK_VEUHD = 3,
} WatermarkContent;

typedef struct {
    int a0, a1, a2, a3;
} QuadrupleInt;

// bit-depth conversion
// bit-depth reduction
#define SR_DEPTHCVT_TRUNC   0x00000001       // truncation
#define SR_DEPTHCVT_ROUND   0x00000002       // rounding
#define SR_DEPTHCVT_EDFS    0x00000004       // Floyd-Steinberg error diffusion
#define SR_DEPTHCVT_EDJJN   0x00000008       // Jarvis-Judice-Ninke error diffusion
#define SR_DEPTHCVT_ED2x2   0x00000010       // 2x2 simplified error diffusion
    // bit-depth increase
#define SR_DEPTHCVT_ZEROS   0x00010000       // add zeros in LSBs
#define SR_DEPTHCVT_RANDOM  0x00020000       // add random bits in LSBs

typedef struct {
	// parameters from config file
	SuperResMethod sr_method;		// SR method

	PreFilterMethod pre_filter;		// pre_filter method
	int PFRaidan;					// radian of filter window
	float PFPara1;
	float PFPara2;

	SearchMethod search_method;		// search method
	PatchShape patch_shape;			// patch shape used
	MatchMethod match_method;		// patch match method, e.g. MSE, SAD, MAX, etc.
	
	int MELayerNum;				// number of layers in ME patch matching
	int SearchRangeX;			// search range of ME patch matching
	int SearchRangeY;
	bool PredictMV;				// 1 -- using searching result of neighbour pixels to accelerate the search; 0 -- disable

	int InterSearchFrame;		// number of reference frames in SR search
	int InterSearchRange;		// range in inter-frame search, in pixel (LR image)
	int InterPredictMV;         // 0 -- disable Inter-frame MV prediction
	                            // 1 -- MVP method 1: fast search at key pixel and MVP at others
	int InterRoiMethod;         // 0 -- Disable ROI processing 
	                            // 1 -- geometrical ROI tactics: center of frame
	int InterBlendMFMotion;     // '0' -- default, Blend weight only based on MatchMethod
	                            // '1' -- motion size 
	                            // '2' -- neighbor motion smooth in MVP
	                            // '3' -- both motion size and neighbor motion smooth
	SearchMethod Inter_search_method;		// search method in multi-frame ME
	SearchMethod Inter_Search_Space;		// search spacing method in multi-frame ME

	ContentAnalysisMethod CAMethod;	// content analysis method
    float CABiasFactor;             // bias to CA map, -1~1, 0.5 by default
                        // CABiasFactor:    -1.0    -0.5        0.0     0.5     1.0
                        // Resutant HVS:     0.0    0.5*hvs     0.5     hvs     1.0

	SFMFFusionMethod SFMF_Fusion;	// method to fuse single- and multi-frame SR

	ImageDecompMethod ImageDecomp;	// image decomposition
	EdgeGuideMEMethod EdgeGuideME;	// edge guided ME method
	float EdgePointRatio;			// ratio of edge points selected for edge guided ME
	int DFRaidan;					// radian of filter window
	float DFPara1;
	float DFPara2;
	TextureUpMethod TexUpscale;		// texture/residual upscaling method

	int opt_level;				// optimization level for HAST fixed-point implementation and VEUHD floating-point and fixed-point implementations
	int BPSC_ContAnalysis;		// BPSC of content analysis
	int BPSC_PreDeblur;			// BPSC of pre-deblur
	int BPSC_SharpComp;			// BPSC of sharpness compensation
	int BPSC_SiSS;				// BPSC of SiSS
	int BPSC_BPI1;				// BPSC of IBP1
	int BPSC_BPI2;				// BPSC of IBP1
	int BPSC_Deblur;			// BPSC of deblur
    int BPSC_aUSM;            // BPSC of adaptive-USM
    int BPSC_AdDE;            // BPSC of advanced detail enhancement

	int LRPatchSize;
	int HRPatchSize;

	bool InterpolationGrid;		// true -- reconstruct HR image at the interpolation grid (same location as interpolation), x2 as example:
								// x  x  x  x  x  x  x  x
								// o  x  o  x  o  x  o  x   <-- 'o' pixels in LR image
								// x  x  x  x  x  x  x  x	<-- 'x' new pixels in HR image
								// false -- reconstruct HR image at the resize grid (same location as image resize), x2 as example:
								// x   x   x   x   x   x   x   x
								//   o       o       o       o      <-- 'o' pixels in LR image
								// x   x   x   x   x   x   x   x	<-- 'x' new pixels in HR image
	bool ReconstructOneLayer;	// true -- only reconstruct one layer using SR, followed by post-processing resizing
	bool ReconstructMissing;	// only reconstuct the "missing" pixel
	float SiSSThreshold4x4;		// reconstruction only for 4x4 patch with SiSS larger than 'SiSSThreshold4x4'
	float SiSSThreshold8x8;		// reconstruction only for 8x8 patch with SiSS larger than 'SiSSThreshold8x8'
	ReconstructWeigth rec_weight_method;	// weighting method for patch inpainting
	float SSD_Sigma;			// Small SSD_Sigma, stronger weight on similar patches, default is 25, higher SSD_Sigma, the generated patch tends to average
	float DistMap_Sigma;		// Sigma parameter to calculate pixel weight, only used in SiSS (PatchReconstructWeight = hybrid)

	float fFusionThHigh;		// threshold to fusion the reconstruction layer and the draw-back layer, reconstruction only when weight > fFusionThHigh
	float fFusionThLow;		// threshold to fusion the reconstruction layer and the draw-back layer, draw-back only when weight <= fFusionThLow

	int InPlaceSR;          // 0 -- disable inplace SR; 1 -- full inplace SR (two-layer, downsampled patch in all the layers)
                            // 2 -- partial inplace SR (two-layer, downsampled patch in the first layer, fast algo only)
                            // 3 -- partial inplace SR (two-layer, downsampled patch in the second layer, fast algo only, BKM)
                            // 4 -- one-layer and downsampled patch (fast algo only)
	int PreDeblur;			// special design option for HD-to-UHD conversion
                                    // 0 -- disable
                                    // 1 -- enable (normal predeblur)
                                    // 2 -- denoise mode 1 (for binary-image)
                                    // 3 -- denoise mode 2 (for text image)
                                    // 4 -- denoise mode 3 (for natural scene)

	BackProjectionMethod Backprojection;	// back-projection method
	int BPLoopNum;				// loop number of back-projection
	int BPLoopNum2;				// loop number of back-projection or for deblur (TEDBIBP or ADIBPDB)\n");
	bool BPLoopReduction;		// reduce projection loop number while building higher layers
	bool CompatibleMode;		// compatible mode where the reference image for IBP is downsampled to simulate the LR image, only used in "in-place" SR
	int BPCannyThMin;			// min threshold in canny edge detector
	int BPCannyThMax;			// max threshold in canny edge detector
	float GauVarProjection;		// variance of Gaussian filter in projection
	float GauVarBackProj;		// variance of Gaussian filter in back-projection
	float GauVarInplace;		    // filter parameters for in-place SR
    float EdgeEnhPara;           // strength of edge enhancement
    float DetailEnhPara;          // strength of detail enhancement
    float USMPara;                 // strength of USM

	// parameters from arguments
	int TarWidth;			// size of the target image
	int TarHeight;
	float Zooming;			// scaling factor

	// parameters from input
	int SrcWidth;
	int SrcHeight;

	// pre- or post-scaling
	ScalingMethod ScalingMethod; 

	// internal parameters
	float ScalePerLayer;
	int BuildLayerNum;		// Layer number to be built for up-scaling, equals to ceil(log(Zooming) / log(ScalePerLayer))

} ExpbMEPara;

typedef enum {
	ENH_PRE_LUMA_BLENC,		// pre-enhancement using BL based decomposition

	ENH_POST_LUMA_BLENC,	// post-enhancement using BL based decomposition
	ENH_POST_LUMA_LAP4,
	ENH_POST_LUMA_LAP8,

    ENH_POST_ADAPT_CONTRAST,  // adaptive contrast enhancement, 22/1/2015
    ENH_POST_ADAPT_CONTRAST_ST,  // adaptive contrast enhancement with HVS
    ENH_POST_SIGMOID_CONTRAST,  // non-linear contrast enhancement in sigmoid space
    ENH_POST_CONTRAST_STRETCH,  // contrast stretch

	ENH_POST_DYNAMIC_CONTRAST,  // dynamic adaptive contrast enhancement, 10/6/2015

	ENH_LUMA_NONE			// no enhancement
} EncMethod;

typedef enum {
    ENH_CHROMA_ADAPTCbCr,   // adaptive Cb and Cr stretching (by Tim)
    ENH_CHROMA_SPEST,           // style-preserving color enhancement (adaptive Cb and Cr stretching, by Tim) tuned by structure-texture map
    ENH_CHROMA_SMARTSAT,    // smart saturation enhancement considering facial color tone (Luhong)

    ENH_CHROMA_NONE         // no enhancement
} ChromaMethod;

typedef struct : public ExpbMEPara {

	// public options
    int input_bit_depth;        // record the data precision (bit-depth) of the input images
    int output_bit_depth;       // record the data precision (bit-depth) of the output images
	int internal_data_type;		// data type in internal image representation, could be SR_DEPTH_32S, SR_DEPTH_32F and customized
	int internal_bit_depth;		// bit depth actual used in internal image representation, specially for integer data type
	int OutputLevel;			// level of intermediate result output
								// 0 -- none, 1 -- only output most important ones, 2 -- output all, -1 -- decided by configure file
	int TestVector;				// to indicate which module outputs testing vector
	bool EchoOptions;			// if output options

	// denoise
	bool NR_FilmGrain;			// film grain noise reduction
    int NR_Contour;           // contour removal
    int NR_Jaggy;              // "jaggy" artifacts reduction
	bool NR_Blocking;			// blocking artifacts reduction
	bool NR_Ringing;			// ringing artifacts reduction
    bool NR_FrameDelay;         // 'true' denotes using previous noise level (for H/W); otherwise using current noise level (for S/W)
	int BPSC_Denoise;			// bypass and strength control (BPSC) mode ID of denoise
	bool NoAutoDetNoise;		// turn off auto noise level detection to manually set the DNLevel
	int  DNLevel;				// indicate de-noise level: = 0 ~ 6 from weak to strong, when NoAutoDet is set to true 

	// pre-filters, pre-scaling
    //int RectPixSupport;			// 0 -- only support square pixel; 1-- support 4:3 pixel shape (HK SD format) by pre-scaling in x-direction
    int pitch_width;                // describe the shape of the pixels on screen
    int pitch_height;
    bool isRectPitch(void) { return (pitch_width == pitch_height); }

	// SR options is inherited from ExpbMEPara

	// enhancement (luma)
	EncMethod LumaEncMethod;
	float LumaEncFactor;
    bool LumaDarkbanEn;         // darkband detection 
	int BPSC_ContrastEnc;		// BPSC of contrast enhancement
    float LumaEnhWfDarka;       // dark region weighting factor constant
    float LumaEnhWfMinda;
    float LumaEnhWfBrighta;
    int LumaEnhBlkConTh;        // Noise level within 3x3 block contrast analysis
    int LumaEnhMaxYeq;          // Maximum adjustment after histogram equalization from original pixel value

	// enhacement (color)
	ChromaFormat ChromaInputFmt;	// format of color data fed into the simulation engine, 4:4:4 or 4:2:2, for opt_level >=5 only
	ChromaFormat ChromaInterFmt;	// Denotes internal format, for opt_level = 5~9 only
    ColorSpaceName ChromaOutputFmt; // color space of the ouput, for opt_level = 5~9
	ChromaResizeMethod ChromaResize;// method to resize chroma

	ChromaMethod ColorEnhMethod;	// color enhancement method
    float ChromaEnhFactor;      // enhancement factor, >= 1.0
    bool ColorDarkbandEn;       // darkband detection
    int ColorCbf;               // 
    int ColorCrf;               //	
    int ColorTHVar;			    // level (strength) of color enhancement, 0~127
	int ColorEncDarkLevel;		// parameter for color enhancement
	int ColorEncBrightLevel;	// parameter for color enhancement
    int BPSC_ChromaEnc;			// BPSC of chroma (saturation) enahcement

    // demo mode
    DemoModeMethod DemoMode;    // default is none
    SuperResMethod BenchmarkMethod; // could only be SR_NN, SR_BICUBIC, SR_EGINTP, SR_CUBUSM, SR_CEI, SR_BIBICU, and SR_BILINEAR
    QuadrupleInt DemoPara;      // ROI or seperation line color for demo
                                // (left,top,width,height) in LR for 'sbs' and 'roi'
                                // (only 'left' is effective in FPGA sim)
                                // (r,g,b,thickness) in LR for 'hh'
    WatermarkContent Watermark;

	// public options

#ifdef _SUPPORT_ASTRI_VEUHD_DLL
	// internal data for callback function
	// internal messages to show vis callback function
	// 0 -- no message, 1 -- error message only, 2 -- basic messages, 3 -- all messages
	int show_msg;
	_VEUHD_CALL_BACK_FUNC pCallback;	// pointer to the callback function. set NULL to disable the callback function
	_VEUHD_CALL_BACK_MSG *pCallbackMsg;			// pointer to the message data buffer allocated by DLL caller and used by the callback function
#endif      // #ifdef _SUPPORT_ASTRI_VEUHD_DLL

} HASTPara;

typedef struct {
	char stSourceFile[256];
	char szResultFile[256];
    int nStartFrameNum, nEndFrameNum;
    float fInputFrameRate, fOutputFrameRate;
    float fInputBitRate, fOutputBitRate;
} IOFileFormat;



// -----------------------------------------------
// definiations for filter-based system
// -----------------------------------------------

#define MAX_FILTER_PARA_NUM     8                               // support 8 parameters for the filter
#define MAX_FILTER_LIST_SIZE     1024                             // support 8 parameters for the filter

// Defination of one filter.
// All the fitlers that will be appied to one frame are orgianized to a link list,.
// namely filter combination. The filters in the list will be applied to the frame
// one by one in a order from head to tail.
typedef struct VEUHDFilter {
    VEUHDFilterName name;   // code to represent the filter name
    float mag_factor;       // magification factor: 1.0 or 2.0
    int pre_frame;          // previous frames used in this filter, for spatial-temporal ("3D") fitler only
    int post_frame;         // successive frames used in this filter, for spatial-temporal ("3D") fitler only
    int para_num;                             // parameter number of the filter
    int para_list[MAX_FILTER_PARA_NUM];       // list of parameters
    VEUHDFilter *next;                          // pointer to the next filter that will be applied to the frame
} VEUHDFilter;

