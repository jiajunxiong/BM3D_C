
////////////////////////////////////////////////////////////////////////////////////////////////////
// This is optimized functions accelerated by SIMD instructions
// References include:
// https://software.intel.com/sites/products/documentation/studio/composer/en-us/2011Update/compiler_c/index.htm#cref_cls/common/cppref_class_cpp_simd.htm
// by Luhong Liang, IC-ASD, ASTRI
// (2014)
////////////////////////////////////////////////////////////////////////////////////////////////////

//
// Function List
//


//
// Functions
//

#ifdef __SR_USE_SIMD

#include <dvec.h>
#include <intrin.h>
#include <malloc.h>

#define _SR_SIMD_MAX_VEC            8
#define _SR_SIMD_MAX_BLOCK      64

//#define SR_DEPTH_1U    IPL_DEPTH_1U

//#define SR_DEPTH_16U   IPL_DEPTH_16U
//#define SR_DEPTH_32F   IPL_DEPTH_32F
//#define SR_DEPTH_64F   IPL_DEPTH_64F


//#define SR_DEPTH_16S IPL_DEPTH_16S
//#define SR_DEPTH_32S IPL_DEPTH_32S

bool CImageUtility::isOpenCVAligned64Bytes(IplImage *iplImage)
// check if each line of the input image is 64-type aligned, IF IT IS allocated by native OpenCV function (which is 4-byte algined)
{
    if (iplImage->depth == SR_DEPTH_1U) {
        int opencv_widthStep = ((((iplImage->width * iplImage->nChannels + 7) / 8) + 3) / 4) * 4;   // 4-byte aligned
        return ((opencv_widthStep % 64) == 0);
    } else if (iplImage->depth == SR_DEPTH_8U || iplImage->depth == SR_DEPTH_8S) {
        int opencv_widthStep = ((iplImage->width * iplImage->nChannels + 3) / 4) * 4;
        return ((opencv_widthStep % 64) == 0);
    } else if (iplImage->depth == SR_DEPTH_16U || iplImage->depth == SR_DEPTH_16S) {
        int opencv_widthStep = ((iplImage->width * iplImage->nChannels * 2 + 3) / 4) * 4;
        return ((opencv_widthStep % 64) == 0);
    } else if (iplImage->depth == SR_DEPTH_32F || iplImage->depth == SR_DEPTH_32S) {
        int opencv_widthStep = iplImage->width * iplImage->nChannels * 4;
        return ((opencv_widthStep % 64) == 0);
    } else if (iplImage->depth == SR_DEPTH_64F) {
        int opencv_widthStep = iplImage->width * iplImage->nChannels * 8;
        return ((opencv_widthStep % 64) == 0);
    } 
    
    showErrMsg("Unsupported data type in CImageUtility::isOpenCVAligned64Bytes()!\n");
    return false;
}

bool CImageUtility::extrStructMap5x5_o3a_32f_SIMD(IplImage *iplImage, IplImage *iplStruct, IplImage *iplGrad, IplImage *iplCorner)
// SIMD accelerated version (use SSE3 instrinsics)
// NOTE: since there mayeb some error beteen the SIMD and the SISD instrinsics, the result of this function maybe a little bit different to
//            the original one!
// Calculate the tensor map of an image, using 5x5 window
// Supposing gx = Gradx(I) and gy = Grady(I), the tensor matrix is T = [ sum(gx*gx) sum(gx*gy); sum(gy*gx), sum(gy*gy) ]
// the tensor-based isotropy at a point is (eg1 - eg2) * (eg1 - eg2) / ((eg1 + eg2) * (eg1 + eg2)). Moreover, the corner
// point likelihood is alphs*eg2. The function map a sum of the isotropy and the corner likelihood to 0~1, where 1 denotes 
// the pixel in structure region. This function supports the tensor estimation on a downsampled image, where ds_level denotes 
// the level (1/2) of the pyramid image (0 denotes no downsample)
// Different from function extrStructMap5x5_o1_32f(), this function
//      (1) abandoned the pyramid image (feature of o2)
//      (2) reduced the memory consumption (feature of o2)
//      (3) only supports the 32F input and output (feature of o2)
//      (4) check the gradient and set low gradient region as "structure"
//      (5) Output corner detection result (optional)
// Compared with extrStructMap5x5_o3_32f(), this function is a fast version with sobel gradient output:
//      (6) The input image iplImage has been padded 3 pixels in each direction
//      (7) move the Sobel + Max5x5 into this function and use boxing filter instead of max filter
// Compared with non-SSE version, this function:
//      (1)
// Argument:
//		iplImage -- [I] 1-channel input image, 32F or 8U datatype
//		iplStruct -- [O] 1-channel result image, 32F datatype
//     iplGrad -- [O] sobel gradient
//     iplCorner -- [O] optional output of corner detection
// by Luhong Liang, IC-ASD, ASTRI, March 10, 2014
// July 18, 2014: Output corner detection result (optional)
{ 
	//if (iplImage == NULL || iplStruct == NULL || iplImage->nChannels != 1 || 
    //    iplStruct->nChannels != 1 || iplGrad->nChannels != 1 ||
    //    iplImage->depth != SR_DEPTH_32F || iplStruct->depth != SR_DEPTH_32F || iplGrad->depth != SR_DEPTH_32F ||
    //    iplImage->width != iplStruct->width+6 || iplImage->height != iplStruct->height+6 ||
    //    iplStruct->width != iplGrad->width || iplStruct->height != iplGrad->height) {
	//	showErrMsg("Invalid input image type or argument in CImageUtility::extrStructMap5x5_o3a_32f()!\n");
	//	return false;
	//}

    if (iplCorner != NULL) {
        showErrMsg("Not support corner map output in SIMD version of CImageUtility::extrStructMap5x5_o3a_32f()!\n");
        return false;
    }

    //empty();        // flush the MMX registers
     
    // allocate aligned buffers
    const int align_floats = _SIMD_ALIGN_BYTES / sizeof(float);
	IplImage *iplGx = createImage( iplImage->width-2, iplImage->height-2, SR_DEPTH_32F, 1 );
	IplImage *iplGy = createImage( iplImage->width-2, iplImage->height-2, SR_DEPTH_32F, 1 );
	IplImage *iplGxy = createImage( iplImage->width-2, iplImage->height-2, SR_DEPTH_32F, 1 );
    IplImage *iplGm = createImage( iplImage->width-2, iplImage->height-2, SR_DEPTH_32F, 1 );
    int aligned_size_line = get_aligned_size(iplImage->width, align_floats);
    __declspec(align(64)) float *pGxxLine = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
    __declspec(align(64)) float *pGxyLine = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
    __declspec(align(64)) float *pGyyLine = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
	if (iplGx == NULL || iplGy == NULL || iplGxy == NULL || iplGm == NULL ||
		pGxxLine == NULL || pGxyLine == NULL || pGyyLine == NULL)  {
		safeReleaseImage(&iplGx, &iplGy, &iplGxy, &iplGm);
        _aligned_free(pGxxLine);
        _aligned_free(pGxyLine);
        _aligned_free(pGyyLine);
		showErrMsg("Fail to allocate image buffer in CImageUtility::extrStructMap5x5_o3a_32f()!\n");
		return false;
	}

	// calculate gradient
    static const __m128 vec_const0 = _mm_set1_ps(0.0f);
    static const __m128 vec_consto16 = _mm_set1_ps(1.0f/16.0f);
    static const __m128 vec_consto8 = _mm_set1_ps(1.0f/8.0f);
    static const __m128 SIGNMASK = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)); 
    //int most_right = (aligned_size_line - 4) < (iplImage->width-2) ? (aligned_size_line - 8) : (iplImage->width-2);
    int most_right = (aligned_size_line - 4) < (iplImage->width-2) ? (aligned_size_line - 4) : (iplImage->width-2);
    for (int y=0; y<(iplImage->height-2); y++) {
        __declspec(align(64)) float *pSrcN = (__declspec(align(64)) float *)(iplImage->imageData + y * iplImage->widthStep);        // each line of the image has been aligned
        __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplImage->imageData + (y+1) * iplImage->widthStep);
        __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplImage->imageData + (y+2) * iplImage->widthStep);
        __declspec(align(64)) float *pGmm = (__declspec(align(64)) float *)(iplGm->imageData + y * iplGm->widthStep);
        __declspec(align(64)) float *pGmx = (__declspec(align(64)) float *)(iplGx->imageData + y * iplGx->widthStep);
        __declspec(align(64)) float *pGmy = (__declspec(align(64)) float *)(iplGy->imageData + y * iplGy->widthStep);
        __declspec(align(64)) float *pGmxy = (__declspec(align(64)) float *)(iplGxy->imageData + y * iplGxy->widthStep);
        // pre-calculate in Y direction
        int x;
        for (x=0; x<iplImage->width; x+=4) {
            //pGxxLine[x] = pSrcN[x]  + pSrc0[x]  + pSrc0[x] + pSrc1[x];      // 1 2 1 in column
            F32vec4 vec_p0 = _mm_load_ps(pSrcN+x);
            F32vec4 vec_p1 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_p2 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_grad_x = _mm_add_ps(vec_p0, vec_p1);
            vec_grad_x = _mm_add_ps(vec_grad_x, vec_p1);
            vec_grad_x = _mm_add_ps(vec_grad_x, vec_p2);
			//pGyyLine[x] = pSrc1[x] - pSrcN[x];                                          // -1 0 1 in column
            F32vec4 vec_grad_y = _mm_sub_ps(vec_p2, vec_p0);
            // write back
            _mm_store_ps(pGxxLine+x, vec_grad_x);
            _mm_store_ps(pGyyLine+x, vec_grad_y);
        }
        for (; x<iplImage->width; x++) {        // remaining pixels
            // -1 0 1
            // -2 0 2
            // -1 0 1
            pGxxLine[x] = pSrcN[x]  + pSrc0[x]  + pSrc0[x] + pSrc1[x];      // 1 2 1 in column
            // -1  -2  -1
            //  0   0   0
            //  1   2   1
			pGyyLine[x] = pSrc1[x] - pSrcN[x];                                          // -1 0 1 in column
        }

        // calculate the filter
        // pre load 4 elements
        F32vec4 vec_gx0 = _mm_load_ps(pGxxLine);
        F32vec4 vec_gy0 = _mm_load_ps(pGyyLine);
        for (x=0; x<most_right; x+=4) {
            int xx = x + 4;
            //float grad_x = - pGxxLine[x] + pGxxLine[x+2];
            F32vec4 vec_gx1 = _mm_load_ps(pGxxLine+xx);
            F32vec4 vec_gy1 = _mm_load_ps(pGyyLine+xx);
            // Gx:
            //                       gx0     |      gx1
            //                  0  1  2  3  |  4  5  6  7
            //  to get:      2  3  4  5 using shuffle(gx0, gx1, [1 0 3 2])
            F32vec4 vec_gx2 = _mm_shuffle_ps(vec_gx0, vec_gx1, _MM_SHUFFLE(1, 0, 3, 2));
            // ---------------- Usage of _MM_SHUFFLE -------------------
            // vec0.m_128_f32[4] = abcd;  vec0.m_128_f32[4]=efgh
            // vec2 = _mm_shuffle_ps(vec0, vec1, _MM_SHUFFLE(i3, i2, i1, i0))
            // Then:
            // vec2.m_128_f32[4] = vec0(i0) vec0(i1) vec1(i2) vec1(i3)
            // e.g.
            // // vec0=1234 vec1=5678 -->_mm_shuffle_ps(vec0, vec1, _MM_SHUFFLE(1, 2, 0, 3))=4176
            // ---------------------------------------------------------------
            
            // Gy: 
            //                       gx0     |      gx1
            //                  0  1  2  3  |  4  5  6  7
            //  to get:      2  3  4  5 using shuffle(gx0, gx1, [1 0 3 2])
            F32vec4 vec_gy2 = _mm_shuffle_ps(vec_gy0, vec_gy1, _MM_SHUFFLE(1, 0, 3, 2));
            //  to get:      1  2  3  4  using gx2=shuffle(gx0, gx1, [0 1 2 1]), gx3=shuffle(gx0, gx1, [0, 1, 3, 2] and shuffle(gx2, gx3, [3, 1, 1, 0])
            F32vec4 vec_gy3 = _mm_shuffle_ps(vec_gy0, vec_gy1, _MM_SHUFFLE(0, 1, 2, 1));        // 1 2 5 4
            F32vec4 vec_gy4 = _mm_shuffle_ps(vec_gy0, vec_gy1, _MM_SHUFFLE(0, 1, 3, 2));        // 2 3 5 4
            vec_gy3 = _mm_shuffle_ps(vec_gy3, vec_gy4, _MM_SHUFFLE(3, 1, 1, 0));                     // 1 2 3 4

            // calculate the final filter response
            vec_gx2 = _mm_sub_ps(vec_gx2, vec_gx0);
            vec_gy2 = _mm_add_ps(vec_gy2, vec_gy0);     // g0 + g3 + g3 + g2
            vec_gy2 = _mm_add_ps(vec_gy2, vec_gy3);
            vec_gy2 = _mm_add_ps(vec_gy2, vec_gy3);
            // pGmx[x] = (grad_x * grad_x) * 0.0625f;
            F32vec4 vec_gxx = _mm_mul_ps(vec_gx2, vec_gx2);
            vec_gxx = _mm_mul_ps(vec_gxx, vec_consto16);
            // pGmy[x] = (grad_y * grad_y) * 0.0625f;
            F32vec4 vec_gyy = _mm_mul_ps(vec_gy2, vec_gy2);
            vec_gyy = _mm_mul_ps(vec_gyy, vec_consto16);
            // pGmxy[x] = (grad_x * grad_y) * 0.0625f;
            F32vec4 vec_gxy = _mm_mul_ps(vec_gx2, vec_gy2);
            vec_gxy = _mm_mul_ps(vec_gxy, vec_consto16);
            //grad_x = grad_x < 0.0f ? - grad_x : grad_x;
            vec_gx2 = _mm_and_ps(vec_gx2, SIGNMASK);
            //grad_y = grad_y < 0.0f ? - grad_y : grad_y;
            vec_gy2 = _mm_and_ps(vec_gy2, SIGNMASK);
            //pGmm[x] = (grad_x + grad_y) * 0.125f;
            F32vec4 vec_gm = _mm_add_ps(vec_gx2, vec_gy2);
            vec_gm = _mm_mul_ps(vec_gm, vec_consto8);
         
            // swap registers
            vec_gx0 = vec_gx1;
            vec_gy0 = vec_gy1;

            // copy data
            _mm_store_ps(pGmx+x, vec_gxx);
            _mm_store_ps(pGmy+x, vec_gyy);
            _mm_store_ps(pGmxy+x, vec_gxy);
            _mm_store_ps(pGmm+x, vec_gm);
		}
        for (; x<(iplImage->width-2); x++) {    // remaining pixels
            // -1 0 1
            // -2 0 2
            // -1 0 1
            float grad_x = - pGxxLine[x] + pGxxLine[x+2];
            // Gxx
            pGmx[x] = (grad_x * grad_x) * 0.0625f;

            // -1  -2  -1
            //  0   0   0
            //  1   2   1
			float grad_y = pGyyLine[x] + pGyyLine[x+1] + pGyyLine[x+1] + pGyyLine[x+2];
            // Gyy
            pGmy[x] = (grad_y * grad_y) * 0.0625f;

            // Gxy 
            pGmxy[x] = (grad_x * grad_y) * 0.0625f;
            
            // Gm
            grad_x = grad_x < 0.0f ? - grad_x : grad_x;
            grad_y = grad_y < 0.0f ? - grad_y : grad_y;
            pGmm[x] = (grad_x + grad_y) * 0.125f;
		}

    }
    // saveImage("_Gm_o3a_new.bmp", iplGm);    saveImage("_Gxx_o3a_new.bmp", iplGx);    saveImage("_Gyy_o3a_new.bmp", iplGy);    saveImage("_Gxy_o3a_new.bmp", iplGxy);

    // max filter
    boxing5x5_o1_32f(iplGm, iplGrad);
    //saveImage("_boxing5x5_simd.bmp", iplGrad);

    // Tensor
    __declspec(align(64)) float _buf_vec_pxx[4], _buf_vec_pxy[4], _buf_vec_pyy[4];
    __declspec(align(64)) float _buf_vec_ten[4], _buf_vec_gabs[4];
    static const __m128 vec_const4 = _mm_set1_ps(4.0f);
    static const __m128 vec_const1 = _mm_set1_ps(1.0f);
    static const __m128 vec_const_div_cor = _mm_set1_ps(1.0f/(1024.0f * 128.0f));
    static const __m128 vec_const_eps = _mm_set1_ps(0.0001f);
    static const __m128 vec_const_upb = _mm_set1_ps(65535.0f);

    F32vec4 vec_tmp0, vec_tmp1, vec_k, vec_eg1, vec_eg2, vec_edge_reg, vec_corner, vec_w_g;
    F32vec4 vec00, vec01, vec02, vec03, vec04;
    F32vec4 vec10, vec11, vec12, vec13, vec14;

	for (int y=0; y<iplStruct->height; y++) {
		float *pTen = (float*)((char *)iplStruct->imageData + y * iplStruct->widthStep);
        __declspec(align(64)) float *pGabs = (__declspec(align(64)) float *)(iplGm->imageData + (y+2) * iplGm->widthStep);
        __declspec(align(64)) float *pGxx0 = (__declspec(align(64)) float *)(iplGx->imageData + y * iplGx->widthStep);
        __declspec(align(64)) float *pGyy0 = (__declspec(align(64)) float *)(iplGy->imageData + y * iplGy->widthStep);
        __declspec(align(64)) float *pGxy0 = (__declspec(align(64)) float *)(iplGxy->imageData + y * iplGxy->widthStep);
        __declspec(align(64)) float *pGxx1 = (__declspec(align(64)) float *)(iplGx->imageData + (y+1) * iplGx->widthStep);
        __declspec(align(64)) float *pGyy1 = (__declspec(align(64)) float *)(iplGy->imageData + (y+1) * iplGy->widthStep);
        __declspec(align(64)) float *pGxy1 = (__declspec(align(64)) float *)(iplGxy->imageData + (y+1) * iplGxy->widthStep);
        __declspec(align(64)) float *pGxx2 = (__declspec(align(64)) float *)(iplGx->imageData + (y+2) * iplGx->widthStep);
        __declspec(align(64)) float *pGyy2 = (__declspec(align(64)) float *)(iplGy->imageData + (y+2) * iplGy->widthStep);
        __declspec(align(64)) float *pGxy2 = (__declspec(align(64)) float *)(iplGxy->imageData + (y+2) * iplGxy->widthStep);
        __declspec(align(64)) float *pGxx3 = (__declspec(align(64)) float *)(iplGx->imageData + (y+3) * iplGx->widthStep);
        __declspec(align(64)) float *pGyy3 = (__declspec(align(64)) float *)(iplGy->imageData + (y+3) * iplGy->widthStep);
        __declspec(align(64)) float *pGxy3 = (__declspec(align(64)) float *)(iplGxy->imageData + (y+3) * iplGxy->widthStep);
        __declspec(align(64)) float *pGxx4 = (__declspec(align(64)) float *)(iplGx->imageData + (y+4) * iplGx->widthStep);
        __declspec(align(64)) float *pGyy4 = (__declspec(align(64)) float *)(iplGy->imageData + (y+4) * iplGy->widthStep);
        __declspec(align(64)) float *pGxy4 = (__declspec(align(64)) float *)(iplGxy->imageData + (y+4) * iplGxy->widthStep);
		// sum in Y directions
		for (int x=0; x<iplGx->width; x+=4) {
            // xx
            F32vec4 vec_p0 = _mm_load_ps(pGxx0+x); 
            F32vec4 vec_p1 = _mm_load_ps(pGxx1+x); 
            F32vec4 vec_p2 = _mm_load_ps(pGxx2+x); 
            F32vec4 vec_p3 = _mm_load_ps(pGxx3+x); 
            F32vec4 vec_p4 = _mm_load_ps(pGxx4+x); 
            vec_p0 = _mm_add_ps(vec_p0, vec_p1);
            vec_p0 = _mm_add_ps(vec_p0, vec_p2);
            vec_p0 = _mm_add_ps(vec_p0, vec_p3);
            vec_p0 = _mm_add_ps(vec_p0, vec_p4);
            // round to avoid error incalculation (in isolated single point in testing pattern) -- by Luhong,  July 19, 2014
            vec_p0 = _mm_round_ps(vec_p0, _MM_ROUND_NEAREST);   
            _mm_store_ps(pGxxLine+x, vec_p0);
            // xy
            vec_p0 = _mm_load_ps(pGxy0+x); 
            vec_p1 = _mm_load_ps(pGxy1+x); 
            vec_p2 = _mm_load_ps(pGxy2+x); 
            vec_p3 = _mm_load_ps(pGxy3+x); 
            vec_p4 = _mm_load_ps(pGxy4+x); 
            vec_p0 = _mm_add_ps(vec_p0, vec_p1);
            vec_p0 = _mm_add_ps(vec_p0, vec_p2);
            vec_p0 = _mm_add_ps(vec_p0, vec_p3);
            vec_p0 = _mm_add_ps(vec_p0, vec_p4);
            vec_p0 = _mm_round_ps(vec_p0, _MM_ROUND_NEAREST);
            _mm_store_ps(pGxyLine+x, vec_p0);
            // yy
            vec_p0 = _mm_load_ps(pGyy0+x); 
            vec_p1 = _mm_load_ps(pGyy1+x); 
            vec_p2 = _mm_load_ps(pGyy2+x); 
            vec_p3 = _mm_load_ps(pGyy3+x); 
            vec_p4 = _mm_load_ps(pGyy4+x); 
            vec_p0 = _mm_add_ps(vec_p0, vec_p1);
            vec_p0 = _mm_add_ps(vec_p0, vec_p2);
            vec_p0 = _mm_add_ps(vec_p0, vec_p3);
            vec_p0 = _mm_add_ps(vec_p0, vec_p4);
            vec_p0 = _mm_round_ps(vec_p0, _MM_ROUND_NEAREST);
            _mm_store_ps(pGyyLine+x, vec_p0);
		}
		// tensor calculation
        int x;
		for (x=0; x<iplStruct->width; x+=4) {
            //if (x == 896 && y==52) 
            //    int p=0;
            // calculate tensor matrix T = [pxx pxy; pxy pyy] in wnd_num * wnd_num window
            for (int n=0; n<4; n++) {
                int xx = x + n;
                _buf_vec_pxx[n] = pGxxLine[xx] + pGxxLine[xx+1] + pGxxLine[xx+2] + pGxxLine[xx+3] + pGxxLine[xx+4];
                _buf_vec_pxy[n] = pGxyLine[xx] + pGxyLine[xx+1] + pGxyLine[xx+2] + pGxyLine[xx+3] + pGxyLine[xx+4];
                _buf_vec_pyy[n] = pGyyLine[xx] + pGyyLine[xx+1] + pGyyLine[xx+2] + pGyyLine[xx+3] + pGyyLine[xx+4];

                _buf_vec_gabs[n] = pGabs[x+n+2];        // +2 for alignment
            }

            // calculate eigenvalues
            F32vec4 vec_pxx = _mm_load_ps(_buf_vec_pxx); 
            F32vec4 vec_pxy = _mm_load_ps(_buf_vec_pxy); 
            F32vec4 vec_pyy = _mm_load_ps(_buf_vec_pyy); 
            F32vec4 vec_gabs = _mm_load_ps(_buf_vec_gabs); 

            // float k = (pxx+pyy) * (pxx+pyy) - 4 * (pxx * pyy - pxy * pxy);
            vec_tmp0 = _mm_mul_ps(vec_pxx, vec_pyy);        // pxx * pyy
            vec_tmp1 = _mm_mul_ps(vec_pxy, vec_pxy);        // pxy * pxy
            vec_tmp0 = _mm_sub_ps(vec_tmp0, vec_tmp1);   // (pxx * pyy - pxy * pxy)
            vec_tmp0 = _mm_mul_ps(vec_tmp0, vec_const4);// 4 * (pxx * pyy - pxy * pxy)
            vec_tmp1 = _mm_add_ps(vec_pxx, vec_pyy);       // (pxx+pyy), NOTE: vec_tmp1 will be used later
            vec_k = _mm_mul_ps(vec_tmp1, vec_tmp1);        // (pxx+pyy) * (pxx+pyy) 
            vec_k = _mm_sub_ps(vec_k, vec_tmp0);

            // if ( k<=0.0f) {
            __m128 vec_is_neg_k = _mm_cmple_ps(vec_k, vec_const0);    // (a0 <= b0) ? 0xffffffff : 0x0

            // temp1 = (float)sqrt(k);
            vec_k = _mm_max_ps(vec_k, vec_const0);      // make k >= 0
            vec_tmp0 = _mm_sqrt_ps(vec_k);                  

            // temp2 = (pxx + pyy); done above
            // eg1 =  temp2 + temp1;		// since temp2 > 0 && temp1 > 0, eg1 must be larger than eg2
            vec_eg1 = _mm_add_ps(vec_tmp0, vec_tmp1);
            // eg2 = temp2 - temp1;
            vec_eg2 = _mm_sub_ps(vec_tmp1, vec_tmp0);
            // eg2 = eg2 < 0 ? -eg2 : eg2;	// abs
            vec_eg2 = _mm_and_ps(vec_eg2, SIGNMASK);
            
            // calculate the degree of anisotropy 
            //if (eg2 == 0.0f) {
            __m128 vec_is_eg2_zero = _mm_cmpeq_ps(vec_eg2, vec_const0);    // (a0 == b0) ? 0xffffffff : 0x0
            // edge_regular = (eg1 - eg2) * (eg1 - eg2) / ((eg1 + eg2) * (eg1 + eg2));
            vec_eg2 = _mm_add_ps(vec_eg2, vec_const_eps);          // add a small bias to aviod divided-by-zero
            vec_tmp0 = _mm_add_ps(vec_eg1, vec_eg2);                // (eg1 + eg2)
            vec_tmp0 = _mm_mul_ps(vec_tmp0, vec_tmp0);           // ((eg1 + eg2) * (eg1 + eg2))
            vec_tmp1 = _mm_sub_ps(vec_eg1, vec_eg2);                // (eg1 - eg2) 
            vec_tmp1 = _mm_mul_ps(vec_tmp1, vec_tmp1);           //  (eg1 - eg2) * (eg1 - eg2)
            vec_edge_reg = _mm_div_ps(vec_tmp1, vec_tmp0);      // (eg1 - eg2) * (eg1 - eg2) / ((eg1 + eg2) * (eg1 + eg2))
            // if (eg2 == 0.0f) {edge_regular = 1.0;	}	// NOTE: we consider a constant window as "well-aligned" (i.e. regular region)
            vec_tmp0 = _mm_and_ps(vec_const1, vec_is_eg2_zero);
            vec_edge_reg = _mm_max_ps(vec_edge_reg, vec_tmp0);

            // calculate corner
            vec_corner = _mm_mul_ps(vec_eg2, vec_const_div_cor);    // corner = eg2 / (1024.0f * 128.0f);
            vec_corner = _mm_min_ps(vec_corner, vec_const1);          // corner = corner > 1.0f ? 1.0f : corner;
                    
            // weight for gradient
            vec_w_g = _mm_sub_ps(vec_const4, vec_gabs);                 // (4.0f - gabs)
            vec_w_g = _mm_div_ps(vec_w_g, vec_const4);                   // w_g = (4.0f - gabs) / 4.0f;
            __m128 vec_is_g_lt4 = _mm_cmplt_ps(vec_gabs, vec_const4);    // (a0 < b0) ? 0xffffffff : 0x0
            vec_w_g = _mm_and_ps(vec_w_g, vec_is_g_lt4);                // if (gabs>=4) w_g = 0.0f;

            // fusion
            //ten = (edge_regular + corner) * (1.0f - w_g) + w_g;
            vec_edge_reg = _mm_add_ps(vec_edge_reg, vec_corner);        // (edge_regular + corner) 
            vec_tmp0 = _mm_sub_ps(vec_const1, vec_w_g);                      //  (1.0f - w_g) 
            vec_tmp0 = _mm_mul_ps(vec_tmp0, vec_edge_reg);               // (edge_regular + corner) * (1.0f - w_g) 
            vec_tmp0 = _mm_add_ps(vec_tmp0, vec_w_g);
            vec_tmp0 = _mm_min_ps(vec_tmp0, vec_const1);                   // ten = ten > 1.0f ? 1.0f : ten;

            // if ( k<=0.0f) pTen[x+n] = 1.0f;                     // This means the isolated points, considered as structure!!!!!
            vec_tmp1 = _mm_and_ps(vec_const1, vec_is_neg_k);
            vec_tmp0 = _mm_max_ps(vec_tmp0, vec_tmp1);

            // pTen[x+n] = ten;
            _mm_store_ps(_buf_vec_ten, vec_tmp0);     // TODO: store directly to pTen (by align the buffer of IplImage)
            pTen[x] = _buf_vec_ten[0];
            pTen[x+1] = _buf_vec_ten[1];
            pTen[x+2] = _buf_vec_ten[2];
            pTen[x+3] = _buf_vec_ten[3];
		}

        // other columns
		for (; x<iplStruct->width; x++) {
            // calculate tensor matrix T = [pxx pxy; pxy pyy] in wnd_num * wnd_num window
            // calculate sum in right-most column
            int index = x+4;
            float pxx = pGxx0[index] + pGxx1[index] + pGxx2[index] + pGxx3[index] + pGxx4[index];
            float pxy = pGxy0[index] + pGxy1[index] + pGxy2[index] + pGxy3[index] + pGxy4[index];
            float pyy = pGyy0[index] + pGyy1[index] + pGyy2[index] + pGyy3[index] + pGyy4[index];
            // store in line buffers
            pGxxLine[index] = pxx;
            pGxyLine[index] = pxy;
            pGyyLine[index] = pyy;
            // check gradient
            float gabs = pGabs[x+2];        // +2 for alignment
            // sum to other columns
            pxx += pGxxLine[x] + pGxxLine[x+1] + pGxxLine[x+2] + pGxxLine[x+3];
            pxy += pGxyLine[x] + pGxyLine[x+1] + pGxyLine[x+2] + pGxyLine[x+3];
            pyy += pGyyLine[x] + pGyyLine[x+1] + pGyyLine[x+2] + pGyyLine[x+3];
            // calculate eigenvalues
            float k = (pxx+pyy) * (pxx+pyy) - 4 * (pxx * pyy - pxy * pxy);
            //pTen[x] = k / 256.0f; continue;
            if ( k<=0.0f) {
                // This means the isolated points, considered as structure!!!!!
                pTen[x] = 1.0f;
            } else {
			    float temp1 = (float)sqrt(k);// * 0.5f;
			    float temp2 = (pxx + pyy);// * 0.5f;
			    float eg1 =  temp2 + temp1;		// since temp2 > 0 && temp1 > 0, eg1 must be larger than eg2
			    float eg2 = temp2 - temp1;
			    eg2 = eg2 < 0 ? -eg2 : eg2;	// abs
			    // calculate the degree of anisotropy 
			    float edge_regular;
			    if (eg2 == 0.0f) {		// this equals to eg1 == eg2 == 0!
				    edge_regular = 1.0;		// NOTE: we consider a constant window as "well-aligned" (i.e. regular region)
			    } else {
				    edge_regular = (eg1 - eg2) * (eg1 - eg2) / ((eg1 + eg2) * (eg1 + eg2));
			    }
			    // calculate corner
			    float corner = eg2 / (1024.0f * 128.0f);
			    corner = corner > 1.0f ? 1.0f : corner;
                // weight for gradient
                float w_g = 0.0f;
                if (gabs < 4.0f) {
                    w_g = (4.0f - gabs) / 4.0f;
                }
			    // fusion
                float ten = (edge_regular + corner) * (1.0f - w_g) + w_g;
                //ten = ten < 0.0f ? 0.0f : ten;
                ten = ten > 1.0f ? 1.0f : ten;
			    pTen[x] = ten;
                //pTen[x] = w_g;       // for debug only
            }
		}
	}
    //saveImage("_TensorCorner.bmp", iplStruct, 0, 255.0f);

    safeReleaseImage(&iplGx, &iplGy, &iplGxy, &iplGm);
    _aligned_free(pGxxLine);
    _aligned_free(pGxyLine);
    _aligned_free(pGyyLine);

	return true;
}

