//
// Fast Lanczos interpolation/resize algorithms (SIMD implementations)
// Developed by Shuang Cao, IC-ASD, ASTRI
// Reviewed by Luhong Liang, IC-ASD, ASTRI
// Feb 2, 2015
//

//
// Function List
//


//
// Functions
//

#ifdef __SR_USE_SIMD


bool CImageUtility::resizeLanczos2_4to3_32f_SIMD(IplImage *iplSrc, IplImage *iplDst)
 // Resizes an input image using Lanczos filter.
 // This function is an extension of Matlab buid-in function imresize(...,
 // ..., lanczos3).
 // arguments:
 //			iplSrc -- [I] Input image
 //        iplDst -- [O] Resized image
 //by Cao Shuang
 // Jan, 2015
 // Reviewed and modified by Luhong (9/2/2015)
 {  
    // parse arguments
    if (iplSrc == NULL ||  iplSrc->nChannels != 1 || iplDst == NULL ||  iplDst->nChannels != 1 ||
        iplSrc->depth != SR_DEPTH_32F || iplDst->depth != SR_DEPTH_32F || 
        iplSrc->width <= 0 || iplSrc->height <= 0 || iplDst->width <= 0 || iplDst->height <= 0) {
        showErrMsg( "Invalid inputs in resizeLanczos2_4to3_32f_SIMD()!\n");
        return false;
    }
    if (iplSrc->width*3 != iplDst->width*4 || iplSrc->height*3 != iplDst->height*4) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos2_4to3_32f_SIMD()!\n");
        return false;
    }

    IplImage *iplSrcPad = padding(iplSrc, 2, 2, 2, 2);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos2_4to3_32f_SIMD()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }

    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 3*iplSrc->height/4,  iplSrc->depth, iplSrc->nChannels);
    if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos2_4to3_32f_SIMD()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }

    static const __m128 vec_p0 = _mm_set1_ps(-0.029300f);
    static const __m128 vec_p1 = _mm_set1_ps(0.073894f);
    static const __m128 vec_p2 = _mm_set1_ps(0.720225f);
    static const __m128 vec_p3 = _mm_set1_ps(0.296425f);
    static const __m128 vec_p4 = _mm_set1_ps(-0.061245f);
    static const __m128 vec_p5 = _mm_set1_ps(-0.003186f);
    static const __m128 vec_p6 = _mm_set1_ps(-0.044499f);
    static const __m128 vec_p7 = _mm_set1_ps(0.547685f);
    static const __m128 vec_const0 = _mm_set1_ps(0.0f);
    static const __m128 vec_const255 = _mm_set1_ps(255.0f);
    for (int y = 0, yy = 0; y < iplIy->height; y+=3, yy+=4){
        __declspec(align(64)) float *pIy0 = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
        __declspec(align(64)) float *pIy1 = (__declspec(align(64)) float *)(iplIy->imageData + (y+1) * iplIy->widthStep);
        __declspec(align(64)) float *pIy2 = (__declspec(align(64)) float *)(iplIy->imageData + (y+2) * iplIy->widthStep);
        __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc2 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc3 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc4 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc5 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+5) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc6 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+6) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc7 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+7) * iplSrcPad->widthStep);

       // Iy(y,:,) = weight * Strip(:,:,); 
        int x;
        for ( x = 0; x < iplSrcPad->width; x+=4) {
            //pIy0[x] = - 0.029300f * pSrc0[x] + 0.073894f * pSrc1[x] + 0.720225f * pSrc2[x] + 0.296425f * pSrc3[x] - 0.061245f * pSrc4[x];
            F32vec4 vec_ps0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_ps1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_ps2 = _mm_load_ps(pSrc2+x);
            F32vec4 vec_ps3 = _mm_load_ps(pSrc3+x);
            F32vec4 vec_ps4 = _mm_load_ps(pSrc4+x);
            F32vec4 vec_piy0 = _mm_mul_ps(vec_p0, vec_ps0);
            F32vec4 vec_tmp0 = _mm_mul_ps(vec_p1, vec_ps1);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_ps2);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_ps3);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_ps4);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            //pIy1[x] = - 0.003186f * pSrc1[x] - 0.044499f * pSrc2[x] + 0.547685f * pSrc3[x] + 0.547685f * pSrc4[x] - 0.044499f * pSrc5[x] - 0.003186f * pSrc6[x];
            F32vec4 vec_ps5 = _mm_load_ps(pSrc5+x);
            F32vec4 vec_ps6 = _mm_load_ps(pSrc6+x);
            F32vec4 vec_piy1 = _mm_mul_ps(vec_p5, vec_ps1);
            F32vec4 vec_tmp1 = _mm_mul_ps(vec_p6, vec_ps2);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p7, vec_ps3);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p7, vec_ps4);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p6, vec_ps5);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p5, vec_ps6);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            //pIy2[x] = - 0.061245f * pSrc3[x] + 0.296425f * pSrc4[x] + 0.720225f * pSrc5[x] + 0.073894f * pSrc6[x] - 0.029300f * pSrc7[x];
            F32vec4 vec_ps7 = _mm_load_ps(pSrc7+x);
            F32vec4 vec_piy2 = _mm_mul_ps(vec_p4, vec_ps3);
            F32vec4 vec_tmp2 = _mm_mul_ps(vec_p3, vec_ps4);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p2, vec_ps5);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p1, vec_ps6);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p0, vec_ps7);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            //write back
            _mm_store_ps(pIy0+x, vec_piy0);
            _mm_store_ps(pIy1+x, vec_piy1);
            _mm_store_ps(pIy2+x, vec_piy2);

        }
        for (; x < iplSrcPad->width; x++) { //remaining pixels
            pIy0[x] = - 0.029300f * pSrc0[x] + 0.073894f * pSrc1[x] + 0.720225f * pSrc2[x] + 0.296425f * pSrc3[x] - 0.061245f * pSrc4[x];                 
            pIy1[x] = - 0.003186f * pSrc1[x] - 0.044499f * pSrc2[x] + 0.547685f * pSrc3[x] + 0.547685f * pSrc4[x] - 0.044499f * pSrc5[x] - 0.003186f * pSrc6[x];
            pIy2[x] = - 0.061245f * pSrc3[x] + 0.296425f * pSrc4[x] + 0.720225f * pSrc5[x] + 0.073894f * pSrc6[x] - 0.029300f * pSrc7[x];
        }
    }
    safeReleaseImage(&iplSrcPad);


    // x direction
    for (int y = 0; y < iplDst->height; y++) {       
        __declspec(align(64)) float *pIy = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
        __declspec(align(64)) float *pY = (__declspec(align(64)) float *)(iplDst->imageData + y * iplDst->widthStep);          
        //Y(:,x) = Strip(:,:) * weight_x;
        int x, xx;
        F32vec4 vec_piy0 = _mm_load_ps(pIy);   // 0 1 2 3
        for (x = 0, xx = 0; x < iplDst->width; x+=12, xx+=16){  //column           
            F32vec4 vec_piy1 = _mm_load_ps(pIy+xx+4); // 4 5 6 7
            F32vec4 vec_piy2 = _mm_load_ps(pIy+xx+8); // 8 9 10 11
            F32vec4 vec_piy3 = _mm_load_ps(pIy+xx+12);// 12 13 14 15
            F32vec4 vec_piy4 = _mm_load_ps(pIy+xx+16);// 16 17 18 19
            F32vec4 vec_tmp0 = _mm_shuffle_ps(vec_piy0, vec_piy1, _MM_SHUFFLE(1, 0, 1, 0)); // 0 1 4 5
            F32vec4 vec_tmp1 = _mm_shuffle_ps(vec_piy2, vec_piy3, _MM_SHUFFLE(1, 0, 1, 0)); // 8 9 12 13
            F32vec4 vec_iy0 = _mm_shuffle_ps(vec_tmp0, vec_tmp1, _MM_SHUFFLE(2, 0, 2, 0));  // 0 4 8 12
            F32vec4 vec_iy1 = _mm_shuffle_ps(vec_tmp0, vec_tmp1, _MM_SHUFFLE(3, 1, 3, 1));  // 1 5 9 13
            vec_tmp0 = _mm_shuffle_ps(vec_piy0, vec_piy1, _MM_SHUFFLE(3, 2, 3, 2)); // 2 3 6 7
            vec_tmp1 = _mm_shuffle_ps(vec_piy2 ,vec_piy3, _MM_SHUFFLE(3, 2, 3, 2)); // 10 11 14 15
            F32vec4 vec_iy2 = _mm_shuffle_ps(vec_tmp0, vec_tmp1, _MM_SHUFFLE(2, 0, 2, 0)); // 2 6 10 14
            F32vec4 vec_iy3 = _mm_shuffle_ps(vec_tmp0, vec_tmp1, _MM_SHUFFLE(3, 1, 3, 1)); // 3 7 11 15
            vec_tmp0 = _mm_shuffle_ps(vec_piy3, vec_piy4, _MM_SHUFFLE(1, 0, 1, 0)); // 12 13 16 17
            vec_tmp1 = _mm_shuffle_ps(vec_piy3, vec_piy4, _MM_SHUFFLE(3, 2, 3, 2)); // 14 15 18 19
            F32vec4 vec_iy4 = _mm_shuffle_ps(vec_iy0, vec_tmp0, _MM_SHUFFLE(2, 0, 2, 1)); // 4 8 12 16
            F32vec4 vec_iy5 = _mm_shuffle_ps(vec_iy1, vec_tmp0, _MM_SHUFFLE(3, 1, 2, 1)); // 5 9 13 17
            F32vec4 vec_iy6 = _mm_shuffle_ps(vec_iy2, vec_tmp1, _MM_SHUFFLE(2, 0, 2, 1)); // 6 10 14 18
            F32vec4 vec_iy7 = _mm_shuffle_ps(vec_iy3, vec_tmp1, _MM_SHUFFLE(3, 1, 2, 1)); // 7 11 15 19  
            //pY[x] = clip_0_255(- 0.029300f * pIy[xx] + 0.073894f * pIy[xx+1] + 0.720225f * pIy[xx+2] + 0.296425f * pIy[xx+3] - 0.061245f * pIy[xx+4]);   
            F32vec4 vec_py0 = _mm_mul_ps(vec_p0, vec_iy0);
            vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy1);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy2);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_iy3);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_iy4);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_py0 = _mm_min_ps(vec_py0, vec_const255);
            vec_py0 = _mm_max_ps(vec_py0, vec_const0); // 0 3 6 9
            //pY[x+1] = clip_0_255(- 0.003186f * pIy[xx+1] - 0.044499f * pIy[xx+2] + 0.547685f * pIy[xx+3] + 0.547685f * pIy[xx+4] - 0.044499f * pIy[xx+5] - 0.003186f * pIy[xx+6]);
            F32vec4 vec_py1 = _mm_mul_ps(vec_p5, vec_iy1);
            vec_tmp0 = _mm_mul_ps(vec_p6, vec_iy2);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p7, vec_iy3);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p7, vec_iy4);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 =_mm_mul_ps(vec_p6, vec_iy5);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p5, vec_iy6);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);  
            vec_py1 = _mm_min_ps(vec_py1, vec_const255);
            vec_py1 = _mm_max_ps(vec_py1, vec_const0);  // 1 4 7 10
            //pY[x+2] = clip_0_255(- 0.061245f * pIy[xx+3] + 0.296425f * pIy[xx+4] + 0.720225f * pIy[xx+5] + 0.073894f * pIy[xx+6] - 0.029300f * pIy[xx+7]);   
            F32vec4 vec_py2 = _mm_mul_ps(vec_p4, vec_iy3);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_iy4);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy5);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy6);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p0, vec_iy7);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_py2 = _mm_min_ps(vec_py2, vec_const255);  // 2 5 8 11
            vec_py2 = _mm_max_ps(vec_py2, vec_const0); 
            // write back
            vec_tmp0 = _mm_shuffle_ps(vec_py2, vec_py0, _MM_SHUFFLE(3, 1, 2, 0)); // 2 8 3 9
            vec_tmp1 = _mm_shuffle_ps(vec_py0, vec_py1, _MM_SHUFFLE(2, 0, 2, 0)); // 0 6 1 7
            F32vec4 vec_tmp2 = _mm_shuffle_ps(vec_py1, vec_py2, _MM_SHUFFLE(3, 1, 3, 1)); // 4 10 5 11
            F32vec4 vec_y0 = _mm_shuffle_ps(vec_tmp1, vec_tmp0, _MM_SHUFFLE(2, 0, 2, 0)); // 0 1 2 3
            F32vec4 vec_y1 = _mm_shuffle_ps(vec_tmp2, vec_tmp1, _MM_SHUFFLE(3, 1, 2, 0)); // 4 5 6 7
            F32vec4 vec_y2 = _mm_shuffle_ps(vec_tmp0, vec_tmp2, _MM_SHUFFLE(3, 1, 3, 1)); // 8 9 10 11
            _mm_store_ps(pY+x, vec_y0);
            _mm_store_ps(pY+x+4, vec_y1);
            _mm_store_ps(pY+x+8, vec_y2);
            //swap registers
            vec_piy0 = vec_piy4;
        }     
        for (; x < iplDst->width; x+=3, xx+=4){  //column
            pY[x] = clip_0_255(- 0.029300f * pIy[xx] + 0.073894f * pIy[xx+1] + 0.720225f * pIy[xx+2] + 0.296425f * pIy[xx+3] - 0.061245f * pIy[xx+4]);                 
            pY[x+1] = clip_0_255(- 0.003186f * pIy[xx+1] - 0.044499f * pIy[xx+2] + 0.547685f * pIy[xx+3] + 0.547685f * pIy[xx+4] - 0.044499f * pIy[xx+5] - 0.003186f * pIy[xx+6]);
            pY[x+2] = clip_0_255(- 0.061245f * pIy[xx+3] + 0.296425f * pIy[xx+4] + 0.720225f * pIy[xx+5] + 0.073894f * pIy[xx+6] - 0.029300f * pIy[xx+7]);                
        }     
    }
 
   safeReleaseImage(&iplIy);

    return true;
   // return true;
 }
 
 bool CImageUtility::resizeLanczos2_2to3_32f_SIMD(IplImage *iplSrc, IplImage *iplDst)
 // Resizes an input image using Lanczos filter.
 // This function is an extension of Matlab buid-in function imresize(...,
 // ..., lanczos3).
 // arguments:
 //			iplSrc -- [I] Input image
 //        iplDst -- [O] Resized image
 //by Cao Shuang
 // Jan, 2015
 // Reviewed and modified by Luhong (9/2/2015)
 {  
    // parse arguments
    if (iplSrc == NULL ||  iplSrc->nChannels != 1 || iplDst == NULL ||  iplDst->nChannels != 1 ||
        iplSrc->depth != SR_DEPTH_32F || iplDst->depth != SR_DEPTH_32F || 
        iplSrc->width <= 0 || iplSrc->height <= 0 || iplDst->width <= 0 || iplDst->height <= 0) {
        showErrMsg( "Invalid inputs in resizeLanczos2_2to3_32f_SIMD()!\n");
        return false;
    }
    if (iplSrc->width*3 != iplDst->width*2 || iplSrc->height*3 != iplDst->height*2) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos2_2to3_32f_SIMD()!\n");
        return false;
    }
    
    IplImage *iplSrcPad = padding(iplSrc, 2, 2, 2, 2);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos2_2to3_32f_SIMD()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }
    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 3*iplSrc->height/2, iplSrc->depth, iplSrc->nChannels);
     if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos2_2to3_32f_SIMD()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }

    static const __m128 vec_p0 = _mm_set1_ps(-0.007761f);
    static const __m128 vec_p1 = _mm_set1_ps(0.140190f);
    static const __m128 vec_p2 = _mm_set1_ps(0.939097f);
    static const __m128 vec_p3 = _mm_set1_ps(-0.071525f);
    static const __m128 vec_p4 = _mm_set1_ps(-0.062500f);
    static const __m128 vec_p5 = _mm_set1_ps(0.562500f);
    static const __m128 vec_const0 = _mm_set1_ps(0.0f);
    static const __m128 vec_const255 = _mm_set1_ps(255.0f);
    for (int y = 0, yy=0; y < iplIy->height; y+=3, yy+=2){
        __declspec(align(64)) float *pIy0 = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
        __declspec(align(64)) float *pIy1 = (__declspec(align(64)) float *)(iplIy->imageData + (y+1) * iplIy->widthStep);
        __declspec(align(64)) float *pIy2 = (__declspec(align(64)) float *)(iplIy->imageData + (y+2) * iplIy->widthStep);
        __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc2 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc3 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc4 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc5 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+5) * iplSrcPad->widthStep);
       // Iy(y,:,) = weight * Strip(:,:,); 
        int x;
        for (x = 0; x < iplSrcPad->width; x+=4) {
            F32vec4 vec_ps0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_ps1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_ps2 = _mm_load_ps(pSrc2+x);
            F32vec4 vec_ps3 = _mm_load_ps(pSrc3+x);
            F32vec4 vec_ps4 = _mm_load_ps(pSrc4+x);
            F32vec4 vec_ps5 = _mm_load_ps(pSrc5+x);
            //pIy0[x] = -0.007761f * pSrc0[x] + 0.140190f * pSrc1[x] + 0.939097f * pSrc2[x] - 0.071525f * pSrc3[x];
            F32vec4 vec_piy0 = _mm_mul_ps(vec_p0, vec_ps0);
            F32vec4 vec_tmp0 = _mm_mul_ps(vec_p1, vec_ps1);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_ps2);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_ps3);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);          
            //pIy1[x] = -0.062500f * pSrc1[x] + 0.562500f * pSrc2[x] + 0.562500f * pSrc3[x] - 0.062500f * pSrc4[x];
            F32vec4 vec_piy1 = _mm_mul_ps(vec_p4, vec_ps1);
            F32vec4 vec_tmp1 = _mm_mul_ps(vec_p5, vec_ps2);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p5, vec_ps3);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p4, vec_ps4);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            //pIy2[x] = -0.071525f * pSrc2[x] + 0.939097f * pSrc3[x] + 0.140190f * pSrc4[x] - 0.007761f * pSrc5[x]; 
            F32vec4 vec_piy2 = _mm_mul_ps(vec_p3, vec_ps2);
            F32vec4 vec_tmp2 = _mm_mul_ps(vec_p2, vec_ps3);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p1, vec_ps4);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p0, vec_ps5);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            //write back
            _mm_store_ps(pIy0+x, vec_piy0);
            _mm_store_ps(pIy1+x, vec_piy1);
            _mm_store_ps(pIy2+x, vec_piy2);

        }
         for (; x < iplSrcPad->width; x++) {                           
            pIy0[x] = -0.007761f * pSrc0[x] + 0.140190f * pSrc1[x] + 0.939097f * pSrc2[x] - 0.071525f * pSrc3[x];
            pIy1[x] = -0.062500f * pSrc1[x] + 0.562500f * pSrc2[x] + 0.562500f * pSrc3[x] - 0.062500f * pSrc4[x];
            pIy2[x] = -0.071525f * pSrc2[x] + 0.939097f * pSrc3[x] + 0.140190f * pSrc4[x] - 0.007761f * pSrc5[x];  
        }
    }
    safeReleaseImage(&iplSrcPad);

    // x direction
     for (int y = 0; y < iplDst->height; y++) {
         __declspec(align(64)) float *pIy = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
         __declspec(align(64)) float *pY = (__declspec(align(64)) float *)(iplDst->imageData + y * iplDst->widthStep);   
         int x, xx;
         F32vec4 vec_piy0 = _mm_load_ps(pIy); // 0 1 2 3
         for (x = 0, xx = 0; x < iplDst->width; x+=12, xx+=8){  //column                
            F32vec4 vec_piy1 = _mm_load_ps(pIy+xx+4); // 4 5 6 7
            F32vec4 vec_piy2 = _mm_load_ps(pIy+xx+8); // 8 9 10 11
            F32vec4 vec_iy0 = _mm_shuffle_ps(vec_piy0, vec_piy1, _MM_SHUFFLE(2, 0, 2, 0)); // 0 2 4 6
            F32vec4 vec_iy1 = _mm_shuffle_ps(vec_piy0 ,vec_piy1, _MM_SHUFFLE(3, 1, 3, 1)); // 1 3 5 7
            F32vec4 vec_iy4 = _mm_shuffle_ps(vec_piy1, vec_piy2, _MM_SHUFFLE(2, 0, 2, 0)); // 4 6 8 10
            F32vec4 vec_iy5 = _mm_shuffle_ps(vec_piy1, vec_piy2, _MM_SHUFFLE(3, 1, 3, 1)); // 5 7 9 11
            F32vec4 vec_iy2 = _mm_shuffle_ps(vec_iy0, vec_iy4, _MM_SHUFFLE(2, 1, 2, 1)); // 2 4 6 8
            F32vec4 vec_iy3 = _mm_shuffle_ps(vec_iy1, vec_iy5, _MM_SHUFFLE(2, 1, 2, 1)); // 3 5 7 9
            //pY[x] = clip_0_255(-0.007761f * pIy[xx] + 0.140190f * pIy[xx+1] + 0.939097f * pIy[xx+2] - 0.071525f * pIy[xx+3]);
            F32vec4 vec_py0 = _mm_mul_ps(vec_p0, vec_iy0);
            F32vec4 vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy1);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy2);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_iy3);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_py0 = _mm_min_ps(vec_py0, vec_const255);
            vec_py0 = _mm_max_ps(vec_py0, vec_const0);
            //pY[x+1] = clip_0_255(-0.062500f * pIy[xx+1] + 0.562500f * pIy[xx+2] + 0.562500f * pIy[xx+3] - 0.062500f * pIy[xx+4]);    
            F32vec4 vec_py1 = _mm_mul_ps(vec_p4, vec_iy1);
            vec_tmp0 = _mm_mul_ps(vec_p5, vec_iy2);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p5, vec_iy3);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_iy4);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_py1 = _mm_min_ps(vec_py1, vec_const255);
            vec_py1 = _mm_max_ps(vec_py1, vec_const0);
            //pY[x+2] = clip_0_255(-0.071525f * pIy[xx+2] + 0.939097f * pIy[xx+3] + 0.140190f * pIy[xx+4] - 0.007761f * pIy[xx+5]);
            F32vec4 vec_py2 = _mm_mul_ps(vec_p3, vec_iy2);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy3);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy4);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p0, vec_iy5);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_py2 = _mm_min_ps(vec_py2, vec_const255);
            vec_py2 = _mm_max_ps(vec_py2, vec_const0);
            // write back
            vec_tmp0 = _mm_shuffle_ps(vec_py2, vec_py0, _MM_SHUFFLE(3, 1, 2, 0)); // 2 8 3 9
            F32vec4 vec_tmp1 = _mm_shuffle_ps(vec_py0, vec_py1, _MM_SHUFFLE(2, 0, 2, 0)); // 0 6 1 7
            F32vec4 vec_tmp2 = _mm_shuffle_ps(vec_py1, vec_py2, _MM_SHUFFLE(3, 1, 3, 1)); // 4 10 5 11
            F32vec4 vec_y0 = _mm_shuffle_ps(vec_tmp1, vec_tmp0, _MM_SHUFFLE(2, 0, 2, 0)); // 0 1 2 3
            F32vec4 vec_y1 = _mm_shuffle_ps(vec_tmp2, vec_tmp1, _MM_SHUFFLE(3, 1, 2, 0)); // 4 5 6 7
            F32vec4 vec_y2 = _mm_shuffle_ps(vec_tmp0, vec_tmp2, _MM_SHUFFLE(3, 1, 3, 1)); // 8 9 10 11
            _mm_store_ps(pY+x, vec_y0);
            _mm_store_ps(pY+x+4, vec_y1);
            _mm_store_ps(pY+x+8, vec_y2);
            //swap registers
            vec_piy0 = vec_piy2;

        }         
         for (; x < iplDst->width; x+=3, xx+=2){  //column             
            pY[x] = clip_0_255(-0.007761f * pIy[xx] + 0.140190f * pIy[xx+1] + 0.939097f * pIy[xx+2] - 0.071525f * pIy[xx+3]);
            pY[x+1] = clip_0_255(-0.062500f * pIy[xx+1] + 0.562500f * pIy[xx+2] + 0.562500f * pIy[xx+3] - 0.062500f * pIy[xx+4]);     
            pY[x+2] = clip_0_255(-0.071525f * pIy[xx+2] + 0.939097f * pIy[xx+3] + 0.140190f * pIy[xx+4] - 0.007761f * pIy[xx+5]);
        }         
    } 
    safeReleaseImage(&iplIy);

    return true;
 }

 bool CImageUtility::resizeLanczos2_1to2_32f_SIMD(IplImage *iplSrc, IplImage *iplDst)
 // Resizes an input image using Lanczos filter.
 // This function is an extension of Matlab buid-in function imresize(...,
 // ..., lanczos3).
 // arguments:
 //			iplSrc -- [I] Input image
 //        iplDst -- [O] Resized image
 //by Cao Shuang
 // Jan, 2015
 // Reviewed and modified by Luhong (9/2/2015)
 {  
    // parse arguments
    if (iplSrc == NULL ||  iplSrc->nChannels != 1 || iplDst == NULL ||  iplDst->nChannels != 1 ||
        iplSrc->depth != SR_DEPTH_32F || iplDst->depth != SR_DEPTH_32F || 
        iplSrc->width <= 0 || iplSrc->height <= 0 || iplDst->width <= 0 || iplDst->height <= 0) {
        showErrMsg( "Invalid inputs in resizeLanczos2_1to2_32f_SIMD()!\n");
        return false;
    }
    if (iplSrc->width*2 != iplDst->width || iplSrc->height*2 != iplDst->height) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos2_1to2_32f_SIMD()!\n");
        return false;
    }

    IplImage *iplSrcPad = padding(iplSrc, 2, 2, 2, 2);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos2_1to2_32f_SIMD()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }

    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 2*iplSrc->height,  iplSrc->depth, iplSrc->nChannels);
    if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos2_1to2_32f_SIMD()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }
    
    static const __m128 vec_p0 = _mm_set1_ps(-0.017727f);
    static const __m128 vec_p1 = _mm_set1_ps(0.233000f);
    static const __m128 vec_p2 = _mm_set1_ps(0.868607f);
    static const __m128 vec_p3 = _mm_set1_ps(-0.083880f);
    static const __m128 vec_const0 = _mm_set1_ps(0.0f);
    static const __m128 vec_const255 = _mm_set1_ps(255.0f);
    for (int y = 0, yy = 0; y < iplIy->height; y+=2, yy++){
        __declspec(align(64)) float *pIy0 = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
        __declspec(align(64)) float *pIy1 = (__declspec(align(64)) float *)(iplIy->imageData + (y+1) * iplIy->widthStep);     
        __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc2 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc3 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc4 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
       // Iy(y,:,) = weight * Strip(:,:,); 
        int x;
        for (x = 0; x < iplIy->width; x+=4) {
            F32vec4 vec_ps0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_ps1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_ps2 = _mm_load_ps(pSrc2+x);
            F32vec4 vec_ps3 = _mm_load_ps(pSrc3+x);
            F32vec4 vec_ps4 = _mm_load_ps(pSrc4+x);
            //pIy0[x] = - 0.017727f * pSrc0[x] + 0.233000f * pSrc1[x] + 0.868607f * pSrc2[x] - 0.083880f * pSrc3[x];    
            F32vec4 vec_piy0 = _mm_mul_ps(vec_ps0, vec_p0);
            F32vec4 vec_tmp0 = _mm_mul_ps(vec_ps1, vec_p1);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_ps2, vec_p2);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_ps3, vec_p3);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            //pIy1[x] = - 0.083880f * pSrc1[x] + 0.868607f * pSrc2[x] + 0.233000f * pSrc3[x] - 0.017727f * pSrc4[x]; 
            F32vec4 vec_piy1 = _mm_mul_ps(vec_ps1, vec_p3);
            F32vec4 vec_tmp1 = _mm_mul_ps(vec_ps2, vec_p2);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_ps3, vec_p1);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_ps4, vec_p0);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            // Write back
            _mm_store_ps(pIy0+x, vec_piy0);
            _mm_store_ps(pIy1+x, vec_piy1);
        }
        for (; x < iplIy->width; x++) {
            pIy0[x] = - 0.017727f * pSrc0[x] + 0.233000f * pSrc1[x] + 0.868607f * pSrc2[x] - 0.083880f * pSrc3[x];                 
            pIy1[x] = - 0.083880f * pSrc1[x] + 0.868607f * pSrc2[x] + 0.233000f * pSrc3[x] - 0.017727f * pSrc4[x]; 
        }
    }
    safeReleaseImage(&iplSrcPad);
    
    // x direction
    for (int y = 0; y < iplDst->height; y++) {
        __declspec(align(64)) float *pIy = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
        __declspec(align(64)) float *pY = (__declspec(align(64)) float *)(iplDst->imageData + y * iplDst->widthStep); 
        int x, xx;
        F32vec4 vec_iy0 = _mm_load_ps(pIy); // 0 1 2 3
        for (x = 0, xx = 0; x < iplDst->width; x+=8, xx+=4){  //column          
            F32vec4 vec_iy4 = _mm_load_ps(pIy+xx+4); // 4 5 6 7
            F32vec4 vec_iy2 = _mm_shuffle_ps(vec_iy0, vec_iy4, _MM_SHUFFLE(1, 0, 3, 2)); // 2 3 4 5
            F32vec4 vec_iy1 = _mm_shuffle_ps(vec_iy0, vec_iy2, _MM_SHUFFLE(2, 1, 2, 1)); // 1 2 3 4
            F32vec4 vec_iy3 = _mm_shuffle_ps(vec_iy2, vec_iy4, _MM_SHUFFLE(2, 1, 2, 1)); // 3 4 5 6
            //pY[x] = clip_0_255(- 0.017727f * pIy[xx] + 0.233000f * pIy[xx+1] + 0.868607f * pIy[xx+2] - 0.083880f * pIy[xx+3]);
            F32vec4 vec_py0 = _mm_mul_ps(vec_p0, vec_iy0);
            F32vec4 vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy1);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy2);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_iy3);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_py0 = _mm_min_ps(vec_py0, vec_const255);
            vec_py0 = _mm_max_ps(vec_py0, vec_const0);
            //pY[x+1] = clip_0_255(- 0.083880f * pIy[xx+1] + 0.868607f * pIy[xx+2] + 0.233000f * pIy[xx+3] - 0.017727f * pIy[xx+4]);   
            F32vec4 vec_py1 = _mm_mul_ps(vec_p3, vec_iy1);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy2);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy3);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p0, vec_iy4);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_py1 = _mm_min_ps(vec_py1, vec_const255);
            vec_py1 = _mm_max_ps(vec_py1, vec_const0);
            // Write back
            vec_tmp0  = _mm_shuffle_ps(vec_py0, vec_py1, _MM_SHUFFLE(2, 0, 2, 0)); // 0 4 1 5
            F32vec4 vec_tmp1 = _mm_shuffle_ps(vec_py0, vec_py1, _MM_SHUFFLE(3, 1, 3, 1)); // 2 6 3 7
            F32vec4 vec_y0 = _mm_shuffle_ps(vec_tmp0, vec_tmp1, _MM_SHUFFLE(2, 0, 2, 0)); // 0 1 2 3
            F32vec4 vec_y1 = _mm_shuffle_ps(vec_tmp0, vec_tmp1, _MM_SHUFFLE(3, 1, 3, 1)); // 4 5 6 7
            _mm_store_ps(pY+x, vec_y0);
            _mm_store_ps(pY+x+4, vec_y1);
            //swap registers
            vec_iy0 = vec_iy4;
        }     
        for (; x < iplDst->width; x+=2, xx++){  //column 
            pY[x] = clip_0_255(- 0.017727f * pIy[xx] + 0.233000f * pIy[xx+1] + 0.868607f * pIy[xx+2] - 0.083880f * pIy[xx+3]);
            pY[x+1] = clip_0_255(- 0.083880f * pIy[xx+1] + 0.868607f * pIy[xx+2] + 0.233000f * pIy[xx+3] - 0.017727f * pIy[xx+4]);     
        }     
    }	
    safeReleaseImage(&iplIy);

    return true;
 }

  bool CImageUtility::resizeLanczos2_1to3_32f_SIMD(IplImage *iplSrc, IplImage *iplDst)
 // Resizes an input image using Lanczos filter.
 // This function is an extension of Matlab buid-in function imresize(...,
 // ..., lanczos3).
 // arguments:
 //			iplSrc -- [I] Input image
 //        iplDst -- [O] Resized image
 //by Cao Shuang
 // Jan, 2015
 // Reviewed and modified by Luhong (9/2/2015)
 {  
    // parse arguments
    if (iplSrc == NULL ||  iplSrc->nChannels != 1 || iplDst == NULL ||  iplDst->nChannels != 1 ||
        iplSrc->depth != SR_DEPTH_32F || iplDst->depth != SR_DEPTH_32F || 
        iplSrc->width <= 0 || iplSrc->height <= 0 || iplDst->width <= 0 || iplDst->height <= 0) {
        showErrMsg( "Invalid inputs in resizeLanczos2_1to3_32f_SIMD()!\n");
        return false;
    }
    if (iplSrc->width*3 != iplDst->width || iplSrc->height*3 != iplDst->height) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos2_1to3_32f_SIMD()!\n");
        return false;
    }
   
    IplImage *iplSrcPad = padding(iplSrc, 2, 2, 2, 2);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos2_1to3_32f_SIMD()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }
    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 3*iplSrc->height,  iplSrc->depth, iplSrc->nChannels); 
     if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos2_1to3_32f_SIMD()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }
    
    static const __m128 vec_p0 = _mm_set1_ps(-0.031134f);
    static const __m128 vec_p1 = _mm_set1_ps(0.337038f);
    static const __m128 vec_p2 = _mm_set1_ps(0.778356f);
    static const __m128 vec_p3 = _mm_set1_ps(-0.084259f);
    static const __m128 vec_const0 = _mm_set1_ps(0.0f);
    static const __m128 vec_const255 = _mm_set1_ps(255.0f);
    for (int y = 0, yy = 0; y < iplIy->height; y+=3, yy++){
        __declspec(align(64)) float *pIy0 = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
        __declspec(align(64)) float *pIy1 = (__declspec(align(64)) float *)(iplIy->imageData + (y+1) * iplIy->widthStep);
        __declspec(align(64)) float *pIy2 = (__declspec(align(64)) float *)(iplIy->imageData + (y+2) * iplIy->widthStep);
        __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc2 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc3 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc4 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
       // Iy(y,:,) = weight * Strip(:,:,); 
        int x;
        for (x = 0; x < iplIy->width; x+=4) {
            F32vec4 vec_ps0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_ps1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_ps2 = _mm_load_ps(pSrc2+x);
            F32vec4 vec_ps3 = _mm_load_ps(pSrc3+x);
            F32vec4 vec_ps4 = _mm_load_ps(pSrc4+x);
            //pIy0[x] = -0.031134f * pSrc0[x] + 0.337038f * pSrc1[x] + 0.778356f * pSrc2[x] - 0.084259f * pSrc3[x];  
            F32vec4 vec_piy0 = _mm_mul_ps(vec_p0, vec_ps0);
            F32vec4 vec_tmp0 = _mm_mul_ps(vec_p1, vec_ps1);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_ps2);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_ps3);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            //pIy1[x] = pSrc2[x];
            F32vec4 vec_piy1 = vec_ps2;
            //pIy2[x] = -0.084259f * pSrc1[x] + 0.778356f * pSrc2[x] + 0.337038f * pSrc3[x] - 0.031134f * pSrc4[x];
            F32vec4 vec_piy2 = _mm_mul_ps(vec_p3, vec_ps1);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_ps2);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p1, vec_ps3);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p0, vec_ps4);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp0);
            //write back
            _mm_store_ps(pIy0+x, vec_piy0);
            _mm_store_ps(pIy1+x, vec_piy1);
            _mm_store_ps(pIy2+x, vec_piy2);
        }
        for (; x < iplIy->width; x++) {
            pIy0[x] = -0.031134f * pSrc0[x] + 0.337038f * pSrc1[x] + 0.778356f * pSrc2[x] - 0.084259f * pSrc3[x];                 
            pIy1[x] = pSrc2[x];
            pIy2[x] = -0.084259f * pSrc1[x] + 0.778356f * pSrc2[x] + 0.337038f * pSrc3[x] - 0.031134f * pSrc4[x];
        }
    }
    safeReleaseImage(&iplSrcPad);

    // x direction
    for (int y = 0; y < iplDst->height; y++) {
        __declspec(align(64)) float *pIy = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
        __declspec(align(64)) float *pY = (__declspec(align(64)) float *)(iplDst->imageData + y * iplDst->widthStep);   
        int x, xx;
        F32vec4 vec_iy0 = _mm_load_ps(pIy); // 0 1 2 3
        for (x = 0, xx = 0; x < iplDst->width; x+=12, xx+=4){  //column      
            F32vec4 vec_iy4 = _mm_load_ps(pIy+xx+4); // 4 5 6 7
            F32vec4 vec_iy2 = _mm_shuffle_ps(vec_iy0, vec_iy4, _MM_SHUFFLE(1, 0, 3, 2)); // 2 3 4 5
            F32vec4 vec_iy1 = _mm_shuffle_ps(vec_iy0, vec_iy2, _MM_SHUFFLE(2, 1, 2, 1)); // 1 2 3 4
            F32vec4 vec_iy3 = _mm_shuffle_ps(vec_iy2, vec_iy4, _MM_SHUFFLE(2, 1, 2, 1)); // 3 4 5 6
            //pY[x] = clip_0_255 (-0.031134f * pIy[xx] + 0.337038f * pIy[xx+1] + 0.778356f * pIy[xx+2] - 0.084259f * pIy[xx+3]);
            F32vec4 vec_py0 = _mm_mul_ps(vec_p0, vec_iy0);
            F32vec4 vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy1);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy2);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_iy3);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_py0 = _mm_min_ps(vec_py0, vec_const255);
            vec_py0 = _mm_max_ps(vec_py0, vec_const0);
            //pY[x+1] = clip_0_255 (pIy[xx+2]);
            F32vec4 vec_py1 = _mm_min_ps(vec_iy2, vec_const255);
            vec_py1 = _mm_max_ps(vec_py1, vec_const0);
            //pY[x+2] = clip_0_255 (-0.084259f * pIy[xx+1] + 0.778356f * pIy[xx+2] + 0.337038f * pIy[xx+3] - 0.031134f * pIy[xx+4]); 
            F32vec4 vec_py2 = _mm_mul_ps(vec_p3, vec_iy1);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy2);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy3);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p0, vec_iy4);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_py2 = _mm_min_ps(vec_py2, vec_const255);
            vec_py2 = _mm_max_ps(vec_py2, vec_const0);
            //write back
            vec_tmp0 = _mm_shuffle_ps(vec_py2, vec_py0, _MM_SHUFFLE(3, 1, 2, 0)); // 2 8 3 9
            F32vec4 vec_tmp1 = _mm_shuffle_ps(vec_py0, vec_py1, _MM_SHUFFLE(2, 0, 2, 0)); // 0 6 1 7
            F32vec4 vec_tmp2 = _mm_shuffle_ps(vec_py1, vec_py2, _MM_SHUFFLE(3, 1, 3, 1)); // 4 10 5 11
            F32vec4 vec_y0 = _mm_shuffle_ps(vec_tmp1, vec_tmp0, _MM_SHUFFLE(2, 0, 2, 0)); // 0 1 2 3
            F32vec4 vec_y1 = _mm_shuffle_ps(vec_tmp2, vec_tmp1, _MM_SHUFFLE(3, 1, 2, 0)); // 4 5 6 7
            F32vec4 vec_y2 = _mm_shuffle_ps(vec_tmp0, vec_tmp2, _MM_SHUFFLE(3, 1, 3, 1)); // 8 9 10 11
            _mm_store_ps(pY+x, vec_y0);
            _mm_store_ps(pY+x+4, vec_y1);
            _mm_store_ps(pY+x+8, vec_y2);
            //swap registers
            vec_iy0 = vec_iy4;

        }       
        for (; x < iplDst->width; x+=3, xx++){  //column                       
            pY[x] = clip_0_255 (-0.031134f * pIy[xx] + 0.337038f * pIy[xx+1] + 0.778356f * pIy[xx+2] - 0.084259f * pIy[xx+3]);
            pY[x+1] = clip_0_255 (pIy[xx+2]);
            pY[x+2] = clip_0_255 (-0.084259f * pIy[xx+1] + 0.778356f * pIy[xx+2] + 0.337038f * pIy[xx+3] - 0.031134f * pIy[xx+4]);                  
        }       
    }
    
    safeReleaseImage(&iplIy);

    return true;
 }

  bool CImageUtility::resizeLanczos3_4to3_32f_SIMD(IplImage *iplSrc, IplImage *iplDst)
 // Resizes an input image using Lanczos filter.
 // This function is an extension of Matlab buid-in function imresize(...,
 // ..., lanczos3).
 // arguments:
 //			iplSrc -- [I] Input image
 //        iplDst -- [O] Resized image
 //by Cao Shuang
 // Jan, 2015
 // Reviewed and modified by Luhong (9/2/2015)
 {  
    // parse arguments
    if (iplSrc == NULL ||  iplSrc->nChannels != 1 || iplDst == NULL ||  iplDst->nChannels != 1 ||
        iplSrc->depth != SR_DEPTH_32F || iplDst->depth != SR_DEPTH_32F || 
        iplSrc->width <= 0 || iplSrc->height <= 0 || iplDst->width <= 0 || iplDst->height <= 0) {
        showErrMsg( "Invalid inputs in resizeLanczos3_4to3_32f_SIMD()!\n");
        return false;
    }
    if (iplSrc->width*3 != iplDst->width*4 || iplSrc->height*3 != iplDst->height*4) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos3_4to3_32f_SIMD()!\n");
        return false;
    }

    IplImage *iplSrcPad = padding(iplSrc, 3, 3, 3, 3);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos3_4to3_32f_SIMD()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }
    
    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 3*iplSrc->height/4,  iplSrc->depth, iplSrc->nChannels);
    if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos3_4to3_32f_SIMD()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }
    static const __m128 vec_p0 = _mm_set1_ps(0.022792f);
    static const __m128 vec_p1 = _mm_set1_ps(-0.079290f);
    static const __m128 vec_p2 = _mm_set1_ps(0.090643f);
    static const __m128 vec_p3 = _mm_set1_ps(0.730737f);
    static const __m128 vec_p4 = _mm_set1_ps(0.329114f);
    static const __m128 vec_p5 = _mm_set1_ps(-0.110744f);
    static const __m128 vec_p6 = _mm_set1_ps(0.015369f);
    static const __m128 vec_p7 = _mm_set1_ps(0.001381f);
    static const __m128 vec_p8 = _mm_set1_ps(0.011738f);
    static const __m128 vec_p9 = _mm_set1_ps(-0.023007f);
    static const __m128 vec_p10 = _mm_set1_ps(-0.063909f);
    static const __m128 vec_p11 = _mm_set1_ps(0.575177f);
    static const __m128 vec_const0 = _mm_set1_ps(0.0f);
    static const __m128 vec_const255 = _mm_set1_ps(255.0f);
    for (int y = 0, yy = 0; y < iplIy->height; y+=3, yy+=4){
         __declspec(align(64)) float *pIy0 = ( __declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
         __declspec(align(64)) float *pIy1 = ( __declspec(align(64)) float *)(iplIy->imageData + (y+1) * iplIy->widthStep);
         __declspec(align(64)) float *pIy2 = ( __declspec(align(64)) float *)(iplIy->imageData + (y+2) * iplIy->widthStep);
         __declspec(align(64)) float *pSrc0 = ( __declspec(align(64)) float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
         __declspec(align(64)) float *pSrc1 = ( __declspec(align(64)) float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
         __declspec(align(64)) float *pSrc2 = ( __declspec(align(64)) float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
         __declspec(align(64)) float *pSrc3 = ( __declspec(align(64)) float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
         __declspec(align(64)) float *pSrc4 = ( __declspec(align(64)) float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
         __declspec(align(64)) float *pSrc5 = ( __declspec(align(64)) float *)(iplSrcPad->imageData + (yy+5) * iplSrcPad->widthStep);
         __declspec(align(64)) float *pSrc6 = ( __declspec(align(64)) float *)(iplSrcPad->imageData + (yy+6) * iplSrcPad->widthStep);
         __declspec(align(64)) float *pSrc7 = ( __declspec(align(64)) float *)(iplSrcPad->imageData + (yy+7) * iplSrcPad->widthStep);
         __declspec(align(64)) float *pSrc8 = ( __declspec(align(64)) float *)(iplSrcPad->imageData + (yy+8) * iplSrcPad->widthStep);
         __declspec(align(64)) float *pSrc9 = ( __declspec(align(64)) float *)(iplSrcPad->imageData + (yy+9) * iplSrcPad->widthStep);
       // Iy(y,:,) = weight * Strip(:,:,); 
        int x;
        for (x = 0; x < iplSrcPad->width; x+=4) {
            F32vec4 vec_ps0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_ps1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_ps2 = _mm_load_ps(pSrc2+x);
            F32vec4 vec_ps3 = _mm_load_ps(pSrc3+x);
            F32vec4 vec_ps4 = _mm_load_ps(pSrc4+x);
            F32vec4 vec_ps5 = _mm_load_ps(pSrc5+x);
            F32vec4 vec_ps6 = _mm_load_ps(pSrc6+x);
            F32vec4 vec_ps7 = _mm_load_ps(pSrc7+x);
            F32vec4 vec_ps8 = _mm_load_ps(pSrc8+x);
            F32vec4 vec_ps9 = _mm_load_ps(pSrc9+x);
            //pIy0[x] = 0.022792f * pSrc0[x] - 0.079290f * pSrc1[x] + 0.090643f * pSrc2[x] + 0.730737f * pSrc3[x] + 0.329114f * pSrc4[x] - 0.110744f * pSrc5[x] + 0.015369f * pSrc6[x] + 0.001381f * pSrc7[x];                 
            F32vec4 vec_piy0 = _mm_mul_ps(vec_p0, vec_ps0);
            F32vec4 vec_tmp0 = _mm_mul_ps(vec_p1, vec_ps1);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_ps2);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_ps3);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_ps4);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p5, vec_ps5);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p6, vec_ps6);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p7, vec_ps7);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);           
            //pIy1[x] = 0.011738f * pSrc1[x] - 0.023007f * pSrc2[x] - 0.063909f * pSrc3[x] + 0.575177f * pSrc4[x] + 0.575177f * pSrc5[x] - 0.063909f * pSrc6[x] - 0.023007f * pSrc7[x] + 0.011738f * pSrc8[x];
            F32vec4 vec_piy1 = _mm_mul_ps(vec_p8, vec_ps1);
            F32vec4 vec_tmp1 = _mm_mul_ps(vec_p9, vec_ps2);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p10, vec_ps3);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p11, vec_ps4);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p11, vec_ps5);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p10, vec_ps6);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p9, vec_ps7);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p8, vec_ps8);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            //pIy2[x] = 0.001381f * pSrc2[x] + 0.015369f * pSrc3[x] - 0.110744f * pSrc4[x] + 0.329114f * pSrc5[x] + 0.730737f * pSrc6[x] + 0.090643f * pSrc7[x] - 0.079290f * pSrc8[x] + 0.022792f * pSrc9[x];
            F32vec4 vec_piy2 = _mm_mul_ps(vec_p7, vec_ps2);
            F32vec4 vec_tmp2 = _mm_mul_ps(vec_p6, vec_ps3);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p5, vec_ps4);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p4, vec_ps5);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p3, vec_ps6);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p2, vec_ps7);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p1, vec_ps8);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p0, vec_ps9);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            //write back
            _mm_store_ps(pIy0+x, vec_piy0);
            _mm_store_ps(pIy1+x, vec_piy1);
            _mm_store_ps(pIy2+x, vec_piy2);

        }
        for (; x < iplSrcPad->width; x++) {
            pIy0[x] = 0.022792f * pSrc0[x] - 0.079290f * pSrc1[x] + 0.090643f * pSrc2[x] + 0.730737f * pSrc3[x] + 0.329114f * pSrc4[x] - 0.110744f * pSrc5[x] + 0.015369f * pSrc6[x] + 0.001381f * pSrc7[x];                 
            pIy1[x] = 0.011738f * pSrc1[x] - 0.023007f * pSrc2[x] - 0.063909f * pSrc3[x] + 0.575177f * pSrc4[x] + 0.575177f * pSrc5[x] - 0.063909f * pSrc6[x] - 0.023007f * pSrc7[x] + 0.011738f * pSrc8[x];
            pIy2[x] = 0.001381f * pSrc2[x] + 0.015369f * pSrc3[x] - 0.110744f * pSrc4[x] + 0.329114f * pSrc5[x] + 0.730737f * pSrc6[x] + 0.090643f * pSrc7[x] - 0.079290f * pSrc8[x] + 0.022792f * pSrc9[x];
        }
    }
    safeReleaseImage(&iplSrcPad);
    //saveImage("orgLancozosiy.bmp",iplIy);

    // x direction
     //Y(:,x) = Strip(:,:) * weight_x;
    for (int y = 0; y < iplDst->height; y++) {
        __declspec(align(64)) float *pIy = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
        __declspec(align(64)) float *pY = (__declspec(align(64)) float *)(iplDst->imageData + y * iplDst->widthStep);
        int x,xx;
        F32vec4 vec_piy0 = _mm_load_ps(pIy); // 0 1 2 3
        F32vec4 vec_piy1 = _mm_load_ps(pIy+4); // 4 5 6 7
        for (x = 0, xx = 0; x < iplDst->width; x+=12, xx+=16){  //column            
            F32vec4 vec_piy2 = _mm_load_ps(pIy+xx+8); // 8 9 10 11
            F32vec4 vec_piy3 = _mm_load_ps(pIy+xx+12);// 12 13 14 15
            F32vec4 vec_piy4 = _mm_load_ps(pIy+xx+16);// 16 17 18 19
            F32vec4 vec_piy5 = _mm_load_ps(pIy+xx+20);// 20 21 22 23
            F32vec4 vec_tmp0 = _mm_shuffle_ps(vec_piy0, vec_piy1, _MM_SHUFFLE(1, 0, 1, 0)); // 0 1 4 5
            F32vec4 vec_tmp1 = _mm_shuffle_ps(vec_piy2, vec_piy3, _MM_SHUFFLE(1, 0, 1, 0)); // 8 9 12 13
            F32vec4 vec_iy0 = _mm_shuffle_ps(vec_tmp0, vec_tmp1, _MM_SHUFFLE(2, 0, 2, 0));  // 0 4 8 12
            F32vec4 vec_iy1 = _mm_shuffle_ps(vec_tmp0, vec_tmp1, _MM_SHUFFLE(3, 1, 3, 1));  // 1 5 9 13
            vec_tmp0 = _mm_shuffle_ps(vec_piy0, vec_piy1, _MM_SHUFFLE(3, 2, 3, 2)); // 2 3 6 7
            vec_tmp1 = _mm_shuffle_ps(vec_piy2 ,vec_piy3, _MM_SHUFFLE(3, 2, 3, 2)); // 10 11 14 15
            F32vec4 vec_iy2 = _mm_shuffle_ps(vec_tmp0, vec_tmp1, _MM_SHUFFLE(2, 0, 2, 0)); // 2 6 10 14
            F32vec4 vec_iy3 = _mm_shuffle_ps(vec_tmp0, vec_tmp1, _MM_SHUFFLE(3, 1, 3, 1)); // 3 7 11 15
            vec_tmp0 = _mm_shuffle_ps(vec_piy3, vec_piy4, _MM_SHUFFLE(1, 0, 1, 0)); // 12 13 16 17
            vec_tmp1 = _mm_shuffle_ps(vec_piy3, vec_piy4, _MM_SHUFFLE(3, 2, 3, 2)); // 14 15 18 19
            F32vec4 vec_iy4 = _mm_shuffle_ps(vec_iy0, vec_tmp0, _MM_SHUFFLE(2, 0, 2, 1)); // 4 8 12 16
            F32vec4 vec_iy5 = _mm_shuffle_ps(vec_iy1, vec_tmp0, _MM_SHUFFLE(3, 1, 2, 1)); // 5 9 13 17
            F32vec4 vec_iy6 = _mm_shuffle_ps(vec_iy2, vec_tmp1, _MM_SHUFFLE(2, 0, 2, 1)); // 6 10 14 18
            F32vec4 vec_iy7 = _mm_shuffle_ps(vec_iy3, vec_tmp1, _MM_SHUFFLE(3, 1, 2, 1)); // 7 11 15 19  
            vec_tmp0 = _mm_shuffle_ps(vec_piy4, vec_piy5, _MM_SHUFFLE(1, 0, 1, 0)); // 16 17 20 21
            F32vec4 vec_iy8 = _mm_shuffle_ps(vec_iy4, vec_tmp0, _MM_SHUFFLE(2, 0, 2, 1)); //8 12 16 17
            F32vec4 vec_iy9 = _mm_shuffle_ps(vec_iy5, vec_tmp0, _MM_SHUFFLE(3, 1, 2, 1)); //9 13 17 21 
            //pY[x] = clip_0_255(0.022792f * pIy[xx] - 0.079290f * pIy[xx+1] + 0.090643f * pIy[xx+2] + 0.730737f * pIy[xx+3] + 0.329114f * pIy[xx+4] - 0.110744f * pIy[xx+5] + 0.015369f * pIy[xx+6] + 0.001381f * pIy[xx+7]);   
            F32vec4 vec_py0 = _mm_mul_ps(vec_p0, vec_iy0);
            vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy1);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy2);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_iy3);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_iy4);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p5, vec_iy5);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p6, vec_iy6);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p7, vec_iy7);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_py0 = _mm_min_ps(vec_py0, vec_const255);
            vec_py0 = _mm_max_ps(vec_py0, vec_const0);
            //pY[x+1] = clip_0_255(0.011738f * pIy[xx+1] - 0.023007f * pIy[xx+2] - 0.063909f * pIy[xx+3] + 0.575177f * pIy[xx+4] + 0.575177f * pIy[xx+5] - 0.063909f * pIy[xx+6] - 0.023007f * pIy[xx+7] + 0.011738f * pIy[xx+8]);
            F32vec4 vec_py1 = _mm_mul_ps(vec_p8, vec_iy1);
            vec_tmp0 = _mm_mul_ps(vec_p9, vec_iy2);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p10, vec_iy3);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p11, vec_iy4);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p11, vec_iy5);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p10, vec_iy6);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p9, vec_iy7);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p8, vec_iy8);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_py1 = _mm_min_ps(vec_py1, vec_const255);
            vec_py1 = _mm_max_ps(vec_py1, vec_const0);
            //pY[x+2] = clip_0_255(0.001381f * pIy[xx+2] + 0.015369f * pIy[xx+3] - 0.110744f * pIy[xx+4] + 0.329114f * pIy[xx+5] + 0.730737f * pIy[xx+6] + 0.090643f * pIy[xx+7] - 0.079290f * pIy[xx+8] + 0.022792f * pIy[xx+9]);                
            F32vec4 vec_py2 = _mm_mul_ps(vec_p7, vec_iy2);
            vec_tmp0 = _mm_mul_ps(vec_p6, vec_iy3);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p5, vec_iy4);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_iy5);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_iy6);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy7);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy8);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p0, vec_iy9);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_py2 = _mm_min_ps(vec_py2, vec_const255);
            vec_py2 = _mm_max_ps(vec_py2, vec_const0);
            //write back
            vec_tmp0 = _mm_shuffle_ps(vec_py2, vec_py0, _MM_SHUFFLE(3, 1, 2, 0)); // 2 8 3 9
            vec_tmp1 = _mm_shuffle_ps(vec_py0, vec_py1, _MM_SHUFFLE(2, 0, 2, 0)); // 0 6 1 7
            F32vec4 vec_tmp2 = _mm_shuffle_ps(vec_py1, vec_py2, _MM_SHUFFLE(3, 1, 3, 1)); // 4 10 5 11
            F32vec4 vec_y0 = _mm_shuffle_ps(vec_tmp1, vec_tmp0, _MM_SHUFFLE(2, 0, 2, 0)); // 0 1 2 3
            F32vec4 vec_y1 = _mm_shuffle_ps(vec_tmp2, vec_tmp1, _MM_SHUFFLE(3, 1, 2, 0)); // 4 5 6 7
            F32vec4 vec_y2 = _mm_shuffle_ps(vec_tmp0, vec_tmp2, _MM_SHUFFLE(3, 1, 3, 1)); // 8 9 10 11
            _mm_store_ps(pY+x, vec_y0);
            _mm_store_ps(pY+x+4, vec_y1);
            _mm_store_ps(pY+x+8, vec_y2);
            //swap registers
            vec_piy0 = vec_piy4;
            vec_piy1 = vec_piy5;
        
        }        
        for (; x < iplDst->width; x+=3, xx+=4){  //column
            pY[x] = clip_0_255(0.022792f * pIy[xx] - 0.079290f * pIy[xx+1] + 0.090643f * pIy[xx+2] + 0.730737f * pIy[xx+3] + 0.329114f * pIy[xx+4] - 0.110744f * pIy[xx+5] + 0.015369f * pIy[xx+6] + 0.001381f * pIy[xx+7]);                 
            pY[x+1] = clip_0_255(0.011738f * pIy[xx+1] - 0.023007f * pIy[xx+2] - 0.063909f * pIy[xx+3] + 0.575177f * pIy[xx+4] + 0.575177f * pIy[xx+5] - 0.063909f * pIy[xx+6] - 0.023007f * pIy[xx+7] + 0.011738f * pIy[xx+8]);
            pY[x+2] = clip_0_255(0.001381f * pIy[xx+2] + 0.015369f * pIy[xx+3] - 0.110744f * pIy[xx+4] + 0.329114f * pIy[xx+5] + 0.730737f * pIy[xx+6] + 0.090643f * pIy[xx+7] - 0.079290f * pIy[xx+8] + 0.022792f * pIy[xx+9]);                
        }     
    }
 
    safeReleaseImage(&iplIy);

    return true;
 }

 bool CImageUtility::resizeLanczos3_2to3_32f_SIMD(IplImage *iplSrc, IplImage *iplDst)
 // Resizes an input image using Lanczos filter.
 // This function is an extension of Matlab buid-in function imresize(...,
 // ..., lanczos3).
 // arguments:
 //			iplSrc -- [I] Input image
 //        iplDst -- [O] Resized image
 //by Cao Shuang
 // Jan, 2015
 // Reviewed and modified by Luhong (9/2/2015)
 {  
    // parse arguments
    if (iplSrc == NULL ||  iplSrc->nChannels != 1 || iplDst == NULL ||  iplDst->nChannels != 1 ||
        iplSrc->depth != SR_DEPTH_32F || iplDst->depth != SR_DEPTH_32F || 
        iplSrc->width <= 0 || iplSrc->height <= 0 || iplDst->width <= 0 || iplDst->height <= 0) {
        showErrMsg( "Invalid inputs in resizeLanczos3_2to3_32f_SIMD()!\n");
        return false;
    }
    if (iplSrc->width*3 != iplDst->width*2 || iplSrc->height*3 != iplDst->height*2) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos3_2to3_32f_SIMD()!\n");
        return false;
    }

    IplImage *iplSrcPad = padding(iplSrc, 3, 3, 3, 3);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos3_2to3_32f_SIMD()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }

    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 3*iplSrc->height/2,  iplSrc->depth, iplSrc->nChannels);
    if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in fastresizeLanczos3_3_2()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }
    
    static const __m128 vec_p0 = _mm_set1_ps(0.003293f);
    static const __m128 vec_p1 = _mm_set1_ps(-0.042558f);
    static const __m128 vec_p2 = _mm_set1_ps(0.167918f);
    static const __m128 vec_p3 = _mm_set1_ps(0.951600f);
    static const __m128 vec_p4 = _mm_set1_ps(-0.105093f);
    static const __m128 vec_p5 = _mm_set1_ps(0.024840f);
    static const __m128 vec_p6 = _mm_set1_ps(0.024457f);
    static const __m128 vec_p7 = _mm_set1_ps(-0.135870f);
    static const __m128 vec_p8 = _mm_set1_ps(0.611413f);
    static const __m128 vec_const0 = _mm_set1_ps(0.0f);
    static const __m128 vec_const255 = _mm_set1_ps(255.0f);
    
    for (int y = 0, yy = 0; y < 3*iplSrc->height/2; y+=3, yy+=2){
        __declspec(align(64)) float *pIy0 = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
        __declspec(align(64)) float *pIy1 = (__declspec(align(64)) float *)(iplIy->imageData + (y+1) * iplIy->widthStep);
        __declspec(align(64)) float *pIy2 = (__declspec(align(64)) float *)(iplIy->imageData + (y+2) * iplIy->widthStep);
        __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc2 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc3 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc4 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc5 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+5) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc6 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+6) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc7 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+7) * iplSrcPad->widthStep);

       // Iy(y,:,) = weight * Strip(:,:,); 
        int x;
        for (x = 0; x < iplSrcPad->width; x+=4) {
            F32vec4 vec_ps0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_ps1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_ps2 = _mm_load_ps(pSrc2+x);
            F32vec4 vec_ps3 = _mm_load_ps(pSrc3+x);
            F32vec4 vec_ps4 = _mm_load_ps(pSrc4+x);
            F32vec4 vec_ps5 = _mm_load_ps(pSrc5+x);
            F32vec4 vec_ps6 = _mm_load_ps(pSrc6+x);
            F32vec4 vec_ps7 = _mm_load_ps(pSrc7+x);
            //pIy0[x] = 0.003293f * pSrc0[x] - 0.042558f * pSrc1[x] + 0.167918f * pSrc2[x] + 0.951600f * pSrc3[x] - 0.105093f * pSrc4[x] + 0.024840f * pSrc5[x];                 
            F32vec4 vec_piy0 = _mm_mul_ps(vec_p0, vec_ps0);
            F32vec4 vec_tmp0 = _mm_mul_ps(vec_p1, vec_ps1);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_ps2);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_ps3);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_ps4);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p5, vec_ps5);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);            
            //pIy1[x] = 0.024457f * pSrc1[x] - 0.135870f * pSrc2[x] + 0.611413f * pSrc3[x] + 0.611413f * pSrc4[x] - 0.135870f * pSrc5[x] + 0.024457f * pSrc6[x];
            F32vec4 vec_piy1 = _mm_mul_ps(vec_p6, vec_ps1);
            F32vec4 vec_tmp1 = _mm_mul_ps(vec_p7, vec_ps2);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p8, vec_ps3);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p8, vec_ps4);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p7, vec_ps5);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p6, vec_ps6);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);                   
            //pIy2[x] = 0.024840f * pSrc2[x] - 0.105093f *pSrc3[x] + 0.951600f * pSrc4[x] + 0.167919f * pSrc5[x] - 0.042558f * pSrc6[x] + 0.003293f * pSrc7[x];
            F32vec4 vec_piy2 = _mm_mul_ps(vec_p5, vec_ps2);
            F32vec4 vec_tmp2 = _mm_mul_ps(vec_p4, vec_ps3);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p3, vec_ps4);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p2, vec_ps5);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p1, vec_ps6);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p0, vec_ps7);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            // Write back
            _mm_store_ps(pIy0+x, vec_piy0);
            _mm_store_ps(pIy1+x, vec_piy1);
            _mm_store_ps(pIy2+x, vec_piy2);
        }
        for (; x < iplSrcPad->width; x++) {
            pIy0[x] = 0.003293f * pSrc0[x] - 0.042558f * pSrc1[x] + 0.167918f * pSrc2[x] + 0.951600f * pSrc3[x] - 0.105093f * pSrc4[x] + 0.024840f * pSrc5[x];                 
            pIy1[x] = 0.024457f * pSrc1[x] - 0.135870f * pSrc2[x] + 0.611413f * pSrc3[x] + 0.611413f * pSrc4[x] - 0.135870f * pSrc5[x] + 0.024457f * pSrc6[x];
            pIy2[x] = 0.024840f * pSrc2[x] - 0.105093f *pSrc3[x] + 0.951600f * pSrc4[x] + 0.167919f * pSrc5[x] - 0.042558f * pSrc6[x] + 0.003293f * pSrc7[x];
        }
    }
    safeReleaseImage(&iplSrcPad);
    //saveImage("orgLancozosiy.bmp",iplIy);

    // x direction
    //Y(:,x) = Strip(:,:) * weight_x;
    for (int y = 0; y < iplDst->height; y++) {
        __declspec(align(64)) float *pIy = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
        __declspec(align(64)) float *pY = (__declspec(align(64)) float *)(iplDst->imageData + y * iplDst->widthStep);  
        int x, xx;
        F32vec4 vec_piy0 = _mm_load_ps(pIy); // 0 1 2 3
        F32vec4 vec_piy1 = _mm_load_ps(pIy+4); // 4 5 6 7
        for (x = 0, xx = 0; x < iplDst->width; x+=12, xx+=8){  //column               
            F32vec4 vec_piy2 = _mm_load_ps(pIy+xx+8); // 8 9 10 11
            F32vec4 vec_piy3 = _mm_load_ps(pIy+xx+12); // 12 13 14 15
            F32vec4 vec_iy0 = _mm_shuffle_ps(vec_piy0, vec_piy1, _MM_SHUFFLE(2, 0, 2, 0)); // 0 2 4 6
            F32vec4 vec_iy1 = _mm_shuffle_ps(vec_piy0 ,vec_piy1, _MM_SHUFFLE(3, 1, 3, 1)); // 1 3 5 7
            F32vec4 vec_iy4 = _mm_shuffle_ps(vec_piy1, vec_piy2, _MM_SHUFFLE(2, 0, 2, 0)); // 4 6 8 10
            F32vec4 vec_iy5 = _mm_shuffle_ps(vec_piy1, vec_piy2, _MM_SHUFFLE(3, 1, 3, 1)); // 5 7 9 11
            F32vec4 vec_iy2 = _mm_shuffle_ps(vec_iy0, vec_iy4, _MM_SHUFFLE(2, 1, 2, 1)); // 2 4 6 8
            F32vec4 vec_iy3 = _mm_shuffle_ps(vec_iy1, vec_iy5, _MM_SHUFFLE(2, 1, 2, 1)); // 3 5 7 9
            F32vec4 vec_tmp0 = _mm_shuffle_ps(vec_piy2, vec_piy3, _MM_SHUFFLE(1, 0, 3, 2)); // 10 11 12 13
            F32vec4 vec_iy6 = _mm_shuffle_ps(vec_iy4, vec_tmp0, _MM_SHUFFLE(2, 0, 2, 1)); // 6 8 10 12
            F32vec4 vec_iy7 = _mm_shuffle_ps(vec_iy5, vec_tmp0, _MM_SHUFFLE(3, 1, 2, 1)); // 7 9 11 13
            //pY[x] = clip_0_255(0.003293f * pIy[xx] - 0.042558f * pIy[xx+1] + 0.167918f * pIy[xx+2] + 0.951600f * pIy[xx+3] - 0.105093f * pIy[xx+4] + 0.024840f * pIy[xx+5]);
            F32vec4 vec_py0 = _mm_mul_ps(vec_p0, vec_iy0);
            vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy1);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy2);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_iy3);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_iy4);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p5, vec_iy5);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_py0 = _mm_min_ps(vec_py0, vec_const255);
            vec_py0 = _mm_max_ps(vec_py0, vec_const0);
            //pY[x+1] = clip_0_255(0.024457f * pIy[xx+1] - 0.135870f * pIy[xx+2] + 0.611413f * pIy[xx+3] + 0.611413f * pIy[xx+4] - 0.135870f * pIy[xx+5] + 0.024457f * pIy[xx+6]);
            F32vec4 vec_py1 = _mm_mul_ps(vec_p6, vec_iy1);
            vec_tmp0 = _mm_mul_ps(vec_p7, vec_iy2);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p8, vec_iy3);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p8, vec_iy4);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p7, vec_iy5);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p6, vec_iy6);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0); 
            vec_py1 = _mm_min_ps(vec_py1, vec_const255);
            vec_py1 = _mm_max_ps(vec_py1, vec_const0);
            //pY[x+2] = clip_0_255(0.024840f * pIy[xx+2] - 0.105093f * pIy[xx+3] + 0.951600f * pIy[xx+4] + 0.167919f * pIy[xx+5] - 0.042558f * pIy[xx+6] + 0.003293f * pIy[xx+7]);                   
            F32vec4 vec_py2 = _mm_mul_ps(vec_p5, vec_iy2);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_iy3);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_iy4);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy5);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy6);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p0, vec_iy7);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_py2 = _mm_min_ps(vec_py2, vec_const255);
            vec_py2 = _mm_max_ps(vec_py2, vec_const0);
            // write back
            vec_tmp0 = _mm_shuffle_ps(vec_py2, vec_py0, _MM_SHUFFLE(3, 1, 2, 0)); // 2 8 3 9
            F32vec4 vec_tmp1 = _mm_shuffle_ps(vec_py0, vec_py1, _MM_SHUFFLE(2, 0, 2, 0)); // 0 6 1 7
            F32vec4 vec_tmp2 = _mm_shuffle_ps(vec_py1, vec_py2, _MM_SHUFFLE(3, 1, 3, 1)); // 4 10 5 11
            F32vec4 vec_y0 = _mm_shuffle_ps(vec_tmp1, vec_tmp0, _MM_SHUFFLE(2, 0, 2, 0)); // 0 1 2 3
            F32vec4 vec_y1 = _mm_shuffle_ps(vec_tmp2, vec_tmp1, _MM_SHUFFLE(3, 1, 2, 0)); // 4 5 6 7
            F32vec4 vec_y2 = _mm_shuffle_ps(vec_tmp0, vec_tmp2, _MM_SHUFFLE(3, 1, 3, 1)); // 8 9 10 11
            _mm_store_ps(pY+x, vec_y0);
            _mm_store_ps(pY+x+4, vec_y1);
            _mm_store_ps(pY+x+8, vec_y2);
            //swap registers
            vec_piy0 = vec_piy2;
            vec_piy1 = vec_piy3;
        }   
        for (; x < iplDst->width; x+=3, xx+=2){  //column    
            pY[x] = clip_0_255(0.003293f * pIy[xx] - 0.042558f * pIy[xx+1] + 0.167918f * pIy[xx+2] + 0.951600f * pIy[xx+3] - 0.105093f * pIy[xx+4] + 0.024840f * pIy[xx+5]);
            pY[x+1] = clip_0_255(0.024457f * pIy[xx+1] - 0.135870f * pIy[xx+2] + 0.611413f * pIy[xx+3] + 0.611413f * pIy[xx+4] - 0.135870f * pIy[xx+5] + 0.024457f * pIy[xx+6]);
            pY[x+2] = clip_0_255(0.024840f * pIy[xx+2] - 0.105093f * pIy[xx+3] + 0.951600f * pIy[xx+4] + 0.167919f * pIy[xx+5] - 0.042558f * pIy[xx+6] + 0.003293f * pIy[xx+7]);                   
        }   
    }
 
    safeReleaseImage(&iplIy);

    return true;
 }

 bool CImageUtility::resizeLanczos3_1to2_32f_SIMD(IplImage *iplSrc, IplImage *iplDst)
 // Resizes an input image using Lanczos filter.
 // This function is an extension of Matlab buid-in function imresize(...,
 // ..., lanczos3).
 // arguments:
 //			iplSrc -- [I] Input image
 //        iplDst -- [O] Resized image
 //by Cao Shuang
 // Jan, 2015
 // Reviewed and modified by Luhong (9/2/2015)
 {  
    // parse arguments
    if (iplSrc == NULL ||  iplSrc->nChannels != 1 || iplDst == NULL ||  iplDst->nChannels != 1 ||
        iplSrc->depth != SR_DEPTH_32F || iplDst->depth != SR_DEPTH_32F || 
        iplSrc->width <= 0 || iplSrc->height <= 0 || iplDst->width <= 0 || iplDst->height <= 0) {
        showErrMsg( "Invalid inputs in resizeLanczos3_1to2_32f_SIMD()!\n");
        return false;
    }
    if (iplSrc->width*2 != iplDst->width || iplSrc->height*2 != iplDst->height) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos3_1to2_32f_SIMD()!\n");
        return false;
    }
   
    IplImage *iplSrcPad = padding(iplSrc, 3, 3, 3, 3);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos3_1to2_32f_SIMD()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }

    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 2*iplSrc->height,  iplSrc->depth, iplSrc->nChannels);
    if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos3_1to2_32f_SIMD()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }

    static const __m128 vec_p0 = _mm_set1_ps(0.007378f);
    static const __m128 vec_p1 = _mm_set1_ps(-0.067997f);
    static const __m128 vec_p2 = _mm_set1_ps(0.271011f);
    static const __m128 vec_p3 = _mm_set1_ps(0.892771f);
    static const __m128 vec_p4 = _mm_set1_ps(-0.133275f);
    static const __m128 vec_p5 = _mm_set1_ps(0.030112f);
    static const __m128 vec_const0 = _mm_set1_ps(0.0f);
    static const __m128 vec_const255 = _mm_set1_ps(255.0f);
    for (int y = 0, yy = 0; y < iplIy->height; y+=2, yy++){
        __declspec(align(64)) float *pIy0 = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
        __declspec(align(64)) float *pIy1 = (__declspec(align(64)) float *)(iplIy->imageData + (y+1) * iplIy->widthStep);
        __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc2 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc3 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc4 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc5 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+5) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc6 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+6) * iplSrcPad->widthStep);
       // Iy(y,:,) = weight * Strip(:,:,); 
        int x;
        for (x = 0; x < iplSrcPad->width; x+=4) {
            F32vec4 vec_ps0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_ps1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_ps2 = _mm_load_ps(pSrc2+x);
            F32vec4 vec_ps3 = _mm_load_ps(pSrc3+x);
            F32vec4 vec_ps4 = _mm_load_ps(pSrc4+x);
            F32vec4 vec_ps5 = _mm_load_ps(pSrc5+x);
            F32vec4 vec_ps6 = _mm_load_ps(pSrc6+x);
            //pIy0[x] = 0.007378f * pSrc0[x] - 0.067997f * pSrc1[x] + 0.271011f * pSrc2[x] + 0.892771f * pSrc3[x] - 0.133275f * pSrc4[x] + 0.030112f * pSrc5[x];
            F32vec4 vec_piy0 = _mm_mul_ps(vec_p0, vec_ps0);
            F32vec4 vec_tmp0 = _mm_mul_ps(vec_p1, vec_ps1);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_ps2);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_ps3);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_ps4);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p5, vec_ps5);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            //pIy1[x] = 0.030112f * pSrc1[x] - 0.133275f * pSrc2[x] + 0.892771f * pSrc3[x] + 0.271011f * pSrc4[x] - 0.067997f * pSrc5[x] + 0.007378f * pSrc6[x];
            F32vec4 vec_piy1 = _mm_mul_ps(vec_p5, vec_ps1);
            F32vec4 vec_tmp1 = _mm_mul_ps(vec_p4, vec_ps2);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p3, vec_ps3);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p2, vec_ps4);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p1, vec_ps5);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            vec_tmp1 = _mm_mul_ps(vec_p0, vec_ps6);
            vec_piy1 = _mm_add_ps(vec_piy1, vec_tmp1);
            // Write back
            _mm_store_ps(pIy0+x, vec_piy0);
            _mm_store_ps(pIy1+x, vec_piy1);
        }
        for (; x < iplSrcPad->width; x++) {
            pIy0[x] = 0.007378f * pSrc0[x] - 0.067997f * pSrc1[x] + 0.271011f * pSrc2[x] + 0.892771f * pSrc3[x] - 0.133275f * pSrc4[x] + 0.030112f * pSrc5[x];                 
            pIy1[x] = 0.030112f * pSrc1[x] - 0.133275f * pSrc2[x] + 0.892771f * pSrc3[x] + 0.271011f * pSrc4[x] - 0.067997f * pSrc5[x] + 0.007378f * pSrc6[x];
        }
    }
    safeReleaseImage(&iplSrcPad);
    //saveImage("orgLancozosiy.bmp",iplIy);

    // x direction
    //Y(:,x) = Strip(:,:) * weight_x;
    for (int y = 0; y < iplDst->height; y++) {
        __declspec(align(64)) float *pIy = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
        __declspec(align(64)) float *pY = (__declspec(align(64)) float *)(iplDst->imageData + y * iplDst->widthStep); 
        int x, xx;
        F32vec4 vec_iy0 = _mm_load_ps(pIy); // 0 1 2 3
        F32vec4 vec_iy4 = _mm_load_ps(pIy+4); // 4 5 6 7
        for (x = 0, xx = 0; x < iplDst->width; x+=8, xx+=4){  //column
            F32vec4 vec_iy8 = _mm_load_ps(pIy+xx+8); // 8 9 10 11
            F32vec4 vec_iy2 = _mm_shuffle_ps(vec_iy0, vec_iy4, _MM_SHUFFLE(1, 0, 3, 2)); // 2 3 4 5
            F32vec4 vec_iy1 = _mm_shuffle_ps(vec_iy0, vec_iy2, _MM_SHUFFLE(2, 1, 2, 1)); // 1 2 3 4
            F32vec4 vec_iy3 = _mm_shuffle_ps(vec_iy2, vec_iy4, _MM_SHUFFLE(2, 1, 2, 1)); // 3 4 5 6
            F32vec4 vec_iy6 = _mm_shuffle_ps(vec_iy4, vec_iy8, _MM_SHUFFLE(1, 0, 3, 2)); // 6 7 8 9
            F32vec4 vec_iy5 = _mm_shuffle_ps(vec_iy4, vec_iy6, _MM_SHUFFLE(2, 1, 2, 1)); // 5 6 7 8
            //pY[x] = clip_0_255(0.007378f * pIy[xx] - 0.067997f * pIy[xx+1] + 0.271011f * pIy[xx+2] + 0.892771f * pIy[xx+3] - 0.133275f * pIy[xx+4] + 0.030112f * pIy[xx+5]);
            F32vec4 vec_py0 = _mm_mul_ps(vec_p0, vec_iy0);
            F32vec4 vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy1);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy2);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_iy3);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_iy4);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p5, vec_iy5);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_py0 = _mm_min_ps(vec_py0, vec_const255);
            vec_py0 = _mm_max_ps(vec_py0, vec_const0);           
            //pY[x+1] = clip_0_255(0.030112f * pIy[xx+1] - 0.133275f * pIy[xx+2] + 0.892771f * pIy[xx+3] + 0.271011f * pIy[xx+4] - 0.067997f * pIy[xx+5] + 0.007378f * pIy[xx+6]);   
            F32vec4 vec_py1 = _mm_mul_ps(vec_p5, vec_iy1);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_iy2);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_iy3);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy4);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy5);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p0, vec_iy6);
            vec_py1 = _mm_add_ps(vec_py1, vec_tmp0);
            vec_py1 = _mm_min_ps(vec_py1, vec_const255);
            vec_py1 = _mm_max_ps(vec_py1, vec_const0);
            // Write back
            vec_tmp0  = _mm_shuffle_ps(vec_py0, vec_py1, _MM_SHUFFLE(2, 0, 2, 0)); // 0 4 1 5
            F32vec4 vec_tmp1 = _mm_shuffle_ps(vec_py0, vec_py1, _MM_SHUFFLE(3, 1, 3, 1)); // 2 6 3 7
            F32vec4 vec_y0 = _mm_shuffle_ps(vec_tmp0, vec_tmp1, _MM_SHUFFLE(2, 0, 2, 0)); // 0 1 2 3
            F32vec4 vec_y1 = _mm_shuffle_ps(vec_tmp0, vec_tmp1, _MM_SHUFFLE(3, 1, 3, 1)); // 4 5 6 7
            _mm_store_ps(pY+x, vec_y0);
            _mm_store_ps(pY+x+4, vec_y1);
            //swap registers
            vec_iy0 = vec_iy4;
            vec_iy4 = vec_iy8;
        } 
        for (; x < iplDst->width; x+=2, xx++){  //column
            pY[x] = clip_0_255(0.007378f * pIy[xx] - 0.067997f * pIy[xx+1] + 0.271011f * pIy[xx+2] + 0.892771f * pIy[xx+3] - 0.133275f * pIy[xx+4] + 0.030112f * pIy[xx+5]);
            pY[x+1] = clip_0_255(0.030112f * pIy[xx+1] - 0.133275f * pIy[xx+2] + 0.892771f * pIy[xx+3] + 0.271011f * pIy[xx+4] - 0.067997f * pIy[xx+5] + 0.007378f * pIy[xx+6]);                   
        }
    }

    safeReleaseImage(&iplIy);

    return true;
 }
 
 bool CImageUtility::resizeLanczos3_1to3_32f_SIMD(IplImage *iplSrc, IplImage *iplDst)
 // Resizes an input image using Lanczos filter.
 // This function is an extension of Matlab buid-in function imresize(...,
 // ..., lanczos3).
 // arguments:
 //			iplSrc -- [I] Input image
 //        iplDst -- [O] Resized image
 //by Cao Shuang
 // Jan, 2015
 // Reviewed and modified by Luhong (9/2/2015)
 {  
    // parse arguments
    if (iplSrc == NULL ||  iplSrc->nChannels != 1 || iplDst == NULL ||  iplDst->nChannels != 1 ||
        iplSrc->depth != SR_DEPTH_32F || iplDst->depth != SR_DEPTH_32F || 
        iplSrc->width <= 0 || iplSrc->height <= 0 || iplDst->width <= 0 || iplDst->height <= 0) {
        showErrMsg( "Invalid inputs in resizeLanczos3_1to3_32f_SIMD()!\n");
        return false;
    }
    if (iplSrc->width*3 != iplDst->width || iplSrc->height*3 != iplDst->height) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos3_1to3_32f_SIMD()!\n");
        return false;
    }

    IplImage *iplSrcPad = padding(iplSrc, 3, 3, 3, 3);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos3_1to3_32f_SIMD()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }

    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 3*iplSrc->height,  iplSrc->depth, iplSrc->nChannels);
    if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos3_1to3_32f_SIMD()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }

    static const __m128 vec_p0 = _mm_set1_ps(0.012717f);
    static const __m128 vec_p1 = _mm_set1_ps(-0.093738f);
    static const __m128 vec_p2 = _mm_set1_ps(0.382396f);
    static const __m128 vec_p3 = _mm_set1_ps(0.813875f);
    static const __m128 vec_p4 = _mm_set1_ps(-0.146466f);
    static const __m128 vec_p5 = _mm_set1_ps(0.031216f);
    static const __m128 vec_const0 = _mm_set1_ps(0.0f);
    static const __m128 vec_const255 = _mm_set1_ps(255.0f);
    for (int y = 0, yy = 0; y < iplIy->height; y+=3, yy++){
        __declspec(align(64)) float *pIy0 = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
        __declspec(align(64)) float *pIy1 = (__declspec(align(64)) float *)(iplIy->imageData + (y+1) * iplIy->widthStep);
        __declspec(align(64)) float *pIy2 = (__declspec(align(64)) float *)(iplIy->imageData + (y+2) * iplIy->widthStep);
        __declspec(align(64)) float *pSrc0 = (__declspec(align(64)) float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc1 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc2 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc3 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc4 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc5 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+5) * iplSrcPad->widthStep);
        __declspec(align(64)) float *pSrc6 = (__declspec(align(64)) float *)(iplSrcPad->imageData + (yy+6) * iplSrcPad->widthStep);        
        // Iy(y,:,) = weight * Strip(:,:,); 
        int x;
        for (x = 0; x < iplSrcPad->width; x+=4) {
            F32vec4 vec_ps0 = _mm_load_ps(pSrc0+x);
            F32vec4 vec_ps1 = _mm_load_ps(pSrc1+x);
            F32vec4 vec_ps2 = _mm_load_ps(pSrc2+x);
            F32vec4 vec_ps3 = _mm_load_ps(pSrc3+x);
            F32vec4 vec_ps4 = _mm_load_ps(pSrc4+x);
            F32vec4 vec_ps5 = _mm_load_ps(pSrc5+x);
            F32vec4 vec_ps6 = _mm_load_ps(pSrc6+x);
            //pIy0[x] = 0.012717f * pSrc0[x] - 0.093738f * pSrc1[x] + 0.382396f * pSrc2[x] + 0.813875f * pSrc3[x] - 0.146466f * pSrc4[x] + 0.031216f * pSrc5[x];  
            F32vec4 vec_piy0 = _mm_mul_ps(vec_p0, vec_ps0);
            F32vec4 vec_tmp0 = _mm_mul_ps(vec_p1, vec_ps1);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_ps2);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_ps3);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_ps4);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p5, vec_ps5);
            vec_piy0 = _mm_add_ps(vec_piy0, vec_tmp0);
            //pIy1[x] = pSrc3[x];
            F32vec4 vec_piy1 = vec_ps3;
            //pIy2[x] = 0.031216f * pSrc1[x] - 0.146466f * pSrc2[x] + 0.813875f * pSrc3[x] + 0.382396f * pSrc4[x] - 0.093738f * pSrc5[x] + 0.012717f * pSrc6[x];
            F32vec4 vec_piy2 = _mm_mul_ps(vec_p5, vec_ps1);
            F32vec4 vec_tmp2 = _mm_mul_ps(vec_p4, vec_ps2);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p3, vec_ps3);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p2, vec_ps4);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p1, vec_ps5);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            vec_tmp2 = _mm_mul_ps(vec_p0, vec_ps6);
            vec_piy2 = _mm_add_ps(vec_piy2, vec_tmp2);
            // write back
            _mm_store_ps(pIy0+x, vec_piy0);
            _mm_store_ps(pIy1+x, vec_piy1);
            _mm_store_ps(pIy2+x, vec_piy2);

        }
        for (; x < iplSrcPad->width; x++) {
            pIy0[x] = 0.012717f * pSrc0[x] - 0.093738f * pSrc1[x] + 0.382396f * pSrc2[x] + 0.813875f * pSrc3[x] - 0.146466f * pSrc4[x] + 0.031216f * pSrc5[x];                 
            pIy1[x] = pSrc3[x];
            pIy2[x] = 0.031216f * pSrc1[x] - 0.146466f * pSrc2[x] + 0.813875f * pSrc3[x] + 0.382396f * pSrc4[x] - 0.093738f * pSrc5[x] + 0.012717f * pSrc6[x];
        }
    }
    safeReleaseImage(&iplSrcPad);
    //saveImage("orgLancozosiy.bmp",iplIy);

    // x direction
    //Y(:,x) = Strip(:,:) * weight_x;
    for (int y = 0; y < iplDst->height; y++) {
        __declspec(align(64)) float *pIy = (__declspec(align(64)) float *)(iplIy->imageData + y * iplIy->widthStep);
        __declspec(align(64)) float *pY = (__declspec(align(64)) float *)(iplDst->imageData + y * iplDst->widthStep);      
       int x, xx;
       F32vec4 vec_iy0 = _mm_load_ps(pIy); // 0 1 2 3
       F32vec4 vec_iy4 = _mm_load_ps(pIy+4); // 4 5 6 7
        for (x = 0, xx = 0; x < iplDst->width; x+=12, xx+=4){  //column      
            F32vec4 vec_iy8 = _mm_load_ps(pIy+xx+8); // 8 9 10 11
            F32vec4 vec_iy2 = _mm_shuffle_ps(vec_iy0, vec_iy4, _MM_SHUFFLE(1, 0, 3, 2)); // 2 3 4 5
            F32vec4 vec_iy1 = _mm_shuffle_ps(vec_iy0, vec_iy2, _MM_SHUFFLE(2, 1, 2, 1)); // 1 2 3 4
            F32vec4 vec_iy3 = _mm_shuffle_ps(vec_iy2, vec_iy4, _MM_SHUFFLE(2, 1, 2, 1)); // 3 4 5 6
            F32vec4 vec_iy6 = _mm_shuffle_ps(vec_iy4, vec_iy8, _MM_SHUFFLE(1, 0, 3, 2)); // 6 7 8 9
            F32vec4 vec_iy5 = _mm_shuffle_ps(vec_iy4, vec_iy6, _MM_SHUFFLE(2, 1, 2, 1)); // 5 6 7 8
            //pY[x] = clip_0_255(0.012717f * pIy[xx] - 0.093738f * pIy[xx+1] + 0.382396f * pIy[xx+2] + 0.813875f * pIy[xx+3] - 0.146466f * pIy[xx+4] + 0.031216f * pIy[xx+5]);
            F32vec4 vec_py0 = _mm_mul_ps(vec_p0, vec_iy0);
            F32vec4 vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy1);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy2);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_iy3);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_iy4);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p5, vec_iy5);
            vec_py0 = _mm_add_ps(vec_py0, vec_tmp0);
            vec_py0 = _mm_min_ps(vec_py0, vec_const255);
            vec_py0 = _mm_max_ps(vec_py0, vec_const0);           
            //pY[x+1] = clip_0_255(pIy[xx+3]);
            F32vec4 vec_py1 = _mm_min_ps(vec_iy3, vec_const255);
            vec_py1 = _mm_max_ps(vec_py1, vec_const0);
            //pY[x+2] = clip_0_255(0.031216f * pIy[xx+1] - 0.146466f * pIy[xx+2] + 0.813875f * pIy[xx+3] + 0.382396f * pIy[xx+4] - 0.093738f * pIy[xx+5] + 0.012717f * pIy[xx+6]);  
            F32vec4 vec_py2 = _mm_mul_ps(vec_p5, vec_iy1);
            vec_tmp0 = _mm_mul_ps(vec_p4, vec_iy2);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p3, vec_iy3);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p2, vec_iy4);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p1, vec_iy5);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_tmp0 = _mm_mul_ps(vec_p0, vec_iy6);
            vec_py2 = _mm_add_ps(vec_py2, vec_tmp0);
            vec_py2 = _mm_min_ps(vec_py2, vec_const255);
            vec_py2 = _mm_max_ps(vec_py2, vec_const0);
            // write back
            vec_tmp0 = _mm_shuffle_ps(vec_py2, vec_py0, _MM_SHUFFLE(3, 1, 2, 0)); // 2 8 3 9
            F32vec4 vec_tmp1 = _mm_shuffle_ps(vec_py0, vec_py1, _MM_SHUFFLE(2, 0, 2, 0)); // 0 6 1 7
            F32vec4 vec_tmp2 = _mm_shuffle_ps(vec_py1, vec_py2, _MM_SHUFFLE(3, 1, 3, 1)); // 4 10 5 11
            F32vec4 vec_y0 = _mm_shuffle_ps(vec_tmp1, vec_tmp0, _MM_SHUFFLE(2, 0, 2, 0)); // 0 1 2 3
            F32vec4 vec_y1 = _mm_shuffle_ps(vec_tmp2, vec_tmp1, _MM_SHUFFLE(3, 1, 2, 0)); // 4 5 6 7
            F32vec4 vec_y2 = _mm_shuffle_ps(vec_tmp0, vec_tmp2, _MM_SHUFFLE(3, 1, 3, 1)); // 8 9 10 11
            _mm_store_ps(pY+x, vec_y0);
            _mm_store_ps(pY+x+4, vec_y1);
            _mm_store_ps(pY+x+8, vec_y2);
            //swap registers
            vec_iy0 = vec_iy4; 
            vec_iy4 = vec_iy8;

        }  
        for (; x < iplDst->width; x+=3, xx++){  //column              
            pY[x] = clip_0_255(0.012717f * pIy[xx] - 0.093738f * pIy[xx+1] + 0.382396f * pIy[xx+2] + 0.813875f * pIy[xx+3] - 0.146466f * pIy[xx+4] + 0.031216f * pIy[xx+5]);
            pY[x+1] = clip_0_255(pIy[xx+3]);
            pY[x+2] = clip_0_255(0.031216f * pIy[xx+1] - 0.146466f * pIy[xx+2] + 0.813875f * pIy[xx+3] + 0.382396f * pIy[xx+4] - 0.093738f * pIy[xx+5] + 0.012717f * pIy[xx+6]);                   
        }
    } 
    safeReleaseImage(&iplIy);

    return true;
 }

#endif      // #ifdef __SR_USE_SIMD