bool CImageUtility::extrSingularBinMap5x5_v3_32f_SIMD(IplImage *iplSrcImage, IplImage *iplBin, IplImage *iplMaxMin)
// SIMD accelerated version (use SSE3 instrinsics)
// Detect the singular point region (like speckle), binary pattern structure region and the local difference of max-min
// This function is an optimized version of function extrSingularBinMap5x5_32f(), where the singluar number calculation
// part is removed.
// This function supposes the input image iplSrcImage has been padded 2 pixels in each direction!
// Arguments:
//		iplSrcImage -- [I] input image; must be 32F floating point, 1-channel image, 2-pixel padded in boundaries
//     iplBin -- [O] binary pattern measurement, 0 represents the pixel most like binary pattern
//     iplMaxMin -- [O] difference of maximum and minimun value in the window
// by Luhong Liang, IC-ASD, ASTRI
// June 19, 2014
{
	//if (iplSrcImage == NULL || iplBin == NULL || iplMaxMin == NULL ||
    //    iplSrcImage->depth != SR_DEPTH_32F || iplBin->depth != SR_DEPTH_32F || iplMaxMin->depth != SR_DEPTH_32F ||
    //    iplSrcImage->width != iplBin->width+4 || iplSrcImage->height != iplBin->height+4 ||
    //    iplSrcImage->width != iplMaxMin->width+4 || iplSrcImage->height != iplMaxMin->height+4) {		// TODO: complete check needed
	//	showErrMsg("Invalid input/output image in CImageUtility::extrSingularBinMap5x5_32f_v3()!\n");
	//	return false;
	//}

    //empty();        // flush the MMX registers

    const float min_diff_th = 16.0f;
    F32vec4 vec_min_diff_th(min_diff_th, min_diff_th, min_diff_th, min_diff_th);
    __declspec(align(64)) float _simd_img_block[_SR_SIMD_MAX_BLOCK]; 
    __declspec(align(64)) float _simd_img_vec[_SR_SIMD_MAX_VEC]; 

    for (int y=0; y<iplBin->height; y++) {
        float *pSrc0 = (float *)((char *)iplSrcImage->imageData + y * iplSrcImage->widthStep);
        float *pSrc1 = (float *)((char *)iplSrcImage->imageData + (y+1) * iplSrcImage->widthStep);
        float *pSrc2 = (float *)((char *)iplSrcImage->imageData + (y+2) * iplSrcImage->widthStep);
        float *pSrc3 = (float *)((char *)iplSrcImage->imageData + (y+3) * iplSrcImage->widthStep);
        float *pSrc4 = (float *)((char *)iplSrcImage->imageData + (y+4) * iplSrcImage->widthStep);
        float *pBin = (float *)((char *)iplBin->imageData + y * iplBin->widthStep);
        float *pDif = (float *)((char *)iplMaxMin->imageData + y * iplMaxMin->widthStep);
        // set up the initial block data
        _simd_img_block[0]=pSrc0[0];  _simd_img_block[1]=pSrc1[0];  _simd_img_block[2]=pSrc2[0];  _simd_img_block[3]=pSrc3[0];  _simd_img_block[4]=pSrc4[0];  
        _simd_img_block[5]=pSrc0[1];  _simd_img_block[6]=pSrc1[1];  _simd_img_block[7]=pSrc2[1];  _simd_img_block[8]=pSrc3[1];  _simd_img_block[9]=pSrc4[1]; 
        _simd_img_block[10]=pSrc0[2];  _simd_img_block[11]=pSrc1[2];  _simd_img_block[12]=pSrc2[2];  _simd_img_block[13]=pSrc3[2];  _simd_img_block[14]=pSrc4[2]; 
        _simd_img_block[15]=pSrc0[3];  _simd_img_block[16]=pSrc1[3];  _simd_img_block[17]=pSrc2[3];  _simd_img_block[18]=pSrc3[3];  _simd_img_block[19]=pSrc4[3]; 
        //_simd_img_block[20]=pSrc0[4];  _simd_img_block[21]=pSrc1[4];  _simd_img_block[22]=pSrc2[4];  _simd_img_block[23]=pSrc3[4];  _simd_img_block[24]=pSrc4[4]; 
        int update_index = 20;
        for (int x=0; x<iplBin->width; x++) {
            //
            // TODO: process multple blocks to reduce the memory data load/store!
            //

            // update a new row
            _simd_img_block[update_index] = pSrc0[x+4];
            _simd_img_block[update_index+1] = pSrc1[x+4];
            _simd_img_block[update_index+2] = pSrc2[x+4];
            _simd_img_block[update_index+3] = pSrc3[x+4];
            _simd_img_block[update_index+4] = pSrc4[x+4];
            update_index += 5;
            update_index = update_index % 25;
            
            // fill SIMD registers
            F32vec4 vec0 = _mm_load_ps(_simd_img_block); 
            F32vec4 vec1 = _mm_load_ps(_simd_img_block+4); 
            F32vec4 vec2 = _mm_load_ps(_simd_img_block+8); 
            F32vec4 vec3 = _mm_load_ps(_simd_img_block+12); 
            F32vec4 vec4 = _mm_load_ps(_simd_img_block+16); 
            F32vec4 vec5 = _mm_load_ps(_simd_img_block+20); 
            F32vec4 vec6 = _mm_set1_ps(_simd_img_block[24]); 

            // min external 
            F32vec4 vec_min0, vec_min1, vec_min2, vec_max0, vec_max1, vec_max2;
            vec_min0 = _mm_min_ps(vec0, vec1);
            vec_min1 = _mm_min_ps(vec2, vec3);
            vec_min2 = _mm_min_ps(vec4, vec5);
            vec_min1 = _mm_min_ps(vec_min0, vec_min1);
            vec_min0 = _mm_min_ps(vec_min1, vec_min2);
            vec_min0 = _mm_min_ps(vec_min0, vec6);        // last pixel
            // min internal 
            vec_min1 = _mm_movelh_ps(vec_min0, vec_min0);       // 0101: abcd --> abab
            vec_min0 = _mm_min_ps(vec_min0, vec_min1);                 // x x min(a,c), min(b,d)
            vec_min1 = _mm_unpackhi_ps(vec_min0, vec_min0);    //2233: min(a,c), min(a,c), min(b,d), min(b,d)
            vec_min0 = _mm_min_ps(vec_min0, vec_min1);                // x x (min(a,b,c,d), x
            vec_min0 = _mm_unpackhi_ps(vec_min0, vec_min0);   // 2233 --> min, min, x, x
            vec_min0 =_mm_unpacklo_ps(vec_min0, vec_min0);    // 0011-->min, min, min, min

            // max external
            vec_max0 = _mm_max_ps(vec0, vec1);
            vec_max1 = _mm_max_ps(vec2, vec3);
            vec_max2 = _mm_max_ps(vec4, vec5);
            vec_max1 = _mm_max_ps(vec_max0, vec_max1);
            vec_max0 = _mm_max_ps(vec_max1, vec_max2);
            vec_max0 = _mm_max_ps(vec_max0, vec6);        // last pixel
            // max internal
            vec_max1 = _mm_movelh_ps(vec_max0, vec_max0);       // 0101
            vec_max0 = _mm_max_ps(vec_max0, vec_max1);                 // x x max(0,2), max(1,3)
            vec_max1 = _mm_unpackhi_ps(vec_max0, vec_max0);    //2233
            vec_max0 = _mm_max_ps(vec_max0, vec_max1);                // x x (max(0,1,2,3), x
            vec_max0 = _mm_unpackhi_ps(vec_max0, vec_max0);     // 2233 --> max, max, x, x
            vec_max0 =_mm_unpacklo_ps(vec_max0, vec_max0);      // 0011-->max, max, max, max

            // binary likelihood
            F32vec4 vec10 = vec_max0 - vec0;     vec0 = vec0 - vec_min0;      vec0 = _mm_min_ps(vec0, vec10);
            F32vec4 vec11 = vec_max0 - vec1;     vec1 = vec1 - vec_min0;      vec1 = _mm_min_ps(vec1, vec11);
            F32vec4 vec12 = vec_max0 - vec2;     vec2 = vec2 - vec_min0;      vec2 = _mm_min_ps(vec2, vec12);
            F32vec4 vec13 = vec_max0 - vec3;     vec3 = vec3 - vec_min0;      vec3 = _mm_min_ps(vec3, vec13);
            F32vec4 vec14 = vec_max0 - vec4;     vec4 = vec4 - vec_min0;      vec4 = _mm_min_ps(vec4, vec14);
            F32vec4 vec15 = vec_max0 - vec5;     vec5 = vec5 - vec_min0;      vec5 = _mm_min_ps(vec5, vec15);
            F32vec4 vec16 = vec_max0 - vec6;     vec6 = vec6 - vec_min0;      vec6 = _mm_min_ps(vec6, vec16);     // last pixel

            vec0 = vec0 + vec1;            vec2 = vec2 + vec3;            vec4 = vec4 + vec5;
            vec2 = vec2 + vec4;            vec0 = vec0 + vec2;  // sub-sum in vec0
            vec0 = _mm_add_ss(vec0, vec6);      // last pixel

            vec0 = _mm_hadd_ps(vec0, vec0);           // a0+a1, a2+a3, b0+b1,b2+b3:  sum(a,b), sum(c,d), sum(a,b), sum(c,d)
            vec0 = _mm_hadd_ps(vec0, vec0);           // a0+a1, a2+a3, b0+b1,b2+b3:  (sum, sum, sum, sum) as acc_diff

            // diff
            F32vec4 vec_diff = vec_max0 - vec_min0; // diff, diff, diff, diff

            // bin div
            F32vec4 vec_bin_div = _mm_max_ps(vec_diff, vec_min_diff_th);

            // vec_diff:   a, a, a, a
            // vec0     :  b, b, b, b         (acc_diff)
            // bin_div :  c, c,  c, c
            vec0 = _mm_unpackhi_ps(vec0, vec_diff);           // a2b2a3b3 --> b a b a
            vec0 = _mm_unpacklo_ps(vec0, vec_bin_div);     // a0b0a1b1 --> b c a c
            _mm_store_ps(_simd_img_vec, vec0);
            float diff_max_min = _simd_img_vec[2];
            float bin_div = _simd_img_vec[1];
            float acc_diff = _simd_img_vec[0];

            pDif[x] = diff_max_min;
            pBin[x] = acc_diff / bin_div;
        }
    }

	return true;
}


bool CImageUtility::extrSingularBinMap5x5_32f_v3_MMX(IplImage *iplSrcImage, IplImage *iplBin, IplImage *iplMaxMin)
// SIMD accelerated version (use MMX instrinsics only)
// Detect the singular point region (like speckle), binary pattern structure region and the local difference of max-min
// This function is an optimized version of function extrSingularBinMap5x5_32f(), where the singluar number calculation
// part is removed.
// This function supposes the input image iplSrcImage has been padded 2 pixels in each direction!
// Arguments:
//		iplSrcImage -- [I] input image; must be 32F floating point, 1-channel image, 2-pixel padded in boundaries
//     iplBin -- [O] binary pattern measurement, 0 represents the pixel most like binary pattern
//     iplMaxMin -- [O] difference of maximum and minimun value in the window
// by Luhong Liang, IC-ASD, ASTRI
// June 16, 2014
{
	if (iplSrcImage == NULL || iplBin == NULL || iplMaxMin == NULL ||
        iplSrcImage->depth != SR_DEPTH_32F || iplBin->depth != SR_DEPTH_32F || iplMaxMin->depth != SR_DEPTH_32F ||
        iplSrcImage->width != iplBin->width+4 || iplSrcImage->height != iplBin->height+4 ||
        iplSrcImage->width != iplMaxMin->width+4 || iplSrcImage->height != iplMaxMin->height+4) {		// TODO: complete check needed
		showErrMsg("Invalid input/output image in CImageUtility::extrSingularBinMap5x5_32f_v3()!\n");
		return false;
	}

    //empty();        // flush the MMX registers

    const float min_diff_th = 16.0f;
    F32vec4 vec_min_diff_th(min_diff_th, min_diff_th, min_diff_th, min_diff_th);
    __declspec(align(64)) float _simd_img_block[_SR_SIMD_MAX_BLOCK]; 
    __declspec(align(64)) float _simd_img_vec[_SR_SIMD_MAX_VEC]; 

    for (int y=0; y<iplBin->height; y++) {
        float *pSrc0 = (float *)((char *)iplSrcImage->imageData + y * iplSrcImage->widthStep);
        float *pSrc1 = (float *)((char *)iplSrcImage->imageData + (y+1) * iplSrcImage->widthStep);
        float *pSrc2 = (float *)((char *)iplSrcImage->imageData + (y+2) * iplSrcImage->widthStep);
        float *pSrc3 = (float *)((char *)iplSrcImage->imageData + (y+3) * iplSrcImage->widthStep);
        float *pSrc4 = (float *)((char *)iplSrcImage->imageData + (y+4) * iplSrcImage->widthStep);
        float *pBin = (float *)((char *)iplBin->imageData + y * iplBin->widthStep);
        float *pDif = (float *)((char *)iplMaxMin->imageData + y * iplMaxMin->widthStep);
        // set up the initial block data
        _simd_img_block[0]=pSrc0[0];  _simd_img_block[1]=pSrc1[0];  _simd_img_block[2]=pSrc2[0];  _simd_img_block[3]=pSrc3[0];  _simd_img_block[4]=pSrc4[0];  
        _simd_img_block[5]=pSrc0[1];  _simd_img_block[6]=pSrc1[1];  _simd_img_block[7]=pSrc2[1];  _simd_img_block[8]=pSrc3[1];  _simd_img_block[9]=pSrc4[1]; 
        _simd_img_block[10]=pSrc0[2];  _simd_img_block[11]=pSrc1[2];  _simd_img_block[12]=pSrc2[2];  _simd_img_block[13]=pSrc3[2];  _simd_img_block[14]=pSrc4[2]; 
        _simd_img_block[15]=pSrc0[3];  _simd_img_block[16]=pSrc1[3];  _simd_img_block[17]=pSrc2[3];  _simd_img_block[18]=pSrc3[3];  _simd_img_block[19]=pSrc4[3]; 
        //_simd_img_block[20]=pSrc0[4];  _simd_img_block[21]=pSrc1[4];  _simd_img_block[22]=pSrc2[4];  _simd_img_block[23]=pSrc3[4];  _simd_img_block[24]=pSrc4[4]; 
        int update_index = 20;
        for (int x=0; x<iplBin->width; x++) {
            //
            // TODO: process multple blocks to reduce the memory data load/store?
            //

            // update a new row
            _simd_img_block[update_index] = pSrc0[x+4];
            _simd_img_block[update_index+1] = pSrc1[x+4];
            _simd_img_block[update_index+2] = pSrc2[x+4];
            _simd_img_block[update_index+3] = pSrc3[x+4];
            _simd_img_block[update_index+4] = pSrc4[x+4];
            update_index += 5;
            update_index = update_index % 25;
            
            // fill SIMD registers
            F32vec4 vec0 = _mm_load_ps(_simd_img_block); 
            F32vec4 vec1 = _mm_load_ps(_simd_img_block+4); 
            F32vec4 vec2 = _mm_load_ps(_simd_img_block+8); 
            F32vec4 vec3 = _mm_load_ps(_simd_img_block+12); 
            F32vec4 vec4 = _mm_load_ps(_simd_img_block+16); 
            F32vec4 vec5 = _mm_load_ps(_simd_img_block+20); 
            F32vec4 vec6 = _mm_set1_ps(_simd_img_block[24]); 

            // min external 
            F32vec4 vec_min0, vec_min1, vec_min2, vec_max0, vec_max1, vec_max2;
            vec_min0 = _mm_min_ps(vec0, vec1);
            vec_min1 = _mm_min_ps(vec2, vec3);
            vec_min2 = _mm_min_ps(vec4, vec5);
            vec_min1 = _mm_min_ps(vec_min0, vec_min1);
            vec_min0 = _mm_min_ps(vec_min1, vec_min2);
            vec_min0 = _mm_min_ps(vec_min0, vec6);        // last pixel
            // min internal 
            vec_min1 = _mm_movelh_ps(vec_min0, vec_min0);       // 0101: abcd --> abab
            vec_min0 = _mm_min_ps(vec_min0, vec_min1);                 // x x min(a,c), min(b,d)
            vec_min1 = _mm_unpackhi_ps(vec_min0, vec_min0);    //2233: min(a,c), min(a,c), min(b,d), min(b,d)
            vec_min0 = _mm_min_ps(vec_min0, vec_min1);                // x x (min(a,b,c,d), x
            vec_min0 = _mm_unpackhi_ps(vec_min0, vec_min0);   // 2233 --> min, min, x, x
            vec_min0 =_mm_unpacklo_ps(vec_min0, vec_min0);    // 0011-->min, min, min, min

            // max external
            vec_max0 = _mm_max_ps(vec0, vec1);
            vec_max1 = _mm_max_ps(vec2, vec3);
            vec_max2 = _mm_max_ps(vec4, vec5);
            vec_max1 = _mm_max_ps(vec_max0, vec_max1);
            vec_max0 = _mm_max_ps(vec_max1, vec_max2);
            vec_max0 = _mm_max_ps(vec_max0, vec6);        // last pixel
            // max internal
            vec_max1 = _mm_movelh_ps(vec_max0, vec_max0);       // 0101
            vec_max0 = _mm_max_ps(vec_max0, vec_max1);                 // x x max(0,2), max(1,3)
            vec_max1 = _mm_unpackhi_ps(vec_max0, vec_max0);    //2233
            vec_max0 = _mm_max_ps(vec_max0, vec_max1);                // x x (max(0,1,2,3), x
            vec_max0 = _mm_unpackhi_ps(vec_max0, vec_max0);     // 2233 --> max, max, x, x
            vec_max0 =_mm_unpacklo_ps(vec_max0, vec_max0);      // 0011-->max, max, max, max

            // diff
            F32vec4 vec_diff = vec_max0 - vec_min0; // diff, diff, diff, diff

            // binary likelihood
            F32vec4 vec10 = vec_max0 - vec0;     vec0 = vec0 - vec_min0;      vec0 = _mm_min_ps(vec0, vec10);
            F32vec4 vec11 = vec_max0 - vec1;     vec1 = vec1 - vec_min0;      vec1 = _mm_min_ps(vec1, vec11);
            F32vec4 vec12 = vec_max0 - vec2;     vec2 = vec2 - vec_min0;      vec2 = _mm_min_ps(vec2, vec12);
            F32vec4 vec13 = vec_max0 - vec3;     vec3 = vec3 - vec_min0;      vec3 = _mm_min_ps(vec3, vec13);
            F32vec4 vec14 = vec_max0 - vec4;     vec4 = vec4 - vec_min0;      vec4 = _mm_min_ps(vec4, vec14);
            F32vec4 vec15 = vec_max0 - vec5;     vec5 = vec5 - vec_min0;      vec5 = _mm_min_ps(vec5, vec15);
            F32vec4 vec16 = vec_max0 - vec6;     vec6 = vec6 - vec_min0;      vec6 = _mm_min_ps(vec6, vec16);     // last pixel

            vec0 = vec0 + vec1;            vec2 = vec2 + vec3;            vec4 = vec4 + vec5;
            vec2 = vec2 + vec4;            vec0 = vec0 + vec2;  // sub-sum in vec0
            vec0 = _mm_add_ss(vec0, vec6);      // last pixel

            vec1 = _mm_movelh_ps(vec0, vec0);       // 0101: abcd --> abab
            vec0 = vec0 + vec1;                               // x x sum(a,c), sum(b,d)
            vec1 = _mm_unpackhi_ps(vec0, vec0);    //2233: sum(a,c), sum(a,c), sum(b,d), sum(b,d)
            vec0 = vec0 + vec1;                              // x x sum(a,b,c,d), x; vec0 is acc_diff

            // bin div
            F32vec4 vec_bin_div = _mm_max_ps(vec_diff, vec_min_diff_th);

            // vec_diff:   a, a, a, a
            // vec0     :  x, x,  b, x         (acc_diff)
            // bin_div :  c, c,  c, c
            vec0 = _mm_unpackhi_ps(vec_diff, vec0);           // a2b2a3b3 --> a b a x
            vec0 = _mm_unpacklo_ps(vec0, vec_bin_div);     // a0b0a1b1 --> a c b c
            _mm_store_ps(_simd_img_vec, vec0);
            float diff_max_min = _simd_img_vec[0];
            float bin_div = _simd_img_vec[1];
            float acc_diff = _simd_img_vec[2];

            pDif[x] = diff_max_min;
            pBin[x] = acc_diff / bin_div;
        }
    }

	return true;
}

bool CImageUtility::cvtImage8Uto32F_SIMD(IplImage *iplImage8U, IplImage *iplImage32F)
// SIMD accelerated version (use SSE3 instrinsics)
// Convert a 8U image (0~255) to a 32F image (0~255)
// arguments:
//		iplImage8U -- [I] input image, must be 8U data type
//		iplImage32F -- [O] output image, must be 32F data type
// Return:
//		converted image, NULL for failure
// Major Modifications: 
//		Change data range of floating point pixel to 0~255, Nov. 15, 2011
{
	// check input image
	if (iplImage8U == NULL || iplImage8U->depth != SR_DEPTH_8U || 
		iplImage32F == NULL || iplImage32F->depth != SR_DEPTH_32F ||
		iplImage8U->width != iplImage32F->width || iplImage8U->height != iplImage32F->height || 
		iplImage8U->nChannels != iplImage32F->nChannels) {
		showErrMsg("Invalid input image data type in CImageUtility::cvtImage8Uto32F!\n");
		return false;
	}

    //empty();        // flush the MMX registers

	// convert
	for (int y=0; y<iplImage32F->height; y++) {	
		__declspec(align(64)) unsigned char *pSrc = (__declspec(align(64)) unsigned char *)(iplImage8U->imageData) + y * iplImage8U->widthStep;
		__declspec(align(64)) float *pDst = (__declspec(align(64)) float *)((char*)iplImage32F->imageData + y * iplImage32F->widthStep);		// NOTE: widthStep is counted in bytes!
        int x;
		for (x=0; x<iplImage32F->width * iplImage32F->nChannels; x+=16) {
            // load
            //__m128i vec_pi = _mm_loadu_si128((__m128i *)(pSrc+x));        // Loads 128-bit value. Address p not need be 16-byte aligned.
            __m128i vec_pi = _mm_load_si128((__m128i *)(pSrc+x));           // Loads 128-bit value. Address p must be 16-byte aligned.
            // convert 4 bytes to dwords
            __m128i vec_pi1 =_mm_cvtepu8_epi32(vec_pi);
            // convert to floating point
            __m128 vec_p = _mm_cvtepi32_ps(vec_pi1);
            // store
            _mm_store_ps(pDst+x, vec_p);       // address aligned

            // next 4 points
            vec_pi1 = _mm_shuffle_epi32(vec_pi, _MM_SHUFFLE(0, 3, 2, 1));
            // convert 4 bytes to dwords
            vec_pi1 =_mm_cvtepu8_epi32(vec_pi1);
            // convert to floating point
            vec_p = _mm_cvtepi32_ps(vec_pi1);
            // store
            _mm_store_ps(pDst+x+4, vec_p);       // address aligned

            // next 4 points
            vec_pi1 = _mm_shuffle_epi32(vec_pi, _MM_SHUFFLE(1, 0, 3, 2));
            // convert 4 bytes to dwords
            vec_pi1 =_mm_cvtepu8_epi32(vec_pi1);
            // convert to floating point
            vec_p = _mm_cvtepi32_ps(vec_pi1);
            // store
            _mm_store_ps(pDst+x+8, vec_p);       // address aligned

            // next 4 points
            vec_pi1 = _mm_shuffle_epi32(vec_pi, _MM_SHUFFLE(2, 1, 0, 3));
            // convert 4 bytes to dwords
            vec_pi1 =_mm_cvtepu8_epi32(vec_pi1);
            // convert to floating point
            vec_p = _mm_cvtepi32_ps(vec_pi1);
            // store
            _mm_store_ps(pDst+x+12, vec_p);       // address aligned
		}
		for (0; x<iplImage32F->width * iplImage32F->nChannels; x++) {
			pDst[x] = (float)pSrc[x];		// / 255.0f;
		}
	}

	return true;
}

bool CImageUtility::cvtImage8Uto32F_SIMD(unsigned char *pImage8U, IplImage *iplImage32F)
 // SIMD accelerated version (use SSE3 instrinsics)
// Convert a 8U image (0~255) to a 32F image (0~255)
// arguments:
//		pImage8U -- [I] buffer of input image, must be word-aligned (32-bit) and with appropriate size
//		iplImage32F -- [O] output image, must be 32F data type
// Return:
//		converted image, NULL for failure
{
	// check input image
	if (pImage8U == NULL ||	iplImage32F == NULL || iplImage32F->depth != SR_DEPTH_32F) {
		showErrMsg("Invalid input image data type in CImageUtility::cvtImage8Uto32F!\n");
		return false;
	}

    //empty();        // flush the MMX registers

	// convert
    int widthStep = ((iplImage32F->width * iplImage32F->nChannels + 3) / 4) * 4;     // 32-bit aligned
	for (int y=0; y<iplImage32F->height; y++) {	
		__declspec(align(64)) float *pDst = (__declspec(align(64)) float *)((char*)iplImage32F->imageData + y * iplImage32F->widthStep);		// NOTE: widthStep is counted in bytes!
        int x;
		for (x=0; x<iplImage32F->width * iplImage32F->nChannels; x+=16) {
            __m128i vec_pi = _mm_loadu_si128((__m128i *)(pImage8U+x));        // Loads 128-bit value. Address p not need be 16-byte aligned.

            // convert 4 bytes to dwords
            __m128i vec_pi1 =_mm_cvtepu8_epi32(vec_pi);
            // convert to floating point
            __m128 vec_p = _mm_cvtepi32_ps(vec_pi1);
            // store
            _mm_store_ps(pDst+x, vec_p);       // address aligned

            // next 4 points
            vec_pi1 = _mm_shuffle_epi32(vec_pi, _MM_SHUFFLE(0, 3, 2, 1));
            // convert 4 bytes to dwords
            vec_pi1 =_mm_cvtepu8_epi32(vec_pi1);
            // convert to floating point
            vec_p = _mm_cvtepi32_ps(vec_pi1);
            // store
            _mm_store_ps(pDst+x+4, vec_p);       // address aligned

            // next 4 points
            vec_pi1 = _mm_shuffle_epi32(vec_pi, _MM_SHUFFLE(1, 0, 3, 2));
            // convert 4 bytes to dwords
            vec_pi1 =_mm_cvtepu8_epi32(vec_pi1);
            // convert to floating point
            vec_p = _mm_cvtepi32_ps(vec_pi1);
            // store
            _mm_store_ps(pDst+x+8, vec_p);       // address aligned

            // next 4 points
            vec_pi1 = _mm_shuffle_epi32(vec_pi, _MM_SHUFFLE(2, 1, 0, 3));
            // convert 4 bytes to dwords
            vec_pi1 =_mm_cvtepu8_epi32(vec_pi1);
            // convert to floating point
            vec_p = _mm_cvtepi32_ps(vec_pi1);
            // store
            _mm_store_ps(pDst+x+12, vec_p);       // address aligned
		}
		for (; x<iplImage32F->width * iplImage32F->nChannels; x++) {
			pDst[x] = (float)pImage8U[x];		// / 255.0f;
		}
        pImage8U += widthStep;
	}

	return true;
}

bool CImageUtility::cvtImage32Fto8U_SIMD(IplImage *iplImage32F, IplImage *iplImage8U, short bias, float mag_factor, int cvt_method)
// SIMD accelerated version (use SSE3 instrinsics)
// Convert a 32F image (0~255) to a 8U image (0~255)
// out = Cvt(in * mag_factor + bias)
// arguments:
//		iplImage32F -- [I] input image, must be 32F data type
//		iplImage8U -- [O] input image, must be 8U data type
//		bias -- [I] a bias added to the pixel value, ranging 0~255
//		mag_factor -- [I] magnitude factor of the intensitiy, the final pixel value is (org_pixel_value * mag_factor) + bias
//      cvt_method -- [I] method to covert bitdepth (only support truncation and rounding)
// Return:
//		converted image, NULL for failure
// Major Modifications: 
//		Change data range of floating point pixel to 0~255, Nov. 15, 2011
//      Add option for covnersion method, Aug. 25, 2013
{
	// check input image
	if (iplImage32F == NULL || iplImage32F->depth != SR_DEPTH_32F || iplImage8U == NULL || iplImage8U->depth != SR_DEPTH_8U ||
		iplImage32F->width != iplImage8U->width || iplImage32F->height != iplImage8U->height || iplImage32F->nChannels != iplImage8U->nChannels) {
		showErrMsg("Invalid or unmatched input/output image format in CImageUtility::cvtImage32Fto8U()!\n");
		return false;
	}

    //empty();        // flush the MMX registers

	// convert
    //__declspec(align(64)) float _aligned_buf[4];
    __m128 vec_factor = _mm_set1_ps(mag_factor);        // donot use "static", otherwise the instruction will fail!!!
    __m128 vec_bias = _mm_set1_ps(bias);
    static const __m128 vec_const0 = _mm_set1_ps(0.0f);
    static const __m128 vec_const255 = _mm_set1_ps((float)IMAGE_DYNAMIC_RANGE_8U);
    if ((cvt_method & SR_DEPTHCVT_TRUNC) != 0) {
        // truncation
	    for (int y=0; y<iplImage8U->height; y++) {	
		    __declspec(align(64)) float *pSrc = (__declspec(align(64)) float *)(iplImage32F->imageData + y * iplImage32F->widthStep);		// NOTE: widthStep is counted in bytes!
		    __declspec(align(64)) unsigned char *pDst = (__declspec(align(64)) unsigned char *)(iplImage8U->imageData) + y * iplImage8U->widthStep;
            int x;
		    for (x=0; x<iplImage8U->width * iplImage8U->nChannels; x+=4) {      // TODO: convert 16 pixels in each round
                F32vec4 vec_p = _mm_load_ps(pSrc+x);
			    //int pix = (int)(pSrc[x] * mag_factor + bias);
                vec_p = _mm_mul_ps(vec_p, vec_factor);
                vec_p = _mm_add_ps(vec_p, vec_bias);
                // clipping (clipping first to avoid extreme value due to large magnification factor)
                vec_p = _mm_max_ps(vec_p, vec_const0);
                vec_p = _mm_min_ps(vec_p, vec_const255);
			    // convert to integer
                __m128i vec_pi = _mm_cvttps_epi32(vec_p);   // truncation
                // Packs the 8 signed 32-bit integers from a and b into signed 16-bit integers and saturates
                vec_pi = _mm_packs_epi32(vec_pi, vec_pi);
                // Packs the 16 signed 16-bit integers from a and b into 8-bit unsigned integers and saturates.
                vec_pi = _mm_packus_epi16(vec_pi, vec_pi);
                // extract the 32 bit
                unsigned int dst = _mm_extract_epi32(vec_pi, 0);
                unsigned int *pBuf = (unsigned int *)(pDst + x);
                *pBuf = dst;
                //_mm_store_ps(_aligned_buf, vec_p);
            }
            // remaining
		    for (; x<iplImage8U->width * iplImage8U->nChannels; x++) {
			    int pix = (int)(pSrc[x] * mag_factor + bias);
                // clipping
                pDst[x] = (unsigned char)clip_0_255(pix);
            }
	    }
    } else if ((cvt_method & SR_DEPTHCVT_ROUND) != 0) {
        // rounding
	    for (int y=0; y<iplImage8U->height; y++) {	
		    __declspec(align(64)) float *pSrc = (__declspec(align(64)) float *)(iplImage32F->imageData + y * iplImage32F->widthStep);		// NOTE: widthStep is counted in bytes!
		    __declspec(align(64)) unsigned char *pDst = (__declspec(align(64)) unsigned char *)(iplImage8U->imageData) + y * iplImage8U->widthStep;
            int x;
		    for (x=0; x<iplImage8U->width * iplImage8U->nChannels; x+=4) {      // TODO: convert 16 pixels in each round
                F32vec4 vec_p = _mm_load_ps(pSrc+x);
			    //int pix = (int)(pSrc[x] * mag_factor + bias + 0.5f);
                vec_p = _mm_mul_ps(vec_p, vec_factor);
                vec_p = _mm_add_ps(vec_p, vec_bias);
                // clipping (clipping first to avoid extreme value due to large magnification factor)
                vec_p = _mm_max_ps(vec_p, vec_const0);
                vec_p = _mm_min_ps(vec_p, vec_const255);
			    // convert to integer
                __m128i vec_pi = _mm_cvtps_epi32(vec_p);   // rounding?
                // Packs the 8 signed 32-bit integers from a and b into signed 16-bit integers and saturates
                vec_pi = _mm_packs_epi32(vec_pi, vec_pi);
                // Packs the 16 signed 16-bit integers from a and b into 8-bit unsigned integers and saturates.
                vec_pi = _mm_packus_epi16(vec_pi, vec_pi);
                // extract the 32 bit
                unsigned int dst = _mm_extract_epi32(vec_pi, 0);
                unsigned int *pBuf = (unsigned int *)(pDst + x);
                *pBuf = dst;
                //_mm_store_ps(_aligned_buf, vec_p);
            }
            // remaining
		    for (; x<iplImage8U->width * iplImage8U->nChannels; x++) {
			    int pix = (int)(pSrc[x] * mag_factor + bias + 0.5f);
			    // clipping
                pDst[x] = (unsigned char)clip_0_255(pix);
            }
	    }
    } else {
        showErrMsg("Not implemented or unknown bit depth converion method in CImageUtility::cvtImage32Fto8U()!\n");
        return false;
    }

	return true;
}

bool CImageUtility::cvtImage32Fto8U_SIMD(IplImage *iplImage32F, unsigned char *pImage8U, short bias, float mag_factor, int cvt_method)
    // SIMD accelerated version (use SSE3 instrinsics)
// Convert a 32F image (0~255) to a 8U image (0~255)
// out = Cvt(in * mag_factor + bias)
// NOTE: The difference to another function cvtImage32Fto8U_SIMD() is that this function suppose the target buffer is 4-byte aligned!!!!!!
// arguments:
//		iplImage32F -- [I] input image, must be 32F data type
//		pImage8U -- [O] input image, must be word-aligned (32-bit) and with appropriate size
//		bias -- [I] a bias added to the pixel value, ranging 0~255
//		mag_factor -- [I] magnitude factor of the intensitiy, the final pixel value is (org_pixel_value * mag_factor) + bias
//      cvt_method -- [I] method to covert bitdepth (only support truncation and rounding)
// Return:
//		converted image, NULL for failure
{
	// check input image
	if (iplImage32F == NULL || iplImage32F->depth != SR_DEPTH_32F || pImage8U == NULL) {
		showErrMsg("Invalid or unmatched input/output image format in CImageUtility::cvtImage32Fto8U()!\n");
		return false;
	}

    //empty();        // flush the MMX registers

    int widthStep = ((iplImage32F->width * iplImage32F->nChannels + 3) / 4) * 4;     // 32-bit aligned

	// convert
    //__declspec(align(64)) float _aligned_buf[4];
    static const __m128 vec_const0 = _mm_set1_ps(0.0f);
    //static const __m128i vec_consti0 = _mm_set1_epi32(0);
    static const __m128 vec_const255 = _mm_set1_ps((float)IMAGE_DYNAMIC_RANGE_8U);
    //static const __m128i vec_consti255 = _mm_set1_epi32(IMAGE_DYNAMIC_RANGE_8U);
    static const __m128i vec_cvt = _mm_set_epi32(256*256*256, 256*256, 256, 1);
    __m128 vec_factor = _mm_set1_ps(mag_factor);
    __m128 vec_bias = _mm_set1_ps(bias);
    if ((cvt_method & SR_DEPTHCVT_TRUNC) != 0) {
        // truncation
	    for (int y=0; y<iplImage32F->height; y++) {	
		    __declspec(align(64)) float *pSrc = (__declspec(align(64)) float *)(iplImage32F->imageData + y * iplImage32F->widthStep);		// NOTE: widthStep is counted in bytes!
		    unsigned int *pDst = (unsigned int *)(pImage8U + y * widthStep);
            int x;
		    for (x=0; x<iplImage32F->width * iplImage32F->nChannels; x+=4) {        // TODO: convert 16 pixels in each round (need 128-bit alignement)
                F32vec4 vec_p = _mm_load_ps(pSrc+x);
			    //int pix = (int)(pSrc[x] * mag_factor + bias);
                vec_p = _mm_mul_ps(vec_p, vec_factor);
                vec_p = _mm_add_ps(vec_p, vec_bias);
                // clipping (clipping first to avoid extreme value due to large magnification factor)
                vec_p = _mm_max_ps(vec_p, vec_const0);
                vec_p = _mm_min_ps(vec_p, vec_const255);
			    // convert to integer
                __m128i vec_pi = _mm_cvttps_epi32(vec_p);   // truncation
                // Packs the 8 signed 32-bit integers from a and b into signed 16-bit integers and saturates
                vec_pi = _mm_packs_epi32(vec_pi, vec_pi);
                // Packs the 16 signed 16-bit integers from a and b into 8-bit unsigned integers and saturates.
                vec_pi = _mm_packus_epi16(vec_pi, vec_pi);
                // extract the 32 bit
                unsigned int dst = _mm_extract_epi32(vec_pi, 0);
                unsigned int *pBuf = (unsigned int *)(pDst + x);
                *pBuf = dst;
                //_mm_store_ps(_aligned_buf, vec_p);
            }
            // remaining
		    for (; x<iplImage32F->width * iplImage32F->nChannels; x++) {
			    int pix = (int)(pSrc[x] * mag_factor + bias);
                // clipping
                pix = pix < 0 ? 0 : pix;
                pix = pix > IMAGE_DYNAMIC_RANGE_8U ? IMAGE_DYNAMIC_RANGE_8U : pix;
                pDst[x] = pix;
            }
	    }
    } else if ((cvt_method & SR_DEPTHCVT_ROUND) != 0) {
        // rounding
	    for (int y=0; y<iplImage32F->height; y++) {	
		    __declspec(align(64)) float *pSrc = (__declspec(align(64)) float *)(iplImage32F->imageData + y * iplImage32F->widthStep);		// NOTE: widthStep is counted in bytes!
		    unsigned char *pDst = (unsigned char *)(pImage8U + y * widthStep);
            int x;
		    for (x=0; x<iplImage32F->width * iplImage32F->nChannels; x+=4) {        // TODO: convert 16 pixels in each round (nee 128-bit alignement)
                F32vec4 vec_p = _mm_load_ps(pSrc+x);
			    //int pix = (int)(pSrc[x] * mag_factor + bias + 0.5f);
                vec_p = _mm_mul_ps(vec_p, vec_factor);
                vec_p = _mm_add_ps(vec_p, vec_bias);
                // clipping (clipping first to avoid extreme value due to large magnification factor)
                vec_p = _mm_max_ps(vec_p, vec_const0);
                vec_p = _mm_min_ps(vec_p, vec_const255);
			    // convert to integer
                __m128i vec_pi = _mm_cvtps_epi32(vec_p);   // rounding?
                // Packs the 8 signed 32-bit integers from a and b into signed 16-bit integers and saturates
                vec_pi = _mm_packs_epi32(vec_pi, vec_pi);
                // Packs the 16 signed 16-bit integers from a and b into 8-bit unsigned integers and saturates.
                vec_pi = _mm_packus_epi16(vec_pi, vec_pi);
                // extract the 32 bit
                unsigned int dst = _mm_extract_epi32(vec_pi, 0);
                unsigned int *pBuf = (unsigned int *)(pDst + x);
                *pBuf = dst;
                //_mm_store_ps(_aligned_buf, vec_p);
            }
            // remaining
		    for (; x<iplImage32F->width * iplImage32F->nChannels; x++) {
			    int pix = (int)(pSrc[x] * mag_factor + bias + 0.5f);
			    // clipping
                pDst[x] = (unsigned char)clip_0_255(pix);
            }
	    }
    } else {
        showErrMsg("Not implemented or unknown bit depth converion method in CImageUtility::cvtImage32Fto8U()!\n");
        return false;
    }

	return true;
}

bool CImageUtility::min3x3_o1_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage)
// SIMD accelerated version (use SSE3 instrinsics)
// 3x3 min filter
// compared with min3x3_32f, this function suppose the input image has been padded 1 pixel in each direction
{
	if (iplSrcImage == NULL || iplDstImage == NULL ||
		iplSrcImage->nChannels != 1 || iplDstImage->nChannels != 1 || iplSrcImage->depth != SR_DEPTH_32F || iplDstImage->depth != SR_DEPTH_32F ||
        iplSrcImage->width != iplDstImage->width+2 || iplSrcImage->height != iplDstImage->height+2)  {
		showErrMsg("Invalid input argument in CImageUtility::min3x3_o1_32f()!\n");
		return false;
	}
    
    //empty();        // flush the MMX registers

	// line buffer
    const int align_floats = _SIMD_ALIGN_BYTES / sizeof(float);
    int aligned_size_line = get_aligned_size(iplSrcImage->width, align_floats);
	__declspec(align(64)) float *pLineBuf = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
	if (pLineBuf == NULL) {
		showErrMsg("Fail to allocate buffer in CImageUtility::min3x3_o1_32f()!\n");
		return false;
	}

	// filtering
    //int most_right = (aligned_size_line - 4) < iplDstImage->width ? (aligned_size_line - 8) : iplDstImage->width;
    int most_right = (aligned_size_line - 4) < iplDstImage->width ? (aligned_size_line - 4) : iplDstImage->width;
	for (int y=0; y<iplDstImage->height; y++) {
		// convolution in Y direction
		__declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)((char*)iplSrcImage->imageData + y * iplSrcImage->widthStep);
		__declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)((char*)iplSrcImage->imageData + (y+1) * iplSrcImage->widthStep);
		__declspec(align(64)) float *pSrc2 = (__declspec(align(64)) float *)((char*)iplSrcImage->imageData + (y+2) * iplSrcImage->widthStep);
        // calculate in Y direction
        int x;
		for (x=0; x<iplSrcImage->width; x+=4) {
            F32vec4 vec_p0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_p1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_p2 = _mm_load_ps(pSrc2+x);
            vec_p0 = _mm_min_ps(vec_p0, vec_p1);
            vec_p0 = _mm_min_ps(vec_p0, vec_p2);
			_mm_store_ps(pLineBuf+x, vec_p0);
		}
		for (; x<iplSrcImage->width; x++) {  // remaining
            float minv = pSrc0[x] < pSrc1[x] ? pSrc0[x] : pSrc1[x];
            minv = minv < pSrc2[x] ? minv : pSrc2[x];
			pLineBuf[x] = minv;
		}
		// calculate in X direction
		__declspec(align(64)) float *pDst = (__declspec(align(64)) float *)((char*)iplDstImage->imageData + y * iplDstImage->widthStep);
        F32vec4 vec_p0 = _mm_load_ps(pLineBuf);
		for (x=0; x<most_right; x+=4) {
            int xx = x + 4;
            F32vec4 vec_p1 = _mm_load_ps(pLineBuf+xx);
            //                       gx0     |      gx1
            //                  0  1  2  3  |  4  5  6  7
            //  to get:      2  3  4  5 using shuffle(gx0, gx1, [1 0 3 2])
            F32vec4 vec_p2 = _mm_shuffle_ps(vec_p0, vec_p1, _MM_SHUFFLE(1, 0, 3, 2));             // 2 3 4 5
            //  to get:      1  2  3  4  using gx3=shuffle(gx0, gx2, [2, 1, 2, 1]
            F32vec4 vec_p3 = _mm_shuffle_ps(vec_p0, vec_p2, _MM_SHUFFLE(2, 1, 2, 1));             // 1 2 3 4
            // min(p0, p2, p3)
            vec_p2 = _mm_min_ps(vec_p2, vec_p0);
            vec_p2 = _mm_min_ps(vec_p2, vec_p3);
            // swap
            vec_p0 = vec_p1;
            // store
            _mm_store_ps(pDst+x, vec_p2);
		}
		for (; x<iplDstImage->width; x++) {     // remaining
            float minv = pLineBuf[x] < pLineBuf[x+1] ? pLineBuf[x] : pLineBuf[x+1];
            minv = minv < pLineBuf[x+2] ? minv : pLineBuf[x+2];
			pDst[x] = minv;
		}
	}

	_aligned_free(pLineBuf);

	return true;
}

bool CImageUtility::max3x3_o1_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage)
 // SIMD accelerated version (use SSE3 instrinsics)
// 3x3 max filter
// different from max3x3_32f, this function supposes the input image has been padded 1 pixel in each direction
{
	if (iplSrcImage == NULL || iplDstImage == NULL ||
		iplSrcImage->nChannels != 1 || iplDstImage->nChannels != 1 || iplSrcImage->depth != SR_DEPTH_32F || iplDstImage->depth != SR_DEPTH_32F ||
        iplSrcImage->width != iplDstImage->width+2 || iplSrcImage->height != iplDstImage->height+2)  {
		showErrMsg("Invalid input argument in CImageUtility::max3x3_o1_32f()!\n");
		return false;
	}

	// line buffer
    const int align_floats = _SIMD_ALIGN_BYTES / sizeof(float);
    int aligned_size_line = get_aligned_size(iplSrcImage->width, align_floats);
	__declspec(align(64)) float *pLineBuf = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
	if (pLineBuf == NULL) {
		showErrMsg("Fail to allocate buffer in CImageUtility::max3x3_32f()!\n");
		return false;
	}

    //empty();        // flush the MMX registers

	// filtering
    //int most_right = (aligned_size_line - 4) < iplDstImage->width ? (aligned_size_line - 8) : iplDstImage->width;
    int most_right = (aligned_size_line - 4) < iplDstImage->width ? (aligned_size_line - 4) : iplDstImage->width;
	for (int y=0; y<iplDstImage->height; y++) {
		// convolution in Y direction
		__declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)((char*)iplSrcImage->imageData + y * iplSrcImage->widthStep);
		__declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)((char*)iplSrcImage->imageData + (y+1) * iplSrcImage->widthStep);
		__declspec(align(64)) float *pSrc2 = (__declspec(align(64)) float *)((char*)iplSrcImage->imageData + (y+2) * iplSrcImage->widthStep);
        // calculate in Y direction
        int x;
		for (x=0; x<iplSrcImage->width; x+=4) {
            F32vec4 vec_p0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_p1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_p2 = _mm_load_ps(pSrc2+x);
            vec_p0 = _mm_max_ps(vec_p0, vec_p1);
            vec_p0 = _mm_max_ps(vec_p0, vec_p2);
			_mm_store_ps(pLineBuf+x, vec_p0);
		}
		for (; x<iplSrcImage->width; x++) {  // remaining
            float maxv = pSrc0[x] > pSrc1[x] ? pSrc0[x] : pSrc1[x];
            maxv = maxv > pSrc2[x] ? maxv : pSrc2[x];
			pLineBuf[x] = maxv;
		}
		// calculate in X direction
		__declspec(align(64)) float *pDst = (__declspec(align(64)) float *)((char*)iplDstImage->imageData + y * iplDstImage->widthStep);
        F32vec4 vec_p0 = _mm_load_ps(pLineBuf);
		for (x=0; x<most_right; x+=4) {
            int xx = x + 4;
            F32vec4 vec_p1 = _mm_load_ps(pLineBuf+xx);
            //                       gx0     |      gx1
            //                  0  1  2  3  |  4  5  6  7
            //  to get:      2  3  4  5 using shuffle(gx0, gx1, [1 0 3 2])
            F32vec4 vec_p2 = _mm_shuffle_ps(vec_p0, vec_p1, _MM_SHUFFLE(1, 0, 3, 2));             // 2 3 4 5
            //  to get:      1  2  3  4  using gx3=shuffle(gx0, gx2, [2, 1, 2, 1]
            F32vec4 vec_p3 = _mm_shuffle_ps(vec_p0, vec_p2, _MM_SHUFFLE(2, 1, 2, 1));             // 1 2 3 4
            // max(p0, p2, p3)
            vec_p2 = _mm_max_ps(vec_p2, vec_p0);
            vec_p2 = _mm_max_ps(vec_p2, vec_p3);
            // swap
            vec_p0 = vec_p1;
            // store
            _mm_store_ps(pDst+x, vec_p2);
		}
		for (; x<iplDstImage->width; x++) {     // remaining
            float maxv = pLineBuf[x] > pLineBuf[x+1] ? pLineBuf[x] : pLineBuf[x+1];
            maxv = maxv > pLineBuf[x+2] ? maxv : pLineBuf[x+2];
			pDst[x] = maxv;
		}
	}

	_aligned_free(pLineBuf);

	return true;
}

bool CImageUtility::sobel_o1_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage, int direction)
// SIMD accelerated version (use SSE3 instrinsics)
// Sobel operator. We do not use OpenCV build-in function because it uses a Gaussian filter before gradient calculation
// This function supports "in-place" operation
// Arguments:
//		iplSrcImage -- [I] input image; must be 32F floating point, 1-channel image, should be padded 1 pixels in each direction
//		iplDstImage -- [O] input image; must be 32F floating point, 1-channel image
//		direction -- [I] direction: 0 -- horizontal;  1 -- vertical;  2 -- modulation (2-norm);    3 -- ABS (1-norm)
{
	if (iplSrcImage == NULL || iplDstImage == NULL ||
		iplSrcImage->nChannels != 1 || iplDstImage->nChannels != 1 || iplSrcImage->depth != SR_DEPTH_32F || iplDstImage->depth != SR_DEPTH_32F ||
        iplSrcImage->width != iplDstImage->width+2 || iplSrcImage->height != iplDstImage->height+2)  {
		showErrMsg("Invalid input/output image in CImageUtility::sobel_o1_32f_SIMD()!\n");
		return false;
	}

    //empty();        // flush the MMX registers

    // allocate aligned buffers
    const int align_floats = _SIMD_ALIGN_BYTES / sizeof(float);
    int aligned_size_line = get_aligned_size(iplSrcImage->width, align_floats);
    __declspec(align(64)) float *pGxxLine = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
    __declspec(align(64)) float *pGyyLine = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
	if (pGxxLine == NULL || pGyyLine == NULL)  {
        _aligned_free(pGxxLine);
        _aligned_free(pGyyLine);
		showErrMsg("Fail to allocate image buffer in CImageUtility::sobel_o1_32f_SIMD()!\n");
		return false;
	}

	// calculate gradient
    static const __m128 vec_const0 = _mm_set1_ps(0.0f);
    static const __m128 vec_consto4 = _mm_set1_ps(1.0f/4.0f);
    static const __m128 vec_consto8 = _mm_set1_ps(1.0f/8.0f);
    static const __m128 vec_consto16 = _mm_set1_ps(1.0f/16.0f);
    static const __m128 SIGNMASK = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)); 
    //int most_right = (aligned_size_line - 4) < (iplSrcImage->width-2) ? (aligned_size_line - 8) : (iplSrcImage->width-2);
    int most_right = (aligned_size_line - 4) < (iplSrcImage->width-2) ? (aligned_size_line - 4) : (iplSrcImage->width-2);

	if (direction == 0) {
        for (int y=0; y<iplDstImage->height; y++) {
            __declspec(align(64)) float *pSrcN = (__declspec(align(64)) float *)(iplSrcImage->imageData + y * iplSrcImage->widthStep);        // each line of the image has been aligned
            __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+1) * iplSrcImage->widthStep);
            __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+2) * iplSrcImage->widthStep);
            __declspec(align(64)) float *pDst = (__declspec(align(64)) float *)(iplDstImage->imageData + y * iplDstImage->widthStep);
            // pre-calculate in Y direction
            int x;
            for (x=0; x<iplSrcImage->width; x+=4) {
                //pGxxLine[x] = pSrcN[x]  + pSrc0[x]  + pSrc0[x] + pSrc1[x];      // 1 2 1 in column
                F32vec4 vec_p0 = _mm_load_ps(pSrcN+x);
                F32vec4 vec_p1 = _mm_load_ps(pSrc0+x);
                F32vec4 vec_p2 = _mm_load_ps(pSrc1+x);
                F32vec4 vec_grad_x = _mm_add_ps(vec_p0, vec_p1);
                vec_grad_x = _mm_add_ps(vec_grad_x, vec_p1);
                vec_grad_x = _mm_add_ps(vec_grad_x, vec_p2);
                // write back
                _mm_store_ps(pGxxLine+x, vec_grad_x);
            }
            for (; x<iplSrcImage->width; x++) {        // remaining pixels
                // -1 0 1
                // -2 0 2
                // -1 0 1
                pGxxLine[x] = pSrcN[x]  + pSrc0[x]  + pSrc0[x] + pSrc1[x];      // 1 2 1 in column
            }

            // calculate the filter
            // pre load 4 elements
            F32vec4 vec_gx0 = _mm_load_ps(pGxxLine);
            for (x=0; x<most_right; x+=4) {
                int xx = x + 4;
                //float grad_x = - pGxxLine[x] + pGxxLine[x+2];
                F32vec4 vec_gx1 = _mm_load_ps(pGxxLine+xx);
                // Gx:
                //                       gx0     |      gx1
                //                  0  1  2  3  |  4  5  6  7
                //  to get:      2  3  4  5 using shuffle(gx0, gx1, [1 0 3 2])
                F32vec4 vec_gx2 = _mm_shuffle_ps(vec_gx0, vec_gx1, _MM_SHUFFLE(1, 0, 3, 2));
                vec_gx2 = _mm_sub_ps(vec_gx2, vec_gx0);
                //  * 0.25f;
                vec_gx2 = _mm_mul_ps(vec_gx2, vec_consto4);
                // swap registers
                vec_gx0 = vec_gx1;

                // copy data
                _mm_store_ps(pDst+x, vec_gx2);
		    }
            for (; x<iplDstImage->width; x++) {    // remaining pixels
                // -1 0 1
                // -2 0 2
                // -1 0 1
                float grad_x = - pGxxLine[x] + pGxxLine[x+2];

                // Gm
                pDst[x] = grad_x * 0.25f;
		    }
        }
	} else if (direction == 1) {
        for (int y=0; y<iplDstImage->height; y++) {
            __declspec(align(64)) float *pSrcN = (__declspec(align(64)) float *)(iplSrcImage->imageData + y * iplSrcImage->widthStep);        // each line of the image has been aligned
            __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+1) * iplSrcImage->widthStep);
            __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+2) * iplSrcImage->widthStep);
            __declspec(align(64)) float *pDst = (__declspec(align(64)) float *)(iplDstImage->imageData + y * iplDstImage->widthStep);
            // pre-calculate in Y direction
            int x;
            for (x=0; x<iplSrcImage->width; x+=4) {
                F32vec4 vec_p0 = _mm_load_ps(pSrcN+x);
                F32vec4 vec_p1 = _mm_load_ps(pSrc0+x);
                F32vec4 vec_p2 = _mm_load_ps(pSrc1+x);
			    //pGyyLine[x] = pSrc1[x] - pSrcN[x];                                          // -1 0 1 in column
                F32vec4 vec_grad_y = _mm_sub_ps(vec_p2, vec_p0);
                // write back
                _mm_store_ps(pGyyLine+x, vec_grad_y);
            }
            for (; x<iplSrcImage->width; x++) {        // remaining pixels
                // -1  -2  -1
                //  0   0   0
                //  1   2   1
			    pGyyLine[x] = pSrc1[x] - pSrcN[x];                                          // -1 0 1 in column
            }

            // calculate the filter
            // pre load 4 elements
            F32vec4 vec_gy0 = _mm_load_ps(pGyyLine);
            for (x=0; x<most_right; x+=4) {
                int xx = x + 4;
                //float grad_x = - pGxxLine[x] + pGxxLine[x+2];
                F32vec4 vec_gy1 = _mm_load_ps(pGyyLine+xx);
                //                       gx0     |      gx1
                //                  0  1  2  3  |  4  5  6  7
                //  to get:      2  3  4  5 using shuffle(gx0, gx1, [1 0 3 2])
                F32vec4 vec_gy2 = _mm_shuffle_ps(vec_gy0, vec_gy1, _MM_SHUFFLE(1, 0, 3, 2));       // 2 3 4 5
                //  to get:      1  2  3  4  using gx3=shuffle(gx0, gx2, [2, 1, 2, 1]
                F32vec4 vec_gy3 = _mm_shuffle_ps(vec_gy0, vec_gy2, _MM_SHUFFLE(2, 1, 2, 1));        // 1 2 3 4

                // calculate the final filter response
                vec_gy2 = _mm_add_ps(vec_gy2, vec_gy0);     // g0 + g3 + g3 + g2
                vec_gy2 = _mm_add_ps(vec_gy2, vec_gy3);
                vec_gy2 = _mm_add_ps(vec_gy2, vec_gy3);
                //  * 0.25f;
                vec_gy2 = _mm_mul_ps(vec_gy2, vec_consto4);
         
                // swap registers
                vec_gy0 = vec_gy1;

                // copy data
                _mm_store_ps(pDst+x, vec_gy2);
		    }
            for (; x<iplDstImage->width; x++) {    // remaining pixels
                // -1 0 1
                // -2 0 2
                // -1 0 1
                float grad_x = - pGxxLine[x] + pGxxLine[x+2];
                // -1 -2  -1
                //  0   0   0
                //  1   2   1
			    float grad_y = pGyyLine[x] + pGyyLine[x+1] + pGyyLine[x+1] + pGyyLine[x+2];
                // Gm
                pDst[x] = sqrt(grad_x*grad_x + grad_y*grad_y) * 0.25f;
		    }
        }
	} else if (direction == 2) {
        for (int y=0; y<iplDstImage->height; y++) {
            __declspec(align(64)) float *pSrcN = (__declspec(align(64)) float *)(iplSrcImage->imageData + y * iplSrcImage->widthStep);        // each line of the image has been aligned
            __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+1) * iplSrcImage->widthStep);
            __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+2) * iplSrcImage->widthStep);
            __declspec(align(64)) float *pDst = (__declspec(align(64)) float *)(iplDstImage->imageData + y * iplDstImage->widthStep);
            // pre-calculate in Y direction
            int x;
            for (x=0; x<iplSrcImage->width; x+=4) {
                //pGxxLine[x] = pSrcN[x]  + pSrc0[x]  + pSrc0[x] + pSrc1[x];      // 1 2 1 in column
                F32vec4 vec_p0 = _mm_load_ps(pSrcN+x);
                F32vec4 vec_p1 = _mm_load_ps(pSrc0+x);
                F32vec4 vec_p2 = _mm_load_ps(pSrc1+x);
                F32vec4 vec_grad_x = _mm_add_ps(vec_p0, vec_p1);
                vec_grad_x = _mm_add_ps(vec_grad_x, vec_p1);
                vec_grad_x = _mm_add_ps(vec_grad_x, vec_p2);
			    //pGyyLine[x] = pSrc1[x] - pSrcN[x];                                          // -1 0 1 in column
                F32vec4 vec_grad_y = _mm_sub_ps(vec_p2, vec_p0);
                // write back
                _mm_store_ps(pGxxLine+x, vec_grad_x);
                _mm_store_ps(pGyyLine+x, vec_grad_y);
            }
            for (; x<iplSrcImage->width; x++) {        // remaining pixels
                // -1 0 1
                // -2 0 2
                // -1 0 1
                pGxxLine[x] = pSrcN[x]  + pSrc0[x]  + pSrc0[x] + pSrc1[x];      // 1 2 1 in column
                // -1  -2  -1
                //  0   0   0
                //  1   2   1
			    pGyyLine[x] = pSrc1[x] - pSrcN[x];                                          // -1 0 1 in column
            }

            // calculate the filter
            // pre load 4 elements
            F32vec4 vec_gx0 = _mm_load_ps(pGxxLine);
            F32vec4 vec_gy0 = _mm_load_ps(pGyyLine);
            for (x=0; x<most_right; x+=4) {     // use most_right to SIMD as more as possible
                int xx = x + 4;
                //float grad_x = - pGxxLine[x] + pGxxLine[x+2];
                F32vec4 vec_gx1 = _mm_load_ps(pGxxLine+xx);
                F32vec4 vec_gy1 = _mm_load_ps(pGyyLine+xx);
                // Gx:
                //                       gx0     |      gx1
                //                  0  1  2  3  |  4  5  6  7
                //  to get:      2  3  4  5 using shuffle(gx0, gx1, [1 0 3 2])
                F32vec4 vec_gx2 = _mm_shuffle_ps(vec_gx0, vec_gx1, _MM_SHUFFLE(1, 0, 3, 2));
                // ---------------- Usage of _MM_SHUFFLE -------------------
                // vec0.m_128_f32[4] = abcd;  vec0.m_128_f32[4]=efgh
                // vec2 = _mm_shuffle_ps(vec0, vec1, _MM_SHUFFLE(i3, i2, i1, i0))
                // Then:
                // vec2.m_128_f32[4] = vec0(i0) vec0(i1) vec1(i2) vec1(i3)
                // e.g.
                // // vec0=1234 vec1=5678 -->_mm_shuffle_ps(vec0, vec1, _MM_SHUFFLE(1, 2, 0, 3))=4176
                // ---------------------------------------------------------------
            
                // Gy: 
                //                       gx0     |      gx1
                //                  0  1  2  3  |  4  5  6  7
                //  to get:      2  3  4  5 using shuffle(gx0, gx1, [1 0 3 2])
                F32vec4 vec_gy2 = _mm_shuffle_ps(vec_gy0, vec_gy1, _MM_SHUFFLE(1, 0, 3, 2));       // 2 3 4 5
                //  to get:      1  2  3  4  using gx3=shuffle(gx0, gx2, [2, 1, 2, 1]
                F32vec4 vec_gy3 = _mm_shuffle_ps(vec_gy0, vec_gy2, _MM_SHUFFLE(2, 1, 2, 1));        // 1 2 3 4

                // calculate the final filter response
                vec_gx2 = _mm_sub_ps(vec_gx2, vec_gx0);
                vec_gy2 = _mm_add_ps(vec_gy2, vec_gy0);     // g0 + g3 + g3 + g2
                vec_gy2 = _mm_add_ps(vec_gy2, vec_gy3);
                vec_gy2 = _mm_add_ps(vec_gy2, vec_gy3);
                //grad_x = grad_x * grad_x
                vec_gx2 = _mm_mul_ps(vec_gx2, vec_gx2);
                //grad_y = grad_y * grad_y;
                vec_gy2 = _mm_mul_ps(vec_gy2, vec_gy2);
                // add and sqrt
                F32vec4 vec_gm = _mm_add_ps(vec_gx2, vec_gy2);
                vec_gm = _mm_sqrt_ps(vec_gm);
                //  * 0.125f;
                vec_gm = _mm_mul_ps(vec_gm, vec_consto8);
         
                // swap registers
                vec_gx0 = vec_gx1;
                vec_gy0 = vec_gy1;

                // copy data
                _mm_store_ps(pDst+x, vec_gm);
		    }
            for (; x<iplDstImage->width; x++) {    // remaining pixels
                // -1 0 1
                // -2 0 2
                // -1 0 1
                float grad_x = - pGxxLine[x] + pGxxLine[x+2];
                // -1 -2  -1
                //  0   0   0
                //  1   2   1
			    float grad_y = pGyyLine[x] + pGyyLine[x+1] + pGyyLine[x+1] + pGyyLine[x+2];
                // Gm
                pDst[x] = sqrt(grad_x*grad_x + grad_y*grad_y) * 0.125f;
		    }
        }
	} else if (direction == 3) {
        for (int y=0; y<iplDstImage->height; y++) {
            __declspec(align(64)) float *pSrcN = (__declspec(align(64)) float *)(iplSrcImage->imageData + y * iplSrcImage->widthStep);        // each line of the image has been aligned
            __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+1) * iplSrcImage->widthStep);
            __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+2) * iplSrcImage->widthStep);
            __declspec(align(64)) float *pDst = (__declspec(align(64)) float *)(iplDstImage->imageData + y * iplDstImage->widthStep);
            // pre-calculate in Y direction
            int x;
            for (x=0; x<iplSrcImage->width; x+=4) {
                //pGxxLine[x] = pSrcN[x]  + pSrc0[x]  + pSrc0[x] + pSrc1[x];      // 1 2 1 in column
                F32vec4 vec_p0 = _mm_load_ps(pSrcN+x);
                F32vec4 vec_p1 = _mm_load_ps(pSrc0+x);
                F32vec4 vec_p2 = _mm_load_ps(pSrc1+x);
                F32vec4 vec_grad_x = _mm_add_ps(vec_p0, vec_p1);
                vec_grad_x = _mm_add_ps(vec_grad_x, vec_p1);
                vec_grad_x = _mm_add_ps(vec_grad_x, vec_p2);
			    //pGyyLine[x] = pSrc1[x] - pSrcN[x];                                          // -1 0 1 in column
                F32vec4 vec_grad_y = _mm_sub_ps(vec_p2, vec_p0);
                // write back
                _mm_store_ps(pGxxLine+x, vec_grad_x);
                _mm_store_ps(pGyyLine+x, vec_grad_y);
            }
            for (; x<iplSrcImage->width; x++) {        // remaining pixels
                // -1 0 1
                // -2 0 2
                // -1 0 1
                pGxxLine[x] = pSrcN[x]  + pSrc0[x]  + pSrc0[x] + pSrc1[x];      // 1 2 1 in column
                // -1  -2  -1
                //  0   0   0
                //  1   2   1
			    pGyyLine[x] = pSrc1[x] - pSrcN[x];                                          // -1 0 1 in column
            }

            // calculate the filter
            // pre load 4 elements
            F32vec4 vec_gx0 = _mm_load_ps(pGxxLine);
            F32vec4 vec_gy0 = _mm_load_ps(pGyyLine);
            for (x=0; x<most_right; x+=4) {
                int xx = x + 4;
                //float grad_x = - pGxxLine[x] + pGxxLine[x+2];
                F32vec4 vec_gx1 = _mm_load_ps(pGxxLine+xx);
                F32vec4 vec_gy1 = _mm_load_ps(pGyyLine+xx);
                // Gx:
                //                       gx0     |      gx1
                //                  0  1  2  3  |  4  5  6  7
                //  to get:      2  3  4  5 using shuffle(gx0, gx1, [1 0 3 2])
                F32vec4 vec_gx2 = _mm_shuffle_ps(vec_gx0, vec_gx1, _MM_SHUFFLE(1, 0, 3, 2));
                // ---------------- Usage of _MM_SHUFFLE -------------------
                // vec0.m_128_f32[4] = abcd;  vec0.m_128_f32[4]=efgh
                // vec2 = _mm_shuffle_ps(vec0, vec1, _MM_SHUFFLE(i3, i2, i1, i0))
                // Then:
                // vec2.m_128_f32[4] = vec0(i0) vec0(i1) vec1(i2) vec1(i3)
                // e.g.
                // // vec0=1234 vec1=5678 -->_mm_shuffle_ps(vec0, vec1, _MM_SHUFFLE(1, 2, 0, 3))=4176
                // ---------------------------------------------------------------
            
                // Gy: 
                //                       gx0     |      gx1
                //                  0  1  2  3  |  4  5  6  7
                //  to get:      2  3  4  5 using gx2 = shuffle(gx0, gx1, [1 0 3 2])
                F32vec4 vec_gy2 = _mm_shuffle_ps(vec_gy0, vec_gy1, _MM_SHUFFLE(1, 0, 3, 2));       // 2 3 4 5
                //  to get:      1  2  3  4  using gx3=shuffle(gx0, gx2, [2, 1, 2, 1]
                F32vec4 vec_gy3 = _mm_shuffle_ps(vec_gy0, vec_gy2, _MM_SHUFFLE(2, 1, 2, 1));        // 1 2 3 4

                // calculate the final filter response
                vec_gx2 = _mm_sub_ps(vec_gx2, vec_gx0);
                vec_gy2 = _mm_add_ps(vec_gy2, vec_gy0);     // g0 + g3 + g3 + g2
                vec_gy2 = _mm_add_ps(vec_gy2, vec_gy3);
                vec_gy2 = _mm_add_ps(vec_gy2, vec_gy3);
                //grad_x = grad_x < 0.0f ? - grad_x : grad_x;
                vec_gx2 = _mm_and_ps(vec_gx2, SIGNMASK);
                //grad_y = grad_y < 0.0f ? - grad_y : grad_y;
                vec_gy2 = _mm_and_ps(vec_gy2, SIGNMASK);
                //pGmm[x] = (grad_x + grad_y) * 0.125f;
                F32vec4 vec_gm = _mm_add_ps(vec_gx2, vec_gy2);
                vec_gm = _mm_mul_ps(vec_gm, vec_consto8);
         
                // swap registers
                vec_gx0 = vec_gx1;
                vec_gy0 = vec_gy1;

                // copy data
                _mm_store_ps(pDst+x, vec_gm);
		    }
            for (; x<iplDstImage->width; x++) {    // remaining pixels
                // -1 0 1
                // -2 0 2
                // -1 0 1
                float grad_x = - pGxxLine[x] + pGxxLine[x+2];
                // -1  -2  -1
                //  0   0   0
                //  1   2   1
			    float grad_y = pGyyLine[x] + pGyyLine[x+1] + pGyyLine[x+1] + pGyyLine[x+2];
                // Gm
                grad_x = grad_x < 0.0f ? - grad_x : grad_x;
                grad_y = grad_y < 0.0f ? - grad_y : grad_y;
                pDst[x] = (grad_x + grad_y) * 0.125f;
		    }
        }
	} else {
		showErrMsg("Invalid filter type in CImageUtility::sobel_o1_32f_SIMD()!\n");
        _aligned_free(pGxxLine);
        _aligned_free(pGyyLine);
		return false;
	} 

    _aligned_free(pGxxLine);
    _aligned_free(pGyyLine);
	
	return true;
}

bool CImageUtility::sobel_laplace_o1_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage)
// SIMD accelerated version (use SSE3 instrinsics)
// Sobel + Laplace operator: Sobel : SAD of gradients in x and y directions
// Sobel operator. This function suppose the input image has been padded 1 pixel in each direction
// Arguments:
//		iplSrcImage -- [I] input image; must be 32F floating point, 1-channel image, has been padded 1 pixel in each direction
//		iplDstImage -- [O] input image; must be 32F floating point, 1-channel image
{
	if (iplSrcImage == NULL || iplDstImage == NULL ||
		iplSrcImage->nChannels != 1 || iplDstImage->nChannels != 1 || iplSrcImage->depth != SR_DEPTH_32F || iplDstImage->depth != SR_DEPTH_32F ||
        iplSrcImage->width != iplDstImage->width+2 || iplSrcImage->height != iplDstImage->height+2)  {
		showErrMsg("Invalid input/output image in CImageUtility::sobel_laplace_o1_32f()!\n");
		return false;
	}

    //empty();        // flush the MMX registers

    // allocate aligned buffers
    const int align_floats = _SIMD_ALIGN_BYTES / sizeof(float);
    int aligned_size_line = get_aligned_size(iplSrcImage->width, align_floats);
    __declspec(align(64)) float *pGxxLine = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
    __declspec(align(64)) float *pGyyLine = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
    __declspec(align(64)) float *pLapLine = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
	if (pGxxLine == NULL || pGyyLine == NULL || pLapLine == NULL)  {
        _aligned_free(pGxxLine);
        _aligned_free(pGyyLine);
        _aligned_free(pLapLine);
		showErrMsg("Fail to allocate image buffer in CImageUtility::sobel_laplace_32f()!\n");
		return false;
	}

	// calculate gradient
    static const __m128 vec_const0 = _mm_set1_ps(0.0f);
    static const __m128 vec_const4 = _mm_set1_ps(4.0f);
    static const __m128 vec_consto4 = _mm_set1_ps(1.0f/4.0f);
    static const __m128 vec_consto8 = _mm_set1_ps(1.0f/8.0f);
    static const __m128 vec_consto16 = _mm_set1_ps(1.0f/16.0f);
    static const __m128 SIGNMASK = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)); 
    //int most_right = (aligned_size_line - 4) < (iplSrcImage->width-2) ? (aligned_size_line - 8) : (iplSrcImage->width-2);
    int most_right = (aligned_size_line - 4) < (iplSrcImage->width-2) ? (aligned_size_line - 4) : (iplSrcImage->width-2);
    
    for (int y=0; y<iplDstImage->height; y++) {
        __declspec(align(64)) float *pSrcN = (__declspec(align(64)) float *)(iplSrcImage->imageData + y * iplSrcImage->widthStep);        // each line of the image has been aligned
        __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+1) * iplSrcImage->widthStep);
        __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+2) * iplSrcImage->widthStep);
        __declspec(align(64)) float *pDst = (__declspec(align(64)) float *)(iplDstImage->imageData + y * iplDstImage->widthStep);
        // pre-calculate in Y direction
        int x;
        for (x=0; x<iplSrcImage->width; x+=4) {
            //pGxxLine[x] = pSrcN[x]  + pSrc0[x]  + pSrc0[x] + pSrc1[x];      // 1 2 1 in column
            F32vec4 vec_p0 = _mm_load_ps(pSrcN+x);
            F32vec4 vec_p1 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_p2 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_grad_x = _mm_add_ps(vec_p0, vec_p1);
            vec_grad_x = _mm_add_ps(vec_grad_x, vec_p1);
            vec_grad_x = _mm_add_ps(vec_grad_x, vec_p2);
            //pGyyLine[x] = pSrc1[x] - pSrcN[x];                                          // -1 0 1 in column
            F32vec4 vec_grad_y = _mm_sub_ps(vec_p2, vec_p0);
            // pLapLine[x] = - pSrcN[x]  + pSrc0[x] *4.0f - pSrc1[x];              // -1 4 -1 column
            F32vec4 vec_lap = _mm_mul_ps(vec_p1, vec_const4);
            vec_lap = _mm_sub_ps(vec_lap, vec_p0);
            vec_lap = _mm_sub_ps(vec_lap, vec_p2);
            // write back
            _mm_store_ps(pGxxLine+x, vec_grad_x);
            _mm_store_ps(pGyyLine+x, vec_grad_y);
            _mm_store_ps(pLapLine+x, vec_lap);
        }
        for (; x<iplSrcImage->width; x++) {        // remaining pixels
            // -1 0 1
            // -2 0 2
            // -1 0 1
            pGxxLine[x] = pSrcN[x]  + pSrc0[x]  + pSrc0[x] + pSrc1[x];      // 1 2 1 in column
            // -1  -2  -1
            //  0   0   0
            //  1   2   1
            pGyyLine[x] = pSrc1[x] - pSrcN[x];                                          // -1 0 1 in column
            //   0   -1   0
            //  -1   4   -1
            //   0   -1   0
            pLapLine[x] = - pSrcN[x]  + pSrc0[x] *4.0f - pSrc1[x];              // -1 4 -1 column
        }
        
        // calculate the filter
        // pre load 4 elements
        F32vec4 vec_gx0 = _mm_load_ps(pGxxLine);
        F32vec4 vec_gy0 = _mm_load_ps(pGyyLine);
        F32vec4 vec_lapm0 = _mm_load_ps(pLapLine);
        F32vec4 vec_lap0 = _mm_load_ps(pSrc0);
        for (x=0; x<most_right; x+=4) {
            int xx = x + 4;
            //float grad_x = - pGxxLine[x] + pGxxLine[x+2];
            F32vec4 vec_gx1 = _mm_load_ps(pGxxLine+xx);
            F32vec4 vec_gy1 = _mm_load_ps(pGyyLine+xx);
            F32vec4 vec_lapm1 = _mm_load_ps(pLapLine+xx);
            F32vec4 vec_lap1 = _mm_load_ps(pSrc0+xx);
            // Gx:
            //                       gx0     |      gx1
            //                  0  1  2  3  |  4  5  6  7
            //  to get:      2  3  4  5 using shuffle(gx0, gx1, [1 0 3 2])
            F32vec4 vec_gx2 = _mm_shuffle_ps(vec_gx0, vec_gx1, _MM_SHUFFLE(1, 0, 3, 2));
            // ---------------- Usage of _MM_SHUFFLE -------------------
            // vec0.m_128_f32[4] = abcd;  vec0.m_128_f32[4]=efgh
            // vec2 = _mm_shuffle_ps(vec0, vec1, _MM_SHUFFLE(i3, i2, i1, i0))
            // Then:
            // vec2.m_128_f32[4] = vec0(i0) vec0(i1) vec1(i2) vec1(i3)
            // e.g.
            // // vec0=1234 vec1=5678 -->_mm_shuffle_ps(vec0, vec1, _MM_SHUFFLE(1, 2, 0, 3))=4176
            // ---------------------------------------------------------------
            
            // Gy: 
            //                       gx0     |      gx1
            //                  0  1  2  3  |  4  5  6  7
            //  to get:      2  3  4  5 using shuffle(gx0, gx1, [1 0 3 2])
            F32vec4 vec_gy2 = _mm_shuffle_ps(vec_gy0, vec_gy1, _MM_SHUFFLE(1, 0, 3, 2));       // 2 3 4 5
            //  to get:      1  2  3  4  using gx3=shuffle(gx0, gx2, [2, 1, 2, 1]
            F32vec4 vec_gy3 = _mm_shuffle_ps(vec_gy0, vec_gy2, _MM_SHUFFLE(2, 1, 2, 1));        // 1 2 3 4
            
            // calculate the final filter response
            vec_gx2 = _mm_sub_ps(vec_gx2, vec_gx0);
            vec_gy2 = _mm_add_ps(vec_gy2, vec_gy0);     // g0 + g3 + g3 + g2
            vec_gy2 = _mm_add_ps(vec_gy2, vec_gy3);
            vec_gy2 = _mm_add_ps(vec_gy2, vec_gy3);
            //grad_x = grad_x < 0.0f ? - grad_x : grad_x;
            vec_gx2 = _mm_and_ps(vec_gx2, SIGNMASK);
            //grad_y = grad_y < 0.0f ? - grad_y : grad_y;
            vec_gy2 = _mm_and_ps(vec_gy2, SIGNMASK);
            //pGmm[x] = (grad_x + grad_y) * 0.125f;
            F32vec4 vec_gm = _mm_add_ps(vec_gx2, vec_gy2);
            vec_gm = _mm_mul_ps(vec_gm, vec_consto8);
         
            // Lap: 
            //                       lapm0     |      lapm1
            //                  0  1  2  3  |  4  5  6  7
            //                       lap0     |      lap1
            //  to get:      2  3  4  5 using gx2 = shuffle(gx0, gx1, [1 0 3 2])
            F32vec4 vec_lap2 = _mm_shuffle_ps(vec_lap0, vec_lap1, _MM_SHUFFLE(1, 0, 3, 2));             // 2 3 4 5
            //  to get:      1  2  3  4  using gx2 = shuffle(gx0, gx1, [1 0 3 2]) and gx3=shuffle(gx0, gx2, [2, 1, 2, 1]
            F32vec4 vec_lapm = _mm_shuffle_ps(vec_lapm0, vec_lapm1, _MM_SHUFFLE(1, 0, 3, 2));             // 2 3 4 5
            vec_lapm = _mm_shuffle_ps(vec_lapm0, vec_lapm, _MM_SHUFFLE(2, 1, 2, 1));                     // 1 2 3 4

            // -1 1 -1 (-lap0 lap3 -lap2)
            vec_lapm = _mm_sub_ps(vec_lapm, vec_lap0);
            vec_lapm = _mm_sub_ps(vec_lapm, vec_lap2);
            // abs
            vec_lapm = _mm_and_ps(vec_lapm, SIGNMASK);

            // swap registers
            vec_gx0 = vec_gx1;
            vec_gy0 = vec_gy1;
            vec_lap0 = vec_lap1;
            vec_lapm0 = vec_lapm1;

            // Sobel + Laplace
           vec_gm = _mm_add_ps(vec_gm, vec_lapm);
            
            // copy data
            _mm_store_ps(pDst+x, vec_gm);
        }
        
        for (; x<iplDstImage->width; x++) {    // remaining pixels
            // -1 0 1
            // -2 0 2
            // -1 0 1
            float grad_x = - pGxxLine[x] + pGxxLine[x+2];
            // -1  -2  -1
            //  0   0   0
            //  1   2   1
            float grad_y = pGyyLine[x] + pGyyLine[x+1] + pGyyLine[x+1] + pGyyLine[x+2];
            // Gm
            grad_x = grad_x < 0.0f ? - grad_x : grad_x;
            grad_y = grad_y < 0.0f ? - grad_y : grad_y;
            float grad_m = (grad_x + grad_y) * 0.125f;
            // Lap
            float lap = -pLapLine[x] + pLapLine[x+1] - pLapLine[x+2];
            lap = lap < 0.0f ? -lap : lap;
            pDst[x] = grad_m + lap;
        }
	}

    _aligned_free(pGxxLine);
    _aligned_free(pGyyLine);
    _aligned_free(pLapLine);

	return true;
}

bool CImageUtility::gaussian3x3_o1_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage, float sigma)
 // SIMD accelerated version (use SSE3 instrinsics)
// Gaussian filter
// This function supposes the input image has been padded 1 pixel in each direction
// Arguments:
//		iplSrcImage -- [I] input image; must be 32F floating point, 1-channel image, has been padded 1 pixel in each direction
//		iplDstImage -- [O] input image; must be 32F floating point, 1-channel image
// by Luhong Liang
// Aug, 2014
{
	if (iplSrcImage == NULL || iplDstImage == NULL ||
		iplSrcImage->nChannels != 1 || iplDstImage->nChannels != 1 || iplSrcImage->depth != SR_DEPTH_32F || iplDstImage->depth != SR_DEPTH_32F ||
        iplSrcImage->width != iplDstImage->width+2 || iplSrcImage->height != iplDstImage->height+2)  {
		showErrMsg("Invalid input argument in CImageUtility::gaussian3x3_o1_32f()!\n");
		return false;
	}

    //empty();        // flush the MMX registers

	// make filter
    __declspec(align(64)) float pFilter[3];
	float div = 0.0f;
	float div_sigma_sq2 = 0.5f / (sigma * sigma);
	for (int i=0; i<3; i++) {
		float diff = (float)(i - 1);
		float exp_num = - diff * diff * div_sigma_sq2;
		float value = exp(exp_num);
        value = value < FLT_MIN ? 0.0f : value;     // it looks a bug in exp()? must check underflow here!
		pFilter[i] = value;
		div += value;
	}
	for (int i=0; i<3; i++) {
		pFilter[i] = pFilter[i] / div;
	}
    F32vec4 vec_f0 = _mm_set1_ps(pFilter[0]);
    F32vec4 vec_f1 = _mm_set1_ps(pFilter[1]);
    float f0 = pFilter[0];
    float f1 = pFilter[1];

	// allocate buffer
    const int align_floats = _SIMD_ALIGN_BYTES / sizeof(float);
    int aligned_size_line = get_aligned_size(iplSrcImage->width, align_floats);
    __declspec(align(64)) float *pLine = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
	if (pLine == NULL) {
        showErrMsg("Fail to allocate buffer in CImageUtility::gaussian3x3_o1_32f()!\n");
		return false;
	}

	// filtering
    int most_right = (aligned_size_line - 4) < iplDstImage->width ? (aligned_size_line - 4) : iplDstImage->width;
	for (int y=0; y<iplDstImage->height; y++) {
		__declspec(align(64)) float *pDst = (__declspec(align(64)) float *)(iplDstImage->imageData + y * iplDstImage->widthStep);
        __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcImage->imageData + y * iplSrcImage->widthStep);
        __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+1) * iplSrcImage->widthStep);
        __declspec(align(64)) float *pSrc2 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+2) * iplSrcImage->widthStep);
        // convolution in Y direction
        int x;
        for (x=0; x<iplSrcImage->width; x+=4) {
			// load
            F32vec4 vec_p0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_p1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_p2 = _mm_load_ps(pSrc2+x);
            //pLine[x] = pSrc0[x] * f0 + pSrc1[x] * f1 + pSrc2[x] * f2;
            vec_p0 = _mm_mul_ps(vec_p0, vec_f0);
            vec_p1 = _mm_mul_ps(vec_p1, vec_f1);
            vec_p2 = _mm_mul_ps(vec_p2, vec_f0);
            vec_p0 = _mm_add_ps(vec_p0, vec_p1);
            vec_p0 = _mm_add_ps(vec_p0, vec_p2);
            // store
            _mm_store_ps(pLine+x, vec_p0);
		}
        for (; x<iplSrcImage->width; x++) {      // // remains
			pLine[x] = pSrc0[x] * f0 + pSrc1[x] * f1 + pSrc2[x] * f0;
		}
        // convolution in X direction
        F32vec4 vec_p0 = _mm_load_ps(pLine);
		for (x=0; x<most_right; x+=4) {
            int xx = x + 4;
            F32vec4 vec_p1 = _mm_load_ps(pLine+xx);
            //                       gx0     |      gx1
            //                  0  1  2  3  |  4  5  6  7
            //  to get:      2  3  4  5 using shuffle(gx0, gx1, [1 0 3 2])
            F32vec4 vec_p2 = _mm_shuffle_ps(vec_p0, vec_p1, _MM_SHUFFLE(1, 0, 3, 2));             // 2 3 4 5
            //  to get:      1  2  3  4  using gx3=shuffle(gx0, gx2, [2, 1, 2, 1]
            F32vec4 vec_p3 = _mm_shuffle_ps(vec_p0, vec_p2, _MM_SHUFFLE(2, 1, 2, 1));             // 1 2 3 4
            // (p0, p3, p2)*f
            vec_p0 = _mm_mul_ps(vec_p0, vec_f0);
            vec_p3 = _mm_mul_ps(vec_p3, vec_f1);
            vec_p2 = _mm_mul_ps(vec_p2, vec_f0);
            vec_p2 = _mm_add_ps(vec_p2, vec_p0);
            vec_p2 = _mm_add_ps(vec_p2, vec_p3);
            // swap
            vec_p0 = vec_p1;
            // store
            _mm_store_ps(pDst+x, vec_p2);
		}
		for (; x<iplDstImage->width; x++) {      // remains
			pDst[x] = pLine[x] * f0 + pLine[x+1] * f1 + pLine[x+2] * f0;
		}
	}

    _aligned_free(pLine);

    return true;
}

bool CImageUtility::gaussian5x5_o1_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage, float sigma)
// SIMD accelerated version (use SSE3 instrinsics)
// Gaussian filter
// This function supposes the input image has been padded 2 pixels in each direction
// Arguments:
//		iplSrcImage -- [I] input image; must be 32F floating point, 1-channel image, has been padded 2 pixels in each direction
//		iplDstImage -- [O] input image; must be 32F floating point, 1-channel image
// by Luhong Liang
// Aug 6, 2014
{
	if (iplSrcImage == NULL || iplDstImage == NULL ||
		iplSrcImage->nChannels != 1 || iplDstImage->nChannels != 1 || iplSrcImage->depth != SR_DEPTH_32F || iplDstImage->depth != SR_DEPTH_32F ||
        iplSrcImage->width != iplDstImage->width+4 || iplSrcImage->height != iplDstImage->height+4)  {
		showErrMsg("Invalid input argument in CImageUtility::gaussian5x5_o1_32f()!\n");
		return false;
	}

    //empty();        // flush the MMX registers

	// make filter
    __declspec(align(64)) float pFilter[5];
	float div = 0.0f;
	float div_sigma_sq2 = 0.5f / (sigma * sigma);
	for (int i=0; i<5; i++) {
		float diff = (float)(i - 2);
		float exp_num = - diff * diff * div_sigma_sq2;
		float value = exp(exp_num);
        value = value < FLT_MIN ? 0.0f : value;     // it looks a bug in exp()? must check underflow here!
		pFilter[i] = value;
		div += value;
	}
	for (int i=0; i<5; i++) {
		pFilter[i] = pFilter[i] / div;
	}
    F32vec4 vec_f0 = _mm_set1_ps(pFilter[0]);
    F32vec4 vec_f1 = _mm_set1_ps(pFilter[1]);
    F32vec4 vec_f2 = _mm_set1_ps(pFilter[2]);
    float f0 = pFilter[0];
    float f1 = pFilter[1];
    float f2 = pFilter[2];

	// allocate buffer
    const int align_floats = _SIMD_ALIGN_BYTES / sizeof(float);
    int aligned_size_line = get_aligned_size(iplSrcImage->width, align_floats);
    __declspec(align(64)) float *pLine = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
	if (pLine == NULL) {
        showErrMsg("Fail to allocate buffer in CImageUtility::gaussian5x5_o1_32f()!\n");
		return false;
	}

	// filtering
    int most_right = (aligned_size_line - 4) < iplDstImage->width ? (aligned_size_line - 4) : iplDstImage->width;
	for (int y=0; y<iplDstImage->height; y++) {
		__declspec(align(64)) float *pDst = (__declspec(align(64)) float *)(iplDstImage->imageData + y * iplDstImage->widthStep);
        __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcImage->imageData + y * iplSrcImage->widthStep);
        __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+1) * iplSrcImage->widthStep);
        __declspec(align(64)) float *pSrc2 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+2) * iplSrcImage->widthStep);
        __declspec(align(64)) float *pSrc3 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+3) * iplSrcImage->widthStep);
        __declspec(align(64)) float *pSrc4 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+4) * iplSrcImage->widthStep);
        // convolution in Y direction
        int x;
        for (x=0; x<iplSrcImage->width; x+=4) {
			// load
            F32vec4 vec_p0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_p1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_p2 = _mm_load_ps(pSrc2+x);
            F32vec4 vec_p3 = _mm_load_ps(pSrc3+x);
            F32vec4 vec_p4 = _mm_load_ps(pSrc4+x);
            //pLine[x] = pSrc0[x] * f0 + pSrc1[x] * f1 + pSrc2[x] * f2 + pSrc3[x] * f3 + pSrc4[x] * f4;
            vec_p0 = _mm_mul_ps(vec_p0, vec_f0);
            vec_p1 = _mm_mul_ps(vec_p1, vec_f1);
            vec_p2 = _mm_mul_ps(vec_p2, vec_f2);
            vec_p3 = _mm_mul_ps(vec_p3, vec_f1);
            vec_p4 = _mm_mul_ps(vec_p4, vec_f0);
            vec_p0 = _mm_add_ps(vec_p0, vec_p1);
            vec_p0 = _mm_add_ps(vec_p0, vec_p2);
            vec_p0 = _mm_add_ps(vec_p0, vec_p3);
            vec_p0 = _mm_add_ps(vec_p0, vec_p4);
            // store
            _mm_store_ps(pLine+x, vec_p0);
		}
        for (; x<iplSrcImage->width; x++) {      // // remains
			pLine[x] = pSrc0[x] * f0 + pSrc1[x] * f1 + pSrc2[x] * f2 + pSrc3[x] * f1 + pSrc4[x] * f0;
		}
        // convolution in X direction
        F32vec4 vec_p0 = _mm_load_ps(pLine);
        for (x=0; x<most_right; x+=4) {
            // load
            F32vec4 vec_p1 = _mm_load_ps(pLine+x+4);        // 4 5 6 7           
            //  get:      2  3  4  5 by m2=shuffle(p0, p1, [1 0 3 2])
            F32vec4 vec_m2 = _mm_shuffle_ps(vec_p0, vec_p1, _MM_SHUFFLE(1, 0, 3, 2));         // 2 3 4 5
            //  get:      1  2  3  4  by m1=shuffle(p0, m2, [2, 1, 2, 1]
            F32vec4 vec_m1 = _mm_shuffle_ps(vec_p0, vec_m2, _MM_SHUFFLE(2, 1, 2, 1));        // 1 2 3 4
            //  get:      3  4  5  6 by m3=shuffle(m1, p1, [2 1 3 2])
            F32vec4 vec_m3 = _mm_shuffle_ps(vec_m1, vec_p1, _MM_SHUFFLE(2, 1, 3, 2));        // 3 4 5 6

            // pDst[x] = pLine[x] * f0 + pLine[x+1] * f1 + pLine[x+2] * f2 + pLine[x+3] * f1 + pLine[x+4] * f0;
            vec_p0 = _mm_mul_ps(vec_p0, vec_f0);
            vec_m1 = _mm_mul_ps(vec_m1, vec_f1);
            vec_m2 = _mm_mul_ps(vec_m2, vec_f2);
            vec_m3 = _mm_mul_ps(vec_m3, vec_f1);
            F32vec4 vec_m4 = _mm_mul_ps(vec_p1, vec_f0);
            vec_m1 = _mm_add_ps(vec_m1, vec_p0);
            vec_m1 = _mm_add_ps(vec_m1, vec_m2);
            vec_m1 = _mm_add_ps(vec_m1, vec_m3);
            vec_m1 = _mm_add_ps(vec_m1, vec_m4);

            // swap
            vec_p0 = vec_p1;

            // store
            _mm_store_ps(pDst+x, vec_m1);
        }
		for (; x<iplDstImage->width; x++) {      // remains
			pDst[x] = pLine[x] * f0 + pLine[x+1] * f1 + pLine[x+2] * f2 + pLine[x+3] * f1 + pLine[x+4] * f0;
		}
	}

    _aligned_free(pLine);

	return true;
}

bool CImageUtility::gaussian7x7_o1_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage, float sigma)
// Gaussian filter
// This function supposes the input image has been padded 3 pixels in each direction
// Arguments:
//		iplSrcImage -- [I] input image; must be 32F floating point, 1-channel image, has been padded 3 pixels in each direction
//		iplDstImage -- [O] input image; must be 32F floating point, 1-channel image
// by Luhong Liang
// August, 2014
{
	if (iplSrcImage == NULL || iplDstImage == NULL ||
		iplSrcImage->nChannels != 1 || iplDstImage->nChannels != 1 || iplSrcImage->depth != SR_DEPTH_32F || iplDstImage->depth != SR_DEPTH_32F ||
        iplSrcImage->width != iplDstImage->width+6 || iplSrcImage->height != iplDstImage->height+6)  {
		showErrMsg("Invalid input argument in CImageUtility::gaussian7x7_o1_32f()!\n");
		return false;
	}

    //empty();        // flush the MMX registers

	// make filter
    __declspec(align(64)) float pFilter[7];
	float div = 0.0f;
	float div_sigma_sq2 = 0.5f / (sigma * sigma);
	for (int i=0; i<7; i++) {
		float diff = (float)(i - 3);
		float exp_num = - diff * diff * div_sigma_sq2;
		float value = exp(exp_num);
        value = value < FLT_MIN ? 0.0f : value;     // it looks a bug in exp()? must check underflow here!
		pFilter[i] = value;
		div += value;
	}
	for (int i=0; i<4; i++) {
		pFilter[i] = pFilter[i] / div;
	}
    F32vec4 vec_f0 = _mm_set1_ps(pFilter[0]);
    F32vec4 vec_f1 = _mm_set1_ps(pFilter[1]);
    F32vec4 vec_f2 = _mm_set1_ps(pFilter[2]);
    F32vec4 vec_f3 = _mm_set1_ps(pFilter[3]);
    float f0 = pFilter[0];
    float f1 = pFilter[1];
    float f2 = pFilter[2];
    float f3 = pFilter[3];

	// allocate buffer
    const int align_floats = _SIMD_ALIGN_BYTES / sizeof(float);
    int aligned_size_line = get_aligned_size(iplSrcImage->width, align_floats);
    __declspec(align(64)) float *pLine = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
	if (pLine == NULL) {
        showErrMsg("Fail to allocate buffer in CImageUtility::gaussian7x7_o1_32f()!\n");
		return false;
	}

	// filtering
    int most_right = (aligned_size_line - 8) < iplDstImage->width ? (aligned_size_line - 8) : iplDstImage->width;
	for (int y=0; y<iplDstImage->height; y++) {
		__declspec(align(64)) float *pDst = (__declspec(align(64)) float *)(iplDstImage->imageData + y * iplDstImage->widthStep);
        __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcImage->imageData + y * iplSrcImage->widthStep);
        __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+1) * iplSrcImage->widthStep);
        __declspec(align(64)) float *pSrc2 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+2) * iplSrcImage->widthStep);
        __declspec(align(64)) float *pSrc3 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+3) * iplSrcImage->widthStep);
        __declspec(align(64)) float *pSrc4 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+4) * iplSrcImage->widthStep);
        __declspec(align(64)) float *pSrc5 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+5) * iplSrcImage->widthStep);
        __declspec(align(64)) float *pSrc6 = (__declspec(align(64)) float *)(iplSrcImage->imageData + (y+6) * iplSrcImage->widthStep);
        // convolution in Y direction
        int x;
        for (x=0; x<iplSrcImage->width; x+=4) {
			// load
            F32vec4 vec_p0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_p1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_p2 = _mm_load_ps(pSrc2+x);
            F32vec4 vec_p3 = _mm_load_ps(pSrc3+x);
            F32vec4 vec_p4 = _mm_load_ps(pSrc4+x);
            F32vec4 vec_p5 = _mm_load_ps(pSrc5+x);
            F32vec4 vec_p6 = _mm_load_ps(pSrc6+x);
			//pLine[x] = pSrc0[x] * f0 + pSrc1[x] * f1 + pSrc2[x] * f2 + pSrc3[x] * f3 +
            //                pSrc4[x] * f2 + pSrc5[x] * f1 + pSrc6[x] * f0;
            vec_p0 = _mm_mul_ps(vec_p0, vec_f0);
            vec_p1 = _mm_mul_ps(vec_p1, vec_f1);
            vec_p2 = _mm_mul_ps(vec_p2, vec_f2);
            vec_p3 = _mm_mul_ps(vec_p3, vec_f3);
            vec_p4 = _mm_mul_ps(vec_p4, vec_f2);
            vec_p5 = _mm_mul_ps(vec_p5, vec_f1);
            vec_p6 = _mm_mul_ps(vec_p6, vec_f0);
            vec_p0 = _mm_add_ps(vec_p0, vec_p1);
            vec_p0 = _mm_add_ps(vec_p0, vec_p2);
            vec_p0 = _mm_add_ps(vec_p0, vec_p3);
            vec_p0 = _mm_add_ps(vec_p0, vec_p4);
            vec_p0 = _mm_add_ps(vec_p0, vec_p5);
            vec_p0 = _mm_add_ps(vec_p0, vec_p6);
            // store
            _mm_store_ps(pLine+x, vec_p0);
		}
		for (; x<iplSrcImage->width; x++) {      // remains
			pLine[x] = pSrc0[x] * f0 + pSrc1[x] * f1 + pSrc2[x] * f2 + pSrc3[x] * f3 +
                            pSrc4[x] * f2 + pSrc5[x] * f1 + pSrc6[x] * f0;
		}
        // convolution in X direction
        F32vec4 vec_p0 = _mm_load_ps(pLine);        // 1 2 3 4
        F32vec4 vec_p1 = _mm_load_ps(pLine+4);    // 4 5 6 7           
        for (x=0; x<most_right; x+=4) {
            // load
            F32vec4 vec_p2 = _mm_load_ps(pLine+x+8);        // 8 9 10 11

            //  get:      2  3  4  5 by m2=shuffle(p0, p1, [1 0 3 2])
            F32vec4 vec_m2 = _mm_shuffle_ps(vec_p0, vec_p1, _MM_SHUFFLE(1, 0, 3, 2));         // 2 3 4 5
            //  get:      1  2  3  4  by m1=shuffle(p0, m2, [2, 1, 2, 1]
            F32vec4 vec_m1 = _mm_shuffle_ps(vec_p0, vec_m2, _MM_SHUFFLE(2, 1, 2, 1));        // 1 2 3 4
            //  get:      3  4  5  6 by m3=shuffle(m1, p1, [2 1 3 2])
            F32vec4 vec_m3 = _mm_shuffle_ps(vec_m1, vec_p1, _MM_SHUFFLE(2, 1, 3, 2));        // 3 4 5 6
            //  get:      6 7  8  9 by m3=shuffle(p1, p2, [1 0 3 2])
            F32vec4 vec_m6 = _mm_shuffle_ps(vec_p1, vec_p2, _MM_SHUFFLE(1, 0, 3, 2));        // 6 7 8 9
            //  get:      5  6 7  8 by m3=shuffle(m3, m6, [2 1 3 2])
            F32vec4 vec_m5 = _mm_shuffle_ps(vec_m3, vec_m6, _MM_SHUFFLE(2, 1, 3, 2));      // 5 6 7 8

			//pDst[x] = pLine[x] * f0 + pLine[x+1] * f1 + pLine[x+2] * f2 + pLine[x+3] * f3 + 
            //               pLine[x+4] * f2 + pLine[x+5] * f1 + pLine[x+6] * f0;
            vec_p0 = _mm_mul_ps(vec_p0, vec_f0);
            vec_m1 = _mm_mul_ps(vec_m1, vec_f1);
            vec_m2 = _mm_mul_ps(vec_m2, vec_f2);
            vec_m3 = _mm_mul_ps(vec_m3, vec_f3);
            F32vec4 vec_m4 = _mm_mul_ps(vec_p1, vec_f2);
            vec_m5 = _mm_mul_ps(vec_m5, vec_f1);
            vec_m6 = _mm_mul_ps(vec_m6, vec_f0);
            vec_m1 = _mm_add_ps(vec_m1, vec_p0);
            vec_m1 = _mm_add_ps(vec_m1, vec_m2);
            vec_m1 = _mm_add_ps(vec_m1, vec_m3);
            vec_m1 = _mm_add_ps(vec_m1, vec_m4);
            vec_m1 = _mm_add_ps(vec_m1, vec_m5);
            vec_m1 = _mm_add_ps(vec_m1, vec_m6);

            // swap
            vec_p0 = vec_p1;
            vec_p1 = vec_p2;

            // store
            _mm_store_ps(pDst+x, vec_m1);
        }
		for (int x=0; x<iplDstImage->width; x++) {
			pDst[x] = pLine[x] * f0 + pLine[x+1] * f1 + pLine[x+2] * f2 + pLine[x+3] * f3 + 
                           pLine[x+4] * f2 + pLine[x+5] * f1 + pLine[x+6] * f0;
		}
	}

	_aligned_free(pLine);

	return true;
}

bool CImageUtility::boxing5x5_o1_32f_SIMD(IplImage *iplImage, IplImage *iplDstImage, float factor)
// SIMD accelerated version (use SSE3 instrinsics)
// Faster implementation of 5x5 boxing filter. This function DOES NOT supports the "in-place" operation 
// (source and destination images are the same one)!
// Compared with boxing5x5_32f(), this function:
// (1) supposing the input image has been padded 2 pixels
// factor -- amplitude factor of the result, i.e. dst = box(src) * factor
// by Luhong Liang
// Aug 6, 2014
{
	if (iplImage == NULL || iplDstImage == NULL ||
		iplImage->nChannels != 1 || iplDstImage->nChannels != 1 || iplImage->depth != SR_DEPTH_32F || iplDstImage->depth != SR_DEPTH_32F ||
        iplImage->width != iplDstImage->width+4 || iplImage->height != iplDstImage->height+4)  {
		showErrMsg("Invalid input argument in CImageUtility::boxing5x5_o1_32f()!\n");
		return false;
	}

    //empty();        // flush the MMX registers

	// allocate buffer
    const int align_floats = _SIMD_ALIGN_BYTES / sizeof(float);
    int aligned_size_line = get_aligned_size(iplImage->width, align_floats);
    __declspec(align(64)) float *pLine = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
	if (pLine == NULL) {
        showErrMsg("Fail to allocate buffer in CImageUtility::boxing5x5_o1_32f()!\n");
		return false;
	}

	// filtering
    int most_right = (aligned_size_line - 4) < iplDstImage->width ? (aligned_size_line - 4) : iplDstImage->width;
    F32vec4 vec_div = _mm_set1_ps(factor/25.0f);
	for (int y=0; y<iplDstImage->height; y++) {
		__declspec(align(64)) float *pDst = (__declspec(align(64)) float *)(iplDstImage->imageData + y * iplDstImage->widthStep);
        __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplImage->imageData + y * iplImage->widthStep);
        __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplImage->imageData + (y+1) * iplImage->widthStep);
        __declspec(align(64)) float *pSrc2 = (__declspec(align(64)) float *)(iplImage->imageData + (y+2) * iplImage->widthStep);
        __declspec(align(64)) float *pSrc3 = (__declspec(align(64)) float *)(iplImage->imageData + (y+3) * iplImage->widthStep);
        __declspec(align(64)) float *pSrc4 = (__declspec(align(64)) float *)(iplImage->imageData + (y+4) * iplImage->widthStep);
        // convolution in Y direction
        int x;
        for (x=0; x<iplImage->width; x+=4) {
			// load
            F32vec4 vec_p0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_p1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_p2 = _mm_load_ps(pSrc2+x);
            F32vec4 vec_p3 = _mm_load_ps(pSrc3+x);
            F32vec4 vec_p4 = _mm_load_ps(pSrc4+x);
            //pLine[x] = pSrc0[x] + pSrc1[x] + pSrc2[x] + pSrc3[x] + pSrc4[x];
            vec_p0 = _mm_add_ps(vec_p0, vec_p1);
            vec_p0 = _mm_add_ps(vec_p0, vec_p2);
            vec_p0 = _mm_add_ps(vec_p0, vec_p3);
            vec_p0 = _mm_add_ps(vec_p0, vec_p4);
            // store
            _mm_store_ps(pLine+x, vec_p0);
		}
        for (; x<iplImage->width; x++) {      // // remains
			pLine[x] = pSrc0[x] + pSrc1[x] + pSrc2[x] + pSrc3[x] + pSrc4[x];
		}
        // convolution in X direction
        F32vec4 vec_p0 = _mm_load_ps(pLine);
        for (x=0; x<most_right; x+=4) {
            // load
            F32vec4 vec_p1 = _mm_load_ps(pLine+x+4);        // 4 5 6 7           
            //  get:      2  3  4  5 by m2=shuffle(p0, p1, [1 0 3 2])
            F32vec4 vec_m2 = _mm_shuffle_ps(vec_p0, vec_p1, _MM_SHUFFLE(1, 0, 3, 2));         // 2 3 4 5
            //  get:      1  2  3  4  by m1=shuffle(p0, m2, [2, 1, 2, 1]
            F32vec4 vec_m1 = _mm_shuffle_ps(vec_p0, vec_m2, _MM_SHUFFLE(2, 1, 2, 1));        // 1 2 3 4
            //  get:      3  4  5  6 by m3=shuffle(m1, p1, [2 1 3 2])
            F32vec4 vec_m3 = _mm_shuffle_ps(vec_m1, vec_p1, _MM_SHUFFLE(2, 1, 3, 2));        // 3 4 5 6

            // pDst[x] = pLine[x] + pLine[x+1] + pLine[x+2] + pLine[x+3] + pLine[x+4];
            vec_m1 = _mm_add_ps(vec_m1, vec_p0);
            vec_m1 = _mm_add_ps(vec_m1, vec_m2);
            vec_m1 = _mm_add_ps(vec_m1, vec_m3);
            vec_m1 = _mm_add_ps(vec_m1, vec_p1);
            vec_m1 = _mm_mul_ps(vec_m1, vec_div);

            // swap
            vec_p0 = vec_p1;

            // store
            _mm_store_ps(pDst+x, vec_m1);
        }
		for (; x<iplDstImage->width; x++) {      // remains
			pDst[x] = pLine[x] + pLine[x+1] + pLine[x+2] + pLine[x+3] + pLine[x+4];
		}
	}

    _aligned_free(pLine);

	return true;
}

bool CImageUtility::contrast5x5_o1_32f_SIMD(IplImage *iplImage, IplImage *iplDstImage)
// SIMD accelerated version (use SSE3 instrinsics)
// Calculate the local contrast in 5x5 window. The contrast is the intensity difference  between the most 
// bright and the most dark pixels.
// Compared with contrast5x5_o1_32f(), this function:
// (1) supposing the input image has been padded 2 pixels
// by Luhong Liang
// Aug, 2014
{
	//if (iplImage == NULL || iplDstImage == NULL ||
	//	iplImage->nChannels != 1 || iplDstImage->nChannels != 1 || iplImage->depth != SR_DEPTH_32F || iplDstImage->depth != SR_DEPTH_32F ||
    //    iplImage->width != iplDstImage->width+4 || iplImage->height != iplDstImage->height+4)  {
	//	showErrMsg("Invalid input argument in CImageUtility::contrast5x5_o1_32f_SIMD()!\n");
	//	return false;
	//}

    //empty();        // flush the MMX registers

	// allocate buffer
    const int align_floats = _SIMD_ALIGN_BYTES / sizeof(float);
    int aligned_size_line = get_aligned_size(iplImage->width, align_floats);
    __declspec(align(64)) float *pLineMax = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
    __declspec(align(64)) float *pLineMin = (__declspec(align(64)) float *)_aligned_malloc(aligned_size_line*sizeof(float), _SIMD_ALIGN_BYTES);
	if (pLineMax == NULL || pLineMin == NULL) {
        showErrMsg("Fail to allocate buffer in CImageUtility::contrast5x5_o1_32f_SIMD()!\n");
        if (pLineMax != NULL) _aligned_free(pLineMax);
        if (pLineMin != NULL) _aligned_free(pLineMin);
		return false;
	}

	// filtering
    int most_right = (aligned_size_line - 4) < iplDstImage->width ? (aligned_size_line - 4) : iplDstImage->width;
	for (int y=0; y<iplDstImage->height; y++) {
		__declspec(align(64)) float *pDst = (__declspec(align(64)) float *)(iplDstImage->imageData + y * iplDstImage->widthStep);
        __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplImage->imageData + y * iplImage->widthStep);
        __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplImage->imageData + (y+1) * iplImage->widthStep);
        __declspec(align(64)) float *pSrc2 = (__declspec(align(64)) float *)(iplImage->imageData + (y+2) * iplImage->widthStep);
        __declspec(align(64)) float *pSrc3 = (__declspec(align(64)) float *)(iplImage->imageData + (y+3) * iplImage->widthStep);
        __declspec(align(64)) float *pSrc4 = (__declspec(align(64)) float *)(iplImage->imageData + (y+4) * iplImage->widthStep);
        // convolution in Y direction
        int x;
        for (x=0; x<iplImage->width; x+=4) {
			// load
            F32vec4 vec_p0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_p1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_p2 = _mm_load_ps(pSrc2+x);
            F32vec4 vec_p3 = _mm_load_ps(pSrc3+x);
            F32vec4 vec_p4 = _mm_load_ps(pSrc4+x);
            // max
            F32vec4 vec_max = _mm_max_ps(vec_p0, vec_p1);
            vec_max = _mm_max_ps(vec_max, vec_p2);
            vec_max = _mm_max_ps(vec_max, vec_p3);
            vec_max = _mm_max_ps(vec_max, vec_p4);
            // min
            F32vec4 vec_min = _mm_min_ps(vec_p0, vec_p1);
            vec_min = _mm_min_ps(vec_min, vec_p2);
            vec_min = _mm_min_ps(vec_min, vec_p3);
            vec_min = _mm_min_ps(vec_min, vec_p4);
            // store
            _mm_store_ps(pLineMax+x, vec_max);
            _mm_store_ps(pLineMin+x, vec_min);
		}
        for (; x<iplImage->width; x++) {      // // remains
            // max
            float maxv = pSrc0[x] > pSrc1[x] ? pSrc0[x] : pSrc1[x];
            maxv = maxv > pSrc2[x] ? maxv : pSrc2[x];
            maxv = maxv > pSrc3[x] ? maxv : pSrc3[x];
            maxv = maxv > pSrc4[x] ? maxv : pSrc4[x];
			pLineMax[x] = maxv;
            // min
            float minv = pSrc0[x] < pSrc1[x] ? pSrc0[x] : pSrc1[x];
            minv = minv < pSrc2[x] ? minv : pSrc2[x];
            minv = minv < pSrc3[x] ? minv : pSrc3[x];
            minv = minv < pSrc4[x] ? minv : pSrc4[x];
			pLineMin[x] = minv;
		}
        // convolution in X direction
        F32vec4 vec_p0 = _mm_load_ps(pLineMax);
        F32vec4 vec_q0 = _mm_load_ps(pLineMin);
        for (x=0; x<most_right; x+=4) {
            // max
            F32vec4 vec_p1 = _mm_load_ps(pLineMax+x+4);        // 4 5 6 7           
            //  get:      2  3  4  5 by m2=shuffle(p0, p1, [1 0 3 2])
            F32vec4 vec_m2 = _mm_shuffle_ps(vec_p0, vec_p1, _MM_SHUFFLE(1, 0, 3, 2));         // 2 3 4 5
            //  get:      1  2  3  4  by m1=shuffle(p0, m2, [2, 1, 2, 1]
            F32vec4 vec_m1 = _mm_shuffle_ps(vec_p0, vec_m2, _MM_SHUFFLE(2, 1, 2, 1));        // 1 2 3 4
            //  get:      3  4  5  6 by m3=shuffle(m1, p1, [2 1 3 2])
            F32vec4 vec_m3 = _mm_shuffle_ps(vec_m1, vec_p1, _MM_SHUFFLE(2, 1, 3, 2));        // 3 4 5 6

            F32vec4 vec_max = _mm_max_ps(vec_m1, vec_p0);
            vec_max = _mm_max_ps(vec_max, vec_m2);
            vec_max = _mm_max_ps(vec_max, vec_m3);
            vec_max = _mm_max_ps(vec_max, vec_p1);

            // min
            F32vec4 vec_q1 = _mm_load_ps(pLineMin+x+4);        // 4 5 6 7           
            vec_m2 = _mm_shuffle_ps(vec_q0, vec_q1, _MM_SHUFFLE(1, 0, 3, 2));         // 2 3 4 5
            vec_m1 = _mm_shuffle_ps(vec_q0, vec_m2, _MM_SHUFFLE(2, 1, 2, 1));        // 1 2 3 4
            vec_m3 = _mm_shuffle_ps(vec_m1, vec_q1, _MM_SHUFFLE(2, 1, 3, 2));        // 3 4 5 6

            F32vec4 vec_min = _mm_min_ps(vec_m1, vec_q0);
            vec_min = _mm_min_ps(vec_min, vec_m2);
            vec_min = _mm_min_ps(vec_min, vec_m3);
            vec_min = _mm_min_ps(vec_min, vec_q1);

            // contrast
            vec_max = _mm_sub_ps(vec_max, vec_min);

            // swap
            vec_p0 = vec_p1;
            vec_q0 = vec_q1;

            // store
            _mm_store_ps(pDst+x, vec_max);
        }
		for (; x<iplDstImage->width; x++) {      // remains
            // max
            float maxv = pLineMax[x] > pLineMax[x+1] ? pLineMax[x] : pLineMax[x+1];
            maxv = maxv > pLineMax[x+2] ? maxv : pLineMax[x+2];
            maxv = maxv > pLineMax[x+3] ? maxv : pLineMax[x+3];
            maxv = maxv > pLineMax[x+4] ? maxv : pLineMax[x+4];
            // min
            float minv = pLineMin[x] < pLineMin[x+1] ? pLineMin[x] : pLineMin[x+1];
            minv = minv < pLineMin[x+2] ? minv : pLineMin[x+2];
            minv = minv < pLineMin[x+3] ? minv : pLineMin[x+3];
            minv = minv < pLineMin[x+4] ? minv : pLineMin[x+4];
			pDst[x] = maxv - minv;
		}
	}

    if (pLineMax != NULL) _aligned_free(pLineMax);
    if (pLineMin != NULL) _aligned_free(pLineMin);

	return true;
}

bool CImageUtility::cvtBGRtoYUV_32f_SIMD(IplImage *iplBGR, IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, ColorSpaceName color_sp)
 // SIMD accelerated version (use SSE3 instrinsics)
// Convert BGR planar format to YUV planar format with conversion to 4:2:0/4:2:2/4:4:4
// Arguments: 
//		iplRGB -- [I] RGB image; must be 3-channel 32F image
//		iplPlaneY, iplPlaneU, iplPlaneV -- [O] Y, U, V planes; must be 1-channel 32F image
//		color_sp -- color space (CHROMA_BT709 and CHROMA_YUVMS supported)
// by Luhong Liang, ICD-ASD, ASTRI (Aug., 2014)
// CHROMA_YUVMS implemented by Ms. Shuang Cao (Dec. 2014), reviewed by Luhong Liang
{
	// check input arguments
	if (iplPlaneY == NULL || iplPlaneU == NULL || iplPlaneV == NULL || iplBGR == NULL ||
		iplPlaneY->nChannels != 1 || iplPlaneU->nChannels != 1 || iplPlaneV->nChannels != 1 || iplBGR->nChannels != 3 ||
		iplPlaneY->depth != SR_DEPTH_32F || iplPlaneU->depth != SR_DEPTH_32F || iplPlaneV->depth != SR_DEPTH_32F || iplBGR->depth != SR_DEPTH_32F) {
		showErrMsg("Invalid input image in CImageUtility::cvtBGRtoYUV_32f_SIMD()!\n");
		return false;
	}

	// allocate buffers
	IplImage *iplImageU = NULL;
	IplImage *iplImageV = NULL;
    if (!(iplPlaneY->width == iplBGR->width && iplPlaneY->height == iplBGR->height &&
          iplPlaneU->width == iplBGR->width && iplPlaneU->height == iplBGR->height &&
          iplPlaneV->width == iplBGR->width && iplPlaneV->height == iplBGR->height)) {
	    iplImageU = createImage(iplBGR->width, iplBGR->height, SR_DEPTH_32F, 1);
	    iplImageV = createImage(iplBGR->width, iplBGR->height, SR_DEPTH_32F, 1);
	    if (iplImageU == NULL || iplImageV == NULL) {
		    safeReleaseImage(&iplImageU, &iplImageV);
		    showErrMsg("Fail to allocate buffer in CImageUtility::cvtBGRtoYUV_32f_SIMD()!\n");
            return false;
	    }
    } else {
        iplImageU = iplPlaneU;
        iplImageV = iplPlaneV;
    }

	if (color_sp == CHROMA_BT709) {		// BT 709 for HD
        static const __m128 vec_yr = _mm_set1_ps(0.2126f);
        static const __m128 vec_yg = _mm_set1_ps(0.7152f);
        static const __m128 vec_yb = _mm_set1_ps(0.0722f);
        static const __m128 vec_ub = _mm_set1_ps(1.0f/1.8556f);
        static const __m128 vec_vr = _mm_set1_ps(1.0f/1.5748f);
        static const __m128 vec_const0 = _mm_set1_ps(0.0f);
        static const __m128 vec_const128 = _mm_set1_ps(128.0f);
        static const __m128 vec_const255 = _mm_set1_ps(255.0f);
		for (int y=0; y<iplBGR->height; y++) {
			__declspec(align(64)) float *pY = (__declspec(align(64)) float *)(iplPlaneY->imageData + y * iplPlaneY->widthStep);
			__declspec(align(64)) float *pU = (__declspec(align(64)) float *)(iplImageU->imageData + y * iplImageU->widthStep);
			__declspec(align(64)) float *pV = (__declspec(align(64)) float *)(iplImageV->imageData + y * iplImageV->widthStep);
			__declspec(align(64)) float *pBGR = (__declspec(align(64)) float *)(iplBGR->imageData + y * iplBGR->widthStep);
            int x;
			for (x=0; x<iplBGR->width; x+=4) {
                // load BGR
				int xx = x * 3;
                F32vec4 vec_rgb0 = _mm_load_ps(pBGR+xx);
                F32vec4 vec_rgb1 = _mm_load_ps(pBGR+xx+4);
                F32vec4 vec_rgb2 = _mm_load_ps(pBGR+xx+8);
                // compact to planar
                // RGB:      0  1  2  3  |  4  5  6  7   |  8  9  10 11
                // Planar:   0  3  6  9  |  1  4  7  10 |  2  5  8  11
                F32vec4 vec_tmp0 = _mm_shuffle_ps(vec_rgb1, vec_rgb2, _MM_SHUFFLE(2, 1, 3, 2));  // 6 7 9 10
                F32vec4 vec_tmp1 = _mm_shuffle_ps(vec_rgb0, vec_rgb1, _MM_SHUFFLE(1, 0, 2, 1)); //  1 2 4 5
                F32vec4 vec_b = _mm_shuffle_ps(vec_rgb0, vec_tmp0, _MM_SHUFFLE(2, 0, 3, 0));      // 0 3 6 9
                F32vec4 vec_g = _mm_shuffle_ps(vec_tmp1, vec_tmp0, _MM_SHUFFLE(3, 1, 2, 0));     // 1 4 7 10  
                F32vec4 vec_r = _mm_shuffle_ps(vec_tmp1, vec_rgb2, _MM_SHUFFLE(3, 0, 3, 1));       // 2 5 8 11

                //float Y= 0.2126f * R + 0.7152f * G + 0.0722f * B;
                F32vec4 vec_y = _mm_mul_ps(vec_r, vec_yr);
                vec_tmp0 =  _mm_mul_ps(vec_g, vec_yg);
                vec_tmp1 =  _mm_mul_ps(vec_b, vec_yb);
                vec_y = _mm_add_ps(vec_y, vec_tmp0);
                vec_y = _mm_add_ps(vec_y, vec_tmp1);
                // float U = (B - Y) / 1.8556f + 128.0f; 
                F32vec4 vec_u = _mm_sub_ps(vec_b, vec_y);
                vec_u =  _mm_mul_ps(vec_u, vec_ub);
                vec_u =  _mm_add_ps(vec_u, vec_const128);
				//float V = (R - Y) / 1.5748f + 128.0f;
                F32vec4 vec_v = _mm_sub_ps(vec_r, vec_y);
                vec_v =  _mm_mul_ps(vec_v, vec_vr);
                vec_v =  _mm_add_ps(vec_v, vec_const128);
				//pY[x] = clip_0_255(Y);
                vec_y = _mm_max_ps(vec_y, vec_const0);
                vec_y = _mm_min_ps(vec_y, vec_const255);
				//pU[x] = clip_0_255(U);
                vec_u = _mm_max_ps(vec_u, vec_const0);
                vec_u = _mm_min_ps(vec_u, vec_const255);
				//pV[x] = clip_0_255(V);
                vec_v = _mm_max_ps(vec_v, vec_const0);
                vec_v = _mm_min_ps(vec_v, vec_const255);

                // store
                _mm_store_ps(pY+x, vec_y);
                _mm_store_ps(pU+x, vec_u);
                _mm_store_ps(pV+x, vec_v);
			}
            // remaining
			for (; x<iplBGR->width; x++) {
				// Y = 0.2126 * R + 0.7152 * G + 0.0722 * B;
                // U = (B - Y) / 1.8556 + 128;
                // V = (R - Y) / 1.5748 + 128;
				int xx = x * 3;
				float B = pBGR[xx];
				float G = pBGR[xx+1];
				float R = pBGR[xx+2];
				float Y= 0.2126f * R + 0.7152f * G + 0.0722f * B;
				float U = (B - Y) / 1.8556f + 128.0f; 
				float V = (R - Y) / 1.5748f + 128.0f;
				pY[x] = clip_0_255(Y);
				pU[x] = clip_0_255(U);
				pV[x] = clip_0_255(V);
			}
		}
    } else if (color_sp == CHROMA_YUVMS) { // YUV (microsoft)
        static const __m128 vec_yr = _mm_set1_ps(0.257f);
        static const __m128 vec_yg = _mm_set1_ps(0.504f);
        static const __m128 vec_yb = _mm_set1_ps(0.098f);
        static const __m128 vec_ur = _mm_set1_ps( - 0.148f);
        static const __m128 vec_ug = _mm_set1_ps(0.291f);
        static const __m128 vec_ub = _mm_set1_ps(0.439f);
        static const __m128 vec_vr = _mm_set1_ps(0.439f);
        static const __m128 vec_vg = _mm_set1_ps(0.368f);
        static const __m128 vec_vb = _mm_set1_ps(0.071f);
        static const __m128 vec_const0 = _mm_set1_ps(0.0f);
        static const __m128 vec_const16 = _mm_set1_ps(16.0f); 
        static const __m128 vec_const128 = _mm_set1_ps(128.0f);
        static const __m128 vec_const255 = _mm_set1_ps(255.0f);
        for (int y=0; y<iplBGR->height; y++) {
			__declspec(align(64)) float *pY = (__declspec(align(64)) float *)(iplPlaneY->imageData + y * iplPlaneY->widthStep);
			__declspec(align(64)) float *pU = (__declspec(align(64)) float *)(iplImageU->imageData + y * iplImageU->widthStep);
			__declspec(align(64)) float *pV = (__declspec(align(64)) float *)(iplImageV->imageData + y * iplImageV->widthStep);
			__declspec(align(64)) float *pBGR = (__declspec(align(64)) float *)(iplBGR->imageData + y * iplBGR->widthStep);
            int x;
            for (x=0; x<iplBGR->width; x+=4) {
                // load BGR
				int xx = x * 3;
                F32vec4 vec_rgb0 = _mm_load_ps(pBGR+xx);
                F32vec4 vec_rgb1 = _mm_load_ps(pBGR+xx+4);
                F32vec4 vec_rgb2 = _mm_load_ps(pBGR+xx+8);
                // compact to planar
                // RGB:      0  1  2  3  |  4  5  6  7   |  8  9  10 11
                // Planar:   0  3  6  9  |  1  4  7  10 |  2  5  8  11
                F32vec4 vec_tmp0 = _mm_shuffle_ps(vec_rgb1, vec_rgb2, _MM_SHUFFLE(2, 1, 3, 2));  // 6 7 9 10
                F32vec4 vec_tmp1 = _mm_shuffle_ps(vec_rgb0, vec_rgb1, _MM_SHUFFLE(1, 0, 2, 1)); //  1 2 4 5
                F32vec4 vec_b = _mm_shuffle_ps(vec_rgb0, vec_tmp0, _MM_SHUFFLE(2, 0, 3, 0));      // 0 3 6 9
                F32vec4 vec_g = _mm_shuffle_ps(vec_tmp1, vec_tmp0, _MM_SHUFFLE(3, 1, 2, 0));     // 1 4 7 10  
                F32vec4 vec_r = _mm_shuffle_ps(vec_tmp1, vec_rgb2, _MM_SHUFFLE(3, 0, 3, 1));       // 2 5 8 11
                
                // float Y= 0.257f * R + 0.504f * G + 0.098f * B + 16.0f;
                F32vec4 vec_y = _mm_mul_ps(vec_r, vec_yr);
                vec_tmp0 =  _mm_mul_ps(vec_g, vec_yg);
                vec_tmp1 =  _mm_mul_ps(vec_b, vec_yb);
                vec_y = _mm_add_ps(vec_y, vec_tmp0);
                vec_y = _mm_add_ps(vec_y, vec_tmp1);
                vec_y = _mm_add_ps(vec_y, vec_const16);
                //float U = - 0.148f * R - 0.291f * G + 0.439f * B + 128.0f; 
                F32vec4 vec_u = _mm_mul_ps(vec_r, vec_ur);
                vec_tmp0 = _mm_mul_ps(vec_g, vec_ug);
                vec_tmp1 = _mm_mul_ps(vec_b, vec_ub);
                vec_u = _mm_sub_ps(vec_u, vec_tmp0);
                vec_u = _mm_add_ps(vec_u, vec_tmp1);
                vec_u = _mm_add_ps(vec_u, vec_const128);
                //float V = 0.439 * R - 0.368f * G - 0.071 * B + 128.0f;
                F32vec4 vec_v = _mm_mul_ps(vec_r, vec_vr);
                vec_tmp0 = _mm_mul_ps(vec_g, vec_vg);
                vec_tmp1 = _mm_mul_ps(vec_b, vec_vb);
                vec_v = _mm_sub_ps(vec_v, vec_tmp0);
                vec_v = _mm_sub_ps(vec_v, vec_tmp1);
                vec_v = _mm_add_ps(vec_v, vec_const128);
                //pY[x] = clip_0_255(Y);
                vec_y = _mm_max_ps(vec_y, vec_const0);
                vec_y = _mm_min_ps(vec_y, vec_const255);
				//pU[x] = clip_0_255(U);
                vec_u = _mm_max_ps(vec_u, vec_const0);
                vec_u = _mm_min_ps(vec_u, vec_const255);
				//pV[x] = clip_0_255(V);
                vec_v = _mm_max_ps(vec_v, vec_const0);
                vec_v = _mm_min_ps(vec_v, vec_const255);

                // store
                _mm_store_ps(pY+x, vec_y);
                _mm_store_ps(pU+x, vec_u);
                _mm_store_ps(pV+x, vec_v);
            }
            //remaining
            for (; x<iplBGR->width; x++) {      // bug fixed by Luhong Liang, Dec. 10, 2014
				int xx = x * 3;
				float B = pBGR[xx];
				float G = pBGR[xx+1];
				float R = pBGR[xx+2];
				float Y= 0.257f * R + 0.504f * G + 0.098f * B + 16.0f;
				float U = - 0.148f * R - 0.291f * G + 0.439f * B + 128.0f; 
				float V = 0.439f * R - 0.368f * G - 0.071f * B + 128.0f;
				pY[x] = clip_0_255(Y);
				pU[x] = clip_0_255(U);
				pV[x] = clip_0_255(V);
			}
        }
    } else if (color_sp == CHROMA_YCbCr) {
#ifdef __OPENCV_OLD_CV_H__
        IplImage *iplYCbCr = createImage(iplBGR->width, iplBGR->height, iplBGR->depth, iplBGR->nChannels);
        if (iplYCbCr == NULL) return false;
        cvCvtColor(iplBGR, iplYCbCr, CV_BGR2YCrCb);
        cvSplit(iplYCbCr, iplPlaneY, iplImageU, iplImageV, NULL);
        safeReleaseImage(&iplYCbCr);
#else
		showErrMsg("Does not support YCbCr w/o OpenCV in CImageUtility::cvtBGRtoYUV_32f_SIMD()\n");
		safeReleaseImage(&iplImageV, &iplImageV, &iplImageV);
		return false;
#endif  //  __OPENCV_OLD_CV_H__
    } else if (color_sp == CHROMA_YCbCr) {
#ifdef __OPENCV_OLD_CV_H__
        IplImage *iplYUV = createImage(iplBGR->width, iplBGR->height, iplBGR->depth, iplBGR->nChannels);
        if (iplYUV == NULL) return false;
        cvCvtColor(iplBGR, iplYUV, CV_BGR2YUV);
        cvSplit(iplYUV, iplPlaneY, iplImageU, iplImageV, NULL);
        safeReleaseImage(&iplYUV);
#else
		showErrMsg("Does not support YUV w/o OpenCV in CImageUtility::cvtBGRtoYUV_32f_SIMD()\n");
		safeReleaseImage(&iplImageV, &iplImageV, &iplImageV, NULL);
		return false;
#endif  //  __OPENCV_OLD_CV_H__
	} else {
		showErrMsg("Unsupported color space denoted in CImageUtility::cvtBGRtoYUV_32f_SIMD()\n");
        if (!(iplPlaneY->width == iplBGR->width && iplPlaneY->height == iplBGR->height &&
              iplPlaneU->width == iplBGR->width && iplPlaneU->height == iplBGR->height &&
              iplPlaneV->width == iplBGR->width && iplPlaneV->height == iplBGR->height)) {
            safeReleaseImage(&iplImageU, &iplImageV);
        }
		return false;
	}

	// resize
    if (iplPlaneY->width == iplBGR->width && iplPlaneY->height == iplBGR->height &&
        iplPlaneU->width == iplBGR->width && iplPlaneU->height == iplBGR->height &&
        iplPlaneV->width == iplBGR->width && iplPlaneV->height == iplBGR->height) {
        // 4:4:4 
        // do nothing
    } else if (iplPlaneY->width == iplBGR->width && iplPlaneY->height == iplBGR->height &&
               iplPlaneU->width*2 == iplBGR->width && iplPlaneU->height == iplBGR->height &&
               iplPlaneV->width*2 == iplBGR->width && iplPlaneV->height == iplBGR->height) {
        // 4:2:2
        interlaceX(iplImageU, iplPlaneU, true);
	    interlaceX(iplImageV, iplPlaneV, true);
    } else if (iplPlaneY->width == iplBGR->width && iplPlaneY->height == iplBGR->height &&
               iplPlaneU->width*2 == iplBGR->width && iplPlaneU->height*2 == iplBGR->height &&
               iplPlaneV->width*2 == iplBGR->width && iplPlaneV->height*2 == iplBGR->height) {
        // 4:2:0
	    resize(iplImageU, iplPlaneU, SR_INTER_LINEAR);//SR_INTER_CUBIC);
	    resize(iplImageV, iplPlaneV, SR_INTER_LINEAR);//SR_INTER_CUBIC);
    } else {
        showErrMsg("Only support 4:2:0, 4:2:2 and 4:4:4 format in CImageUtility::cvtBGRtoYUV_32f_SIMD()!\n");
        if (!(iplPlaneY->width == iplBGR->width && iplPlaneY->height == iplBGR->height &&
              iplPlaneU->width == iplBGR->width && iplPlaneU->height == iplBGR->height &&
              iplPlaneV->width == iplBGR->width && iplPlaneV->height == iplBGR->height)) {
            safeReleaseImage(&iplImageU, &iplImageV);
        }
        return false;
    }

    if (!(iplPlaneY->width == iplBGR->width && iplPlaneY->height == iplBGR->height &&
          iplPlaneU->width == iplBGR->width && iplPlaneU->height == iplBGR->height &&
          iplPlaneV->width == iplBGR->width && iplPlaneV->height == iplBGR->height)) {
        safeReleaseImage(&iplImageU, &iplImageV);
    }

	return true;
}

bool CImageUtility::cvtYUVtoBGR_32f_SIMD(IplImage *iplPlaneY, IplImage *iplPlaneU, IplImage *iplPlaneV, IplImage *iplBGR, ColorSpaceName color_sp)
// SIMD accelerated version (use SSE3 instrinsics)
// convert YUV planar format to BGR planar format, where an optional bicubic interpolation is used internally (for resize and/or 4:2:0/4:2:2 to 4:4:4)
// Arguments: 
//		iplPlaneY, iplPlaneU, iplPlaneV -- [I] Y, U, V planes; must be 1-channel 32F image
//		iplBGR -- [0] BGR image; must be 3-channel 32F image
//		color_sp -- color space (only CHROMA_BT709 supported)
// by Luhong Liang, ICD-ASD, ASTRI (Aug, 2014)
// CHROMA_YUVMS implemented by Ms. Shuang Cao (Dec. 2014), reviewed by Luhong Liang
{
	// check input arguments. TODO: check the size, channel and bit depth
	if (iplPlaneY == NULL || iplPlaneU == NULL || iplPlaneV == NULL || iplBGR == NULL ||
		iplPlaneY->nChannels != 1 || iplPlaneU->nChannels != 1 || iplPlaneV->nChannels != 1 || iplBGR->nChannels != 3 ||
		iplPlaneY->depth != SR_DEPTH_32F || iplPlaneU->depth != SR_DEPTH_32F || iplPlaneV->depth != SR_DEPTH_32F || iplBGR->depth != SR_DEPTH_32F) {
		showErrMsg("Invalid input image in CImageUtility::cvtYUVtoBGR_32f_SIMD()!\n");
		return false;
	}

	// allocate 4:4:4 buffers
    IplImage *iplImageU = NULL;
    IplImage *iplImageV = NULL;
    if (!(iplPlaneY->width == iplBGR->width && iplPlaneY->height == iplBGR->height &&
          iplPlaneU->width == iplBGR->width && iplPlaneU->height == iplBGR->height &&
          iplPlaneV->width == iplBGR->width && iplPlaneV->height == iplBGR->height)) {
	    IplImage *iplImageU = createImage(iplBGR->width, iplBGR->height, SR_DEPTH_32F, 1);
	    IplImage *iplImageV = createImage(iplBGR->width, iplBGR->height, SR_DEPTH_32F, 1);
	    if (iplImageU == NULL || iplImageV == NULL) {
            safeReleaseImage(&iplImageU, &iplImageV);
		    showErrMsg("Fail to allocate buffer in CImageUtility::cvtYUVtoBGR_32f_SIMD()!\n");
            return false;
	    }
    } else {
        iplImageU = iplPlaneU;
        iplImageV = iplPlaneV;
    }

	// resize
    if (iplPlaneY->width == iplBGR->width && iplPlaneY->height == iplBGR->height &&
        iplPlaneU->width == iplBGR->width && iplPlaneU->height == iplBGR->height &&
        iplPlaneV->width == iplBGR->width && iplPlaneV->height == iplBGR->height) {
        // 4:4:4
        // do nonthing
    } else if (iplPlaneY->width == iplBGR->width && iplPlaneY->height == iplBGR->height &&
               iplPlaneU->width*2 == iplBGR->width && iplPlaneU->height == iplBGR->height &&
               iplPlaneV->width*2 == iplBGR->width && iplPlaneV->height == iplBGR->height) {
        // 4:2:2
        cvt422to444_linear(iplPlaneU, iplImageU);
	    cvt422to444_linear(iplPlaneV, iplImageV);
    } else if (iplPlaneY->width == iplBGR->width && iplPlaneY->height == iplBGR->height &&
               iplPlaneU->width*2 == iplBGR->width && iplPlaneU->height*2 == iplBGR->height &&
               iplPlaneV->width*2 == iplBGR->width && iplPlaneV->height*2 == iplBGR->height) {
        // 4:2:0
	    resize(iplPlaneU, iplImageU, SR_INTER_LINEAR);//SR_INTER_CUBIC);
	    resize(iplPlaneV, iplImageV, SR_INTER_LINEAR);//SR_INTER_CUBIC);
    } else {
        showErrMsg("Only support 4:2:0, 4:2:2 and 4:4:4 format in CImageUtility::cvtYUVtoBGR_32f_SIMD()!\n");
        if (!(iplPlaneY->width == iplBGR->width && iplPlaneY->height == iplBGR->height &&
              iplPlaneU->width == iplBGR->width && iplPlaneU->height == iplBGR->height &&
              iplPlaneV->width == iplBGR->width && iplPlaneV->height == iplBGR->height)) {
            safeReleaseImage(&iplImageU, &iplImageV);
        }
        return false;
    }

	// color conversion
    if (color_sp == CHROMA_BT709) {		// BT 709 for HD
        // B = 1.8556 .* (U - 128) + Y;
        // R = 1.5748 .* (V - 128) + Y;
        // G = (Y - 0.2126 .* R - 0.0722 .* B) ./ 0.7152;
        static const __m128 vec_by = _mm_set1_ps(1.8556f);
        static const __m128 vec_ry = _mm_set1_ps(1.5748f);
        static const __m128 vec_gr = _mm_set1_ps(0.2126f);
        static const __m128 vec_gb = _mm_set1_ps(0.0722f);
        static const __m128 vec_gden = _mm_set1_ps(1.0f/0.7152f);
        static const __m128 vec_const0 = _mm_set1_ps(0.0f);
        static const __m128 vec_const128 = _mm_set1_ps(128.0f);
        static const __m128 vec_const255 = _mm_set1_ps(255.0f);
        for (int y=0; y<iplBGR->height; y++) {
			__declspec(align(64)) float *pY = (__declspec(align(64)) float *)(iplPlaneY->imageData + y * iplPlaneY->widthStep);
			__declspec(align(64)) float *pU = (__declspec(align(64)) float *)(iplImageU->imageData + y * iplImageU->widthStep);
			__declspec(align(64)) float *pV = (__declspec(align(64)) float *)(iplImageV->imageData + y * iplImageV->widthStep);
			__declspec(align(64)) float *pBGR = (__declspec(align(64)) float *)(iplBGR->imageData + y * iplBGR->widthStep);
            int x;
			for (x=0; x<iplBGR->width; x+=4) {
                // load YUV
                F32vec4 vec_y = _mm_load_ps(pY+x);
                F32vec4 vec_u = _mm_load_ps(pU+x);
                F32vec4 vec_v = _mm_load_ps(pV+x);

                //float B = 1.8556f * (U - 128.0f) + Y;
                F32vec4 vec_b = _mm_sub_ps(vec_u, vec_const128);
                vec_b = _mm_mul_ps(vec_b, vec_by);
                vec_b = _mm_add_ps(vec_b, vec_y);
				//float R = 1.5748f * (V - 128.0f) + Y;
                F32vec4 vec_r = _mm_sub_ps(vec_v, vec_const128);
                vec_r = _mm_mul_ps(vec_r, vec_ry);
                vec_r = _mm_add_ps(vec_r, vec_y);
				//float G = (Y - 0.2126f * R - 0.0722f * B) / 0.7152f;
                F32vec4 vec_tmp0 = _mm_mul_ps(vec_r, vec_gr);
                F32vec4 vec_tmp1 = _mm_mul_ps(vec_b, vec_gb);
                F32vec4 vec_g = _mm_sub_ps(vec_y, vec_tmp0);
                vec_g = _mm_sub_ps(vec_g, vec_tmp1);
                vec_g = _mm_mul_ps(vec_g, vec_gden);

				//pBGR[xx] = clip_0_255(B);
                vec_b = _mm_max_ps(vec_b, vec_const0);
                vec_b = _mm_min_ps(vec_b, vec_const255);
				//pBGR[xx+1] = clip_0_255(G);
                vec_g = _mm_max_ps(vec_g, vec_const0);
                vec_g = _mm_min_ps(vec_g, vec_const255);
				//pBGR[xx+2] = clip_0_255(R);
                vec_r = _mm_max_ps(vec_r, vec_const0);
                vec_r = _mm_min_ps(vec_r, vec_const255);

                // planar to compact
                // Planar:   0  1  2  3  |  4  5  6  7   |  8  9  10 11  ( B | G | R )
                // BGR:      0  4  8  1  |  5  9  2  6   | 10  3  7  11
                vec_tmp0 = _mm_shuffle_ps(vec_b, vec_g, _MM_SHUFFLE(2, 0, 2, 0));       // 0 2 4 6
                vec_tmp1 = _mm_shuffle_ps(vec_g, vec_r, _MM_SHUFFLE(3, 1, 3, 1));       // 5 7 9 11
                F32vec4 vec_tmp2 = _mm_shuffle_ps(vec_b, vec_r, _MM_SHUFFLE(2, 0, 3, 1));       // 1 3 8 10               
                F32vec4 vec_rgb0 = _mm_shuffle_ps(vec_tmp0, vec_tmp2, _MM_SHUFFLE(0, 2, 2, 0));       // 0 4 8 1
                F32vec4 vec_rgb1 = _mm_shuffle_ps(vec_tmp1, vec_tmp0, _MM_SHUFFLE(3, 1, 2, 0));       // 5 9 2 6
                F32vec4 vec_rgb2 = _mm_shuffle_ps(vec_tmp2, vec_tmp1, _MM_SHUFFLE(3, 1, 1, 3));       // 10 3 7 11

                // store
                int xx = x * 3;
                _mm_store_ps(pBGR+xx, vec_rgb0);
                _mm_store_ps(pBGR+xx+4, vec_rgb1);
                _mm_store_ps(pBGR+xx+8, vec_rgb2);
			}
            // remaining
			for (; x<iplBGR->width; x++) {
				// B = 1.8556 .* (U - 128) + Y;
                // R = 1.5748 .* (V - 128) + Y;
                // G = (Y - 0.2126 .* R - 0.0722 .* B) ./ 0.7152;
				float Y = pY[x];
				float U = pU[x];
				float V = pV[x];
                float B = 1.8556f * (U - 128.0f) + Y;
				float R = 1.5748f * (V - 128.0f) + Y;
				float G = (Y - 0.2126f * R - 0.0722f * B) / 0.7152f;
				int xx = x * 3;
				pBGR[xx] = clip_0_255(B);
				pBGR[xx+1] = clip_0_255(G);
				pBGR[xx+2] = clip_0_255(R);
			}
		}
    } else if (color_sp == CHROMA_YUVMS) {    // YUV (microsoft)
        //Yprime = 1.164f * (Y - 16.0f);
        //R = Yprime + 1.596f * (V - 128.0f);
		//G = Yprime - 0.813f * (V - 128.0f) - 0.391f * (U - 128.0f);
		//B = Yprime + 2.018f * (U - 128.0f);
        static const __m128 vec_p = _mm_set1_ps(1.164f);
        static const __m128 vec_rv = _mm_set1_ps(1.596f);
        static const __m128 vec_gv = _mm_set1_ps(0.813f);
        static const __m128 vec_gu = _mm_set1_ps(0.391f);
        static const __m128 vec_bu = _mm_set1_ps(2.018f);
        static const __m128 vec_const0 = _mm_set1_ps(0.0f);
        static const __m128 vec_const16 = _mm_set1_ps(16.0f);
        static const __m128 vec_const128 = _mm_set1_ps(128.0f);
        static const __m128 vec_const255 = _mm_set1_ps(255.0f);
        for (int y=0; y<iplBGR->height; y++) {
			__declspec(align(64)) float *pY = (__declspec(align(64)) float *)(iplPlaneY->imageData + y * iplPlaneY->widthStep);
			__declspec(align(64)) float *pU = (__declspec(align(64)) float *)(iplImageU->imageData + y * iplImageU->widthStep);
			__declspec(align(64)) float *pV = (__declspec(align(64)) float *)(iplImageV->imageData + y * iplImageV->widthStep);
			__declspec(align(64)) float *pBGR = (__declspec(align(64)) float *)(iplBGR->imageData + y * iplBGR->widthStep);
            int x;
			for (x=0; x<iplBGR->width; x+=4) {
                // load YUV
                F32vec4 vec_y = _mm_load_ps(pY+x);
                F32vec4 vec_u = _mm_load_ps(pU+x);
                F32vec4 vec_v = _mm_load_ps(pV+x);
                //Yprime = 1.164f * (Y - 16.0f);
                F32vec4 vec_yp = _mm_sub_ps(vec_y, vec_const16);
                vec_yp = _mm_mul_ps(vec_yp, vec_p);
                //R = Yprime + 1.596f * (V - 128.0f);
                F32vec4 vec_tmp0 = _mm_sub_ps(vec_v, vec_const128);
                vec_tmp0 = _mm_mul_ps(vec_tmp0, vec_rv);
                F32vec4 vec_r = _mm_add_ps(vec_yp, vec_tmp0);
                //G = Yprime - 0.813f * (V - 128.0f) - 0.391f * (U - 128.0f);
                vec_tmp0 = _mm_sub_ps(vec_v, vec_const128);
                vec_tmp0 = _mm_mul_ps(vec_tmp0, vec_gv);
                F32vec4 vec_tmp1 = _mm_sub_ps(vec_u, vec_const128);
                vec_tmp1 = _mm_mul_ps(vec_tmp1, vec_gu);
                F32vec4 vec_g = _mm_sub_ps(vec_yp, vec_tmp0);
                vec_g = _mm_sub_ps (vec_g, vec_tmp1);
                //B = Yprime + 2.018f * (U - 128.0f);
                vec_tmp0 = _mm_sub_ps(vec_u, vec_const128);
                vec_tmp0 = _mm_mul_ps(vec_tmp0, vec_bu);
                F32vec4 vec_b = _mm_add_ps(vec_yp, vec_tmp0);

                //pBGR[xx] = clip_0_255(B);
                vec_b = _mm_max_ps(vec_b, vec_const0);
                vec_b = _mm_min_ps(vec_b, vec_const255);
				//pBGR[xx+1] = clip_0_255(G);
                vec_g = _mm_max_ps(vec_g, vec_const0);
                vec_g = _mm_min_ps(vec_g, vec_const255);
				//pBGR[xx+2] = clip_0_255(R);
                vec_r = _mm_max_ps(vec_r, vec_const0);
                vec_r = _mm_min_ps(vec_r, vec_const255);

                // planar to compact
                // Planar:   0  1  2  3  |  4  5  6  7   |  8  9  10 11  ( B | G | R )
                // BGR:      0  4  8  1  |  5  9  2  6   | 10  3  7  11
                vec_tmp0 = _mm_shuffle_ps(vec_b, vec_g, _MM_SHUFFLE(2, 0, 2, 0));       // 0 2 4 6
                vec_tmp1 = _mm_shuffle_ps(vec_g, vec_r, _MM_SHUFFLE(3, 1, 3, 1));       // 5 7 9 11
                F32vec4 vec_tmp2 = _mm_shuffle_ps(vec_b, vec_r, _MM_SHUFFLE(2, 0, 3, 1));       // 1 3 8 10               
                F32vec4 vec_rgb0 = _mm_shuffle_ps(vec_tmp0, vec_tmp2, _MM_SHUFFLE(0, 2, 2, 0));       // 0 4 8 1
                F32vec4 vec_rgb1 = _mm_shuffle_ps(vec_tmp1, vec_tmp0, _MM_SHUFFLE(3, 1, 2, 0));       // 5 9 2 6
                F32vec4 vec_rgb2 = _mm_shuffle_ps(vec_tmp2, vec_tmp1, _MM_SHUFFLE(3, 1, 1, 3));       // 10 3 7 11

                // store
                int xx = x * 3;
                _mm_store_ps(pBGR+xx, vec_rgb0);
                _mm_store_ps(pBGR+xx+4, vec_rgb1);
                _mm_store_ps(pBGR+xx+8, vec_rgb2);
            }    
                //remaining 
            for (; x<iplBGR->width; x++) {
				float Y = pY[x];
				float U = pU[x];
				float V = pV[x];
                float Yprime = 1.164f * (Y - 16.0f);
                float R = Yprime + 1.596f * (V - 128.0f);
				float G = Yprime - 0.813f * (V - 128.0f) - 0.391f * (U - 128.0f);
				float B = Yprime + 2.018f * (U - 128.0f);
				int xx = x * 3;
				pBGR[xx] = clip_0_255(B);
				pBGR[xx+1] = clip_0_255(G);
				pBGR[xx+2] = clip_0_255(R);
			}
        }
    } else if (color_sp == CHROMA_YCbCr) {
#ifdef __OPENCV_OLD_CV_H__
        IplImage *iplYuv = createImage(iplBGR->width, iplBGR->height, iplBGR->depth, iplBGR->nChannels);
        if (iplYuv == NULL) return false;
        cvMerge(iplPlaneY, iplImageU, iplImageV, NULL, iplYuv);
        cvCvtColor(iplYuv, iplBGR, CV_YCrCb2BGR);
        safeReleaseImage(&iplYuv);
#else
		showErrMsg("Does not support YCbCr w/o OpenCV in CImageUtility::cvtYUVtoBGR_32f_SIMD()\n");
		safeReleaseImage(&iplImageV, &iplImageV, &iplImageV);
		return false;
#endif  //  __OPENCV_OLD_CV_H__
    } else if (color_sp == CHROMA_YCbCr) {
#ifdef __OPENCV_OLD_CV_H__
        IplImage *iplYuv = createImage(iplBGR->width, iplBGR->height, iplBGR->depth, iplBGR->nChannels);
        if (iplYuv == NULL) return false;
        cvMerge(iplPlaneY, iplImageU, iplImageV, NULL, iplYuv);
        cvCvtColor(iplYuv, iplBGR, CV_YUV2BGR);
        safeReleaseImage(&iplYuv);
#else
		showErrMsg("Does not support YUV w/o OpenCV in CImageUtility::cvtYUVtoBGR_32f_SIMD()\n");
		safeReleaseImage(&iplImageV, &iplImageV, &iplImageV, NULL);
		return false;
#endif  //  __OPENCV_OLD_CV_H__
	} else {
		showErrMsg("Unsupported color space denoted in CImageUtility::cvtYUVtoBGR_32f_SIMD()\n");
        if (!(iplPlaneY->width == iplBGR->width && iplPlaneY->height == iplBGR->height &&
              iplPlaneU->width == iplBGR->width && iplPlaneU->height == iplBGR->height &&
              iplPlaneV->width == iplBGR->width && iplPlaneV->height == iplBGR->height)) {
            safeReleaseImage(&iplImageU, &iplImageV);
        }
		return false;
	}

    if (!(iplPlaneY->width == iplBGR->width && iplPlaneY->height == iplBGR->height &&
          iplPlaneU->width == iplBGR->width && iplPlaneU->height == iplBGR->height &&
          iplPlaneV->width == iplBGR->width && iplPlaneV->height == iplBGR->height)) {
        safeReleaseImage(&iplImageU, &iplImageV);
    }

	return true;
}

bool CImageUtility::adaNLMFilter11x5_32f_SIMD(IplImage *iplSrcImage, IplImage *iplDstImage, 
	                                          float sensitivity, float hvsimpact, float adaptfactor, float sigma)
// SIMD accelerated version (use SSE3 instrinsics)
// NOTE: since there mayeb some error beteen the SIMD and the SISD instrinsics, the result of this function maybe a little bit different to
//            the original one!
// Adaptive non-local mean filter with 5x5 patch and 11x11 search window.
// This function uses HVS model and edge detector to adaptively adjust the strength of the denoise
// This is a broute force implementation.
// This function supports "in-place" operation. The input and output image should be the same size and type.
// Arguments:
//      iplSrcImage -- [I] input image
//      iplDstImage -- [O] output image; same size and type as the input image
//      sensitivity -- [I] sensitivity to the edges; 0~1, larger value, stronger smoothing on edges and details
//      hvsimpact -- [I] impact factor of the HVS model. When hvsimpact == 0, the texture map is netrual (0.5), i.e. disabled.
//                       When hvsimpact = 0.5, original texture map is unchaged.
//                       When hvsimpact increases to 1, the texture map is evenly close to "texture" (1.0).
//                       When hvsimpact decreases to -1, the texture map is evenly close to "structure" (0.0).
//                       Key value of hvsimpact:    -1.0    -0.5        0.0     0.5     1.0
//                       Resutant HVS value:         0.0    0.5*hvs     0.5     hvs     1.0
//      adaptfactor -- [I] to control the adaptive. When adaptfactor == 0, the actual sigma is the constant original one denoted by parameter 'sigma'.
//                         When adaptfactor == 1, the actual 'sigma' is tuned by 'hvsimpact' and "adaptfactor', and between 'sigma' and 'sensitivity*sigma'.
//      sigma -- [I] sigma in the filter, which controls the weight of each patch
// by Luhong Liang, September, 2014
// May 4, 2015: add parameter 'adaptfactor' to make the SIMD implementation identical to the non-SIMD one
// May 7, 2015: add support on zero sigma
// Make'sigma' identical to original method, by Luhong, 10/05/2015
{
	if (iplSrcImage == NULL || iplDstImage == NULL || iplSrcImage->width != iplDstImage->width || iplSrcImage->height != iplDstImage->height ||
		iplSrcImage->nChannels != iplDstImage->nChannels || iplSrcImage->nChannels != 1 || sigma <  0.0f || sensitivity < 0.0f || sensitivity > 1.0f) {
		showErrMsg("Invalide input image or parameter in CImageUtility::adaNLMFilter11x5_32f_SIMD()!\n");
		return false;
	}

    showMessage1("Adaptive non-local mean filtering ...");

	if (sigma <= 0.0f) {
		copy(iplSrcImage, iplDstImage);
		return true;
	}

	__declspec(align(64)) float *diff_total = (__declspec(align(64)) float *)_aligned_malloc(256, 64);
	if (diff_total == NULL) {
		showErrMsg("Fail to allocate aligned buffer in CImageUtility::adaNLMFilter11x5_32f_SIMD()!\n");
		return false;
	}

    // padding
    IplImage *iplPadded = padding(iplSrcImage, 7, 7, 7, 7);
    IplImage *iplSharp = createImage(iplDstImage);
    IplImage *iplHVS = createImage(iplDstImage);
    IplImage *iplLuma = createImage(iplDstImage);
    if (iplPadded == NULL || iplSharp == NULL || iplHVS == NULL || iplLuma == NULL) {
        safeReleaseImage(&iplPadded, &iplSharp, &iplHVS, &iplLuma);
        return false;
    }

    /// LUT of range filter
    const int exp_lut_max = 128;        // must long engouth to avoid artifact in high contrast text content!
    const int exp_lut_amp = 8;
    const int lut_len = exp_lut_max * exp_lut_amp;
	const float lut_max_idx = (float)(lut_len - 1); 
    float pLUT[lut_len];
    for (int i=0; i< lut_len; i++) {
        pLUT[i] = exp( - (float)i / (float)exp_lut_amp);
    }

	//
	// Calculate edge sharpness (mixture of LoG and MAD)
	//

    // calculate LoG filter
    const float log_sigma = 2.5f;
    const int log_wnd_size = (int)(ceil(log_sigma * 2.5) * 2 + 1.0f);   // wnd_size=15
    filtLoG_32f(iplSrcImage, iplSharp, log_wnd_size, log_sigma, 3);
    //saveImage("_LoG4x32.bmp", iplSharp, 0, 32.0f);

    // calculate local gradient
    if (!extrMADMap5x5_o1_32f(iplSrcImage, iplHVS)) {
        safeReleaseImage(&iplPadded, &iplSharp, &iplHVS, &iplLuma);
        return false;
    }
    //saveImage("_MADx8.bmp", iplHVS, 0, 8.0f);

    // calculate luma
    boxing5x5_32f(iplSrcImage, iplLuma);

    // fusion MAD and LoG
    //saveImage("_Org.bmp", iplSrcImage);
    for (int y=0; y<iplSharp->height; y++) {
        float *pSharp = (float *)(iplSharp->imageData + y * iplSharp->widthStep);
        float *pMAD = (float *)(iplHVS->imageData + y * iplHVS->widthStep);
        float *pLuma = (float *)(iplLuma->imageData + y * iplLuma->widthStep);
        for (int x=0; x<iplSharp->width; x++) {
            float sharp = pSharp[x];
            float mad = pMAD[x];
            float luma = pLuma[x];
            // normalize
            luma = luma < 32.0f ? 32.0f : luma; 
            sharp = sharp * 32.0f / luma;
            mad = mad * 8.0f / luma;
            //pMAD[x] = mad;
            //pSharp[x] = sharp;
            // fusion
            sharp = sharp < mad ? sharp : mad;
            pSharp[x] = sharp;
        }
    }   
    //saveImage("_SharpnessLoGMAD_simd.bmp", iplSharp, 0, 255.0f);

    // calculate adaptive weight map
    float log_weight_low = sensitivity;
    float log_weight_high = sensitivity + 0.25f;
    for (int y=0; y<iplSharp->height; y++) {
        float *pSharp = (float *)(iplSharp->imageData + y * iplSharp->widthStep);
        for (int x=0; x<iplSharp->width; x++) {
            // calculate sigma map
            float sharp = pSharp[x];
            sharp = sharp > log_weight_high ? log_weight_high : sharp;
            sharp = sharp < log_weight_low ? log_weight_low : sharp;
            sharp = (sharp - log_weight_low) * 4.0f;
            pSharp[x] = sharp;
        }
    }
    //saveImage("_SharpnessTunedBySensitivity_simd.bmp", iplSharp, 0, 255.0f);

    // morphology process
    const float bulb_th_low = 0.0625f;
    const int bnd_num = 1;
    //rmIsoBulb9x9_32f(iplSharp, iplSharp, bulb_th_low, bnd_num);
    rmIsoBulb11x11_32f(iplSharp, iplSharp, bulb_th_low, bnd_num);
    //rmIsoBulb13x13_32f(iplSharp, iplSharp, bulb_th_low, bnd_num);
    //saveImage("_SharpnessPostProc_simd.bmp", iplSharp, 0, 255.0f);

	//
    // calculate regularity and weight
	//
    if (adaptfactor != 0.0f) {
        if (hvsimpact != 0.0f) {
            extrStructMap5x5_32f(iplSrcImage, iplHVS, 1);
            max5x5_32f(iplHVS, iplHVS);
			//saveImage("_HVSMax_simd.bmp", iplHVS, 0, 255.0f);
            float tex_lik_low = 0.0f;
            float tex_lik_high = 0.5f;
            for (int y=0; y<iplHVS->height; y++) {
                float *pHVS = (float *)(iplHVS->imageData + y * iplHVS->widthStep);
                for (int x=0; x<iplHVS->width; x++) {
                    // calculate sigma map
                    float tex_lik = 1.0f - pHVS[x];
                    tex_lik = tex_lik < tex_lik_low ? tex_lik_low : tex_lik;
                    tex_lik = tex_lik > tex_lik_high ? tex_lik_high : tex_lik;
                    pHVS[x] = (tex_lik - tex_lik_low) / (tex_lik_high - tex_lik_low);
                }
            }
            //saveImage("_HVSWeight1.bmp", iplHVS, 0, 255.0f);
            // tuning HVS
            float w1 = (hvsimpact + 1.0f);
            float w2 = 0.5f * (1.0f - (- 2.0f * hvsimpact));
            float w3 = (2.0f * hvsimpact);
            float w4 = 0.5f * (1.0f - (2.0f * hvsimpact));
            float w5 = (2.0f * (1.0f - hvsimpact));
            float w6 = 1.0f - w5;
            for (int y=0; y<iplSharp->height; y++) {
                float *pHVS = (float *)(iplHVS->imageData + y * iplHVS->widthStep);
                for (int x=0; x<iplSharp->width; x++) {
                    float hvs = pHVS[x];
                    if (hvsimpact < -0.5f) {
                        hvs = hvs * w1;               //hvs * 0.5f * (hvsimpact + 1.0f) * 2.0f;
                    } else if (hvsimpact < 0.0f) {
                        hvs = - hvs * hvsimpact + w2; //(hvs * 0.5f) * (- 2.0f * hvsimpact) + 0.5f * (1.0f - (- 2.0f * hvsimpact));
                    } else if (hvsimpact < 0.5f) {
                        hvs = hvs * w3 + w4;        // hvs * (2.0f * hvsimpact) + 0.5f * (1.0f - (2.0f * hvsimpact));
                    } else {
                        hvs = hvs * w5 + w6;        //hvs * (2.0f * (1.0f - hvsimpact)) + 1.0f * (1.0f - (2.0f * (1.0f - hvsimpact)))
                    }
                    pHVS[x] = hvs;
                }
            }
			//saveImage("_HVSTuned_simd.bmp", iplHVS, 0, 255.0f);

            // fusion with HVS
            for (int y=0; y<iplSharp->height; y++) {
                float *pWsigma = (float *)(iplSharp->imageData + y * iplSharp->widthStep);
                float *pHVS = (float *)(iplHVS->imageData + y * iplHVS->widthStep);
                for (int x=0; x<iplSharp->width; x++) {
                    // calculate HVS term
                    float hvs = pHVS[x];
                    // min
                    float sharpness = pWsigma[x];
                    pWsigma[x] = sharpness < hvs ? sharpness : hvs;
                }
            }
            //saveImage("_SigmaWeight_HVS.bmp", iplSharp, 0, 255.0f);
        }
        // adaptive
        for (int y=0; y<iplSharp->height; y++) {
            float *pWsigma = (float *)(iplSharp->imageData + y * iplSharp->widthStep);
            for (int x=0; x<iplSharp->width; x++) {
                pWsigma[x] = adaptfactor * pWsigma[x] +  (1.0f - adaptfactor);
            }
        }
    } else {
        for (int y=0; y<iplSharp->height; y++) {
            float *pWsigma = (float *)(iplSharp->imageData + y * iplSharp->widthStep);
            for (int x=0; x<iplSharp->width; x++) {
                pWsigma[x] = 1.0f;
            }
        }
    }
	//saveImage("_SigmaWeight_simd.bmp", iplSharp, 0, 255.0f);

    // smooth sigma weight map
    gaussian5x5_32f(iplSharp, iplSharp, 2.0f);
    //saveImage("_SigmaWeight_HVS_Smooth.bmp", iplSharp, 0, 255.0f);

    // calculate sigma map
	float min_sigma = sensitivity * sigma;
    const float sigma_eps = 0.01f;
    for (int y=0; y<iplSharp->height; y++) {
        float *pWsigma = (float *)(iplSharp->imageData + y * iplSharp->widthStep);
        for (int x=0; x<iplSharp->width; x++) {
            float w_sigma = pWsigma[x];
            float local_sigma = (sigma - min_sigma) * (1.0f - w_sigma) + min_sigma;
            pWsigma[x] = local_sigma + sigma_eps;
        }
    }
    //saveImage("_SigmaMap_simd.bmp", iplSharp, 0, 255.0f/sigma);

    float fStartTime = (float)clock() / (float)CLOCKS_PER_SEC;
    // filtering
	//const float eps = 0.00001f;
    // filtering
    float query0, query1, query2, query3, query4;
    //__declspec(align(64)) float pPatch[32];
    for (int y=0; y<iplDstImage->height; y++) {
        __declspec(align(64)) float *pDst = (__declspec(align(64)) float *)(iplDstImage->imageData + y * iplDstImage->widthStep);
        __declspec(align(64)) float *pSigma = (__declspec(align(64)) float *)(iplSharp->imageData + y * iplSharp->widthStep);
		__declspec(align(64)) float *pQLine0 = (__declspec(align(64)) float *)(iplPadded->imageData + (y+5) * iplPadded->widthStep);
		__declspec(align(64)) float *pQLine1 = (__declspec(align(64)) float *)(iplPadded->imageData + (y+6) * iplPadded->widthStep);
		__declspec(align(64)) float *pQLine2 = (__declspec(align(64)) float *)(iplPadded->imageData + (y+7) * iplPadded->widthStep);
		__declspec(align(64)) float *pQLine3 = (__declspec(align(64)) float *)(iplPadded->imageData + (y+8) * iplPadded->widthStep);
		__declspec(align(64)) float *pQLine4 = (__declspec(align(64)) float *)(iplPadded->imageData + (y+9) * iplPadded->widthStep);
		if (y%128==0) showMessage1("%d ", y);
        for (int x=0; x<iplDstImage->width; x++) {
			//if (x==845 && y == 127) 
			//	int p = 1;
			//printf("x=%d   y=%d\n", x, y);
			// fallback pixel
			float query_centre = pQLine2[x+7]; 

            // get local sigma
            float local_sigma = pSigma[x];
			if (local_sigma <= 0.0f) {		// May 7, 2015
				pDst[x] = query_centre;
				continue;
			}
            float sigma_denom = 1.0f / (local_sigma * local_sigma);

			// get query patch
            int xx = x + 5;
            F32vec4 vec_query0 = _mm_loadu_ps(pQLine0+xx);   // 0 1 2 3
            F32vec4 vec_query1 = _mm_loadu_ps(pQLine1+xx);   // 0 1 2 3
            F32vec4 vec_query2 = _mm_loadu_ps(pQLine2+xx);   // 0 1 2 3
            F32vec4 vec_query3 = _mm_loadu_ps(pQLine3+xx);   // 0 1 2 3
            F32vec4 vec_query4 = _mm_loadu_ps(pQLine4+xx);   // 0 1 2 3
            //float query_centre = pQLine2[x+7]; 
            query0 = pQLine0[x+9]; 
            query1 = pQLine1[x+9]; 
            query2 = pQLine2[x+9]; 
            query3 = pQLine3[x+9]; 
            query4 = pQLine4[x+9]; 

#ifdef _DEBUG
            //if (x == 10 && y ==10) 
            //    int p = 1;
#endif
            // search 11x11 window
            float w_max = 0.0f;
            float sweight = 0.0f;
            float average = 0.0f;
            for (int yy=y; yy<y+11; yy++) {
				//printf("yy=%d\n", yy);
                __declspec(align(64)) float *pLine0 = (__declspec(align(64)) float *)(iplPadded->imageData + yy * iplPadded->widthStep);
                __declspec(align(64)) float *pLine1 = (__declspec(align(64)) float *)(iplPadded->imageData + (yy+1) * iplPadded->widthStep);
                __declspec(align(64)) float *pLine2 = (__declspec(align(64)) float *)(iplPadded->imageData + (yy+2) * iplPadded->widthStep);
                __declspec(align(64)) float *pLine3 = (__declspec(align(64)) float *)(iplPadded->imageData + (yy+3) * iplPadded->widthStep);
                __declspec(align(64)) float *pLine4 = (__declspec(align(64)) float *)(iplPadded->imageData + (yy+4) * iplPadded->widthStep);
                //if ((yy < y+5 || yy > y+7) && (yy-y)%2==1) continue;
				//__declspec(align(64)) float diff_total[64], diff;
				float diff;
                for (int xx=x; xx<x+11; xx++) {
					//printf("%d ", xx);
                    //if ((xx < x+5 || xx > x+7) && (xx-x)%2==1) continue;
                    if (xx==x+5 && yy==y+5) continue;
                    // get partial patch
                    F32vec4 vec_patch0 = _mm_loadu_ps(pLine0+xx);   // 0 1 2 3
                    F32vec4 vec_patch1 = _mm_loadu_ps(pLine1+xx);   // 0 1 2 3
                    F32vec4 vec_patch2 = _mm_loadu_ps(pLine2+xx);   // 0 1 2 3
                    F32vec4 vec_patch3 = _mm_loadu_ps(pLine3+xx);   // 0 1 2 3
                    F32vec4 vec_patch4 = _mm_loadu_ps(pLine4+xx);   // 0 1 2 3
                    // calculate partial squred difference
                    F32vec4 vec_diff = _mm_sub_ps(vec_query0, vec_patch0);
                    F32vec4 vec_total_diff = _mm_mul_ps(vec_diff, vec_diff);
                    vec_diff = _mm_sub_ps(vec_query1, vec_patch1);
                    vec_diff = _mm_mul_ps(vec_diff, vec_diff);
                    vec_total_diff = _mm_add_ps(vec_total_diff, vec_diff);
                    vec_diff = _mm_sub_ps(vec_query2, vec_patch2);
                    vec_diff = _mm_mul_ps(vec_diff, vec_diff);
                    vec_total_diff = _mm_add_ps(vec_total_diff, vec_diff);
                    vec_diff = _mm_sub_ps(vec_query3, vec_patch3);
                    vec_diff = _mm_mul_ps(vec_diff, vec_diff);
                    vec_total_diff = _mm_add_ps(vec_total_diff, vec_diff);
                    vec_diff = _mm_sub_ps(vec_query4, vec_patch4);
                    vec_diff = _mm_mul_ps(vec_diff, vec_diff);
                    vec_total_diff = _mm_add_ps(vec_total_diff, vec_diff);
                    // _mm_hadd_ps --- a0 + a1; a2 + a3; b0 + b1; b2 + b3;
                    vec_total_diff = _mm_hadd_ps(vec_total_diff, vec_total_diff);
                    vec_total_diff = _mm_hadd_ps(vec_total_diff, vec_total_diff);
                    //_mm_store_ss(diff_total, vec_total_diff);
					_mm_store_ss(diff_total, vec_total_diff);
                    // match others
                    diff = query0 - pLine0[xx+4]; diff = diff * diff; *diff_total += diff;
                    diff = query1 - pLine1[xx+4]; diff = diff * diff; *diff_total += diff;
                    diff = query2 - pLine2[xx+4]; diff = diff * diff; *diff_total += diff;
                    diff = query3 - pLine3[xx+4]; diff = diff * diff; *diff_total += diff;
                    diff = query4 - pLine4[xx+4]; diff = diff * diff; *diff_total += diff;
                    // calculate weight
                    float idxf = (*diff_total) * sigma_denom * exp_lut_amp + 0.5f;
					idxf = idxf > lut_max_idx ? lut_max_idx : idxf;
                    int idx = (int)idxf;
					// DON'T change the above 3 lines of code, otherwise the program may crash due to VS's bug?
                    float w = pLUT[idx];
                    w_max = w > w_max ? w : w_max;
                    // accumulate
                    sweight += w;
                    average += w * pLine2[xx+2];
                }
            }
            // accumulate the central pont
            sweight += w_max;
            average += w_max * query_centre;
            // pixel value
            if (sweight > 0.0f) {
                pDst[x] = average / sweight;
            } else {
                pDst[x] = query_centre;
            }
        }
    }
    float fRunTime = (float)clock() / (float)CLOCKS_PER_SEC - fStartTime;
    showMessage1("%f s used in NL search.\n", fRunTime);

    safeReleaseImage(&iplPadded, &iplSharp, &iplHVS, &iplLuma);
	if (diff_total != NULL) _aligned_free(diff_total);

    return true;
}

#endif

