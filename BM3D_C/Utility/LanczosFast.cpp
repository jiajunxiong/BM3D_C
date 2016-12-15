//
// Fast Lanczos interpolation/resize algorithms
// Developed by Shuang Cao, IC-ASD, ASTRI
// Reviewed by Luhong Liang, IC-ASD, ASTRI
// Feb 2, 2015
//

bool CImageUtility::resizeLanczos_32f(IplImage *iplSrc, IplImage *iplDst, int a) 
 // Function Y = resizeLanczos_32f(X, scale, a)
 // resizes an input image X in factor scale using Lanczos filter.
 // This function is an extension of Matlab buid-in function imresize(...,
 //  ..., lanczos3).
 // Arguments:
 //			iplSrc -- [I] Input image
 //        iplDst -- [O] output image
 //			scale -- [I] arbitrary magnification factor
 //			  a -- [I] support Lanczos 2~11 
 // This funciton supports SIMD version
 // by Cao Shuang
 // Jan 15, 2015
 // Reviewed and modified by Luhong (2/2/2015)
{
    if (iplSrc == NULL ||  iplSrc->nChannels != 1 || iplDst == NULL ||  iplDst->nChannels != 1 ||
        iplSrc->depth != SR_DEPTH_32F || iplDst->depth != SR_DEPTH_32F || 
        iplSrc->width <= 0 || iplSrc->height <= 0 || iplDst->width <= 0 || iplDst->height <= 0) {
        showErrMsg( "Invalid inputs in resizeLanczos_32f()!\n");
        return false;
    }

    if (a < 2 || a > 11) {
        showErrMsg("Only support Lanczos parameter a=2~11 in resizeLanczos_32f()!\n");
		return false;
	}

    if (a == 2) {
        if (iplSrc->width*3 == iplDst->width*4 && iplSrc->height*3 == iplDst->height*4) {
            #ifdef __SR_USE_SIMD
            return resizeLanczos2_4to3_32f_SIMD(iplSrc, iplDst);
            #endif 
            
            return resizeLanczos2_4to3_32f(iplSrc, iplDst);
        } else if (iplSrc->width*3 == iplDst->width*2 && iplSrc->height*3 == iplDst->height*2) {
            #ifdef __SR_USE_SIMD
            return resizeLanczos2_2to3_32f_SIMD(iplSrc, iplDst);
            #endif
            
            return resizeLanczos2_2to3_32f(iplSrc, iplDst);
        } else if (iplSrc->width*2 == iplDst->width && iplSrc->height*2 == iplDst->height) {
            #ifdef __SR_USE_SIMD
            return resizeLanczos2_1to2_32f_SIMD(iplSrc, iplDst);
            #endif
            
            return resizeLanczos2_1to2_32f(iplSrc, iplDst);
        } else if (iplSrc->width*3 == iplDst->width && iplSrc->height*3 == iplDst->height) {
            #ifdef __SR_USE_SIMD
            return resizeLanczos2_1to3_32f_SIMD(iplSrc, iplDst);
            #endif
           
            return resizeLanczos2_1to3_32f(iplSrc, iplDst);
        } else {
            return resizeLanczosArb_32f(iplSrc, iplDst, 2);
        }
    } else if (a == 3) {
        if (iplSrc->width*3 == iplDst->width*4 && iplSrc->height*3 == iplDst->height*4) {
            #ifdef __SR_USE_SIMD
            return resizeLanczos3_4to3_32f_SIMD(iplSrc, iplDst); 
            #endif
            
            return resizeLanczos3_4to3_32f(iplSrc, iplDst); 
        } else if (iplSrc->width*3 == iplDst->width*2 && iplSrc->height*3 == iplDst->height*2) {
            #ifdef __SR_USE_SIMD
            return resizeLanczos3_2to3_32f_SIMD(iplSrc, iplDst);
            #endif
           
            return resizeLanczos3_2to3_32f(iplSrc, iplDst);
        } else if (iplSrc->width*2 == iplDst->width && iplSrc->height*2 == iplDst->height) {
            #ifdef __SR_USE_SIMD
            return resizeLanczos3_1to2_32f_SIMD(iplSrc, iplDst);
            #endif
            
            return resizeLanczos3_1to2_32f(iplSrc, iplDst);
        } else if (iplSrc->width*3 == iplDst->width && iplSrc->height*3 == iplDst->height) {
            #ifdef __SR_USE_SIMD
            return resizeLanczos3_1to3_32f_SIMD(iplSrc, iplDst);
            #endif
           
            return resizeLanczos3_1to3_32f(iplSrc, iplDst);
        } else {
            return resizeLanczosArb_32f(iplSrc, iplDst, 3);
        }
    } else {
        return resizeLanczosArb_32f(iplSrc, iplDst, a);
    } 

 }

 bool CImageUtility::resizeLanczosArb_32f(IplImage *iplSrc, IplImage *iplDst, int a)
 //resizes an input image X in factor scale using Lanczos filter.
 //This function is an extension of Matlab buid-in function imresize(...,
 // ..., lanczos3).
 // arguments:
 //			iplSrc -- [I] Input image
 //			scale -- [I] arbitrary magnification factor
 //			  a -- [I] support Lanczos 2~11 
 //by Cao Shuang
 // Dec. 30, 2014
 // Reviewed and modified by Luhong (2/2/2015)
 {  
    // parse arguments
    if (iplSrc == NULL ||  iplSrc->nChannels != 1 || iplDst == NULL ||  iplDst->nChannels != 1 ||
        iplSrc->depth != SR_DEPTH_32F || iplDst->depth != SR_DEPTH_32F || 
        iplSrc->width <= 0 || iplSrc->height <= 0 || iplDst->width <= 0 || iplDst->height <= 0) {
        showErrMsg( "Invalid inputs in resizeLanczosArb_32f()!\n");
        return false;
    }

    if (a < 2 || a > 11) {
        showErrMsg("Only support Lanczos parameter a=2~11 in resizeLanczosArb_32f()!\n");
		return false;
	}

    float scale_x = (float)iplDst->width / (float)iplSrc->width;
    float scale_y = (float)iplDst->height / (float)iplSrc->height;

    //if (scale_x != scale_y) {
    //    showErrMsg("Support support unique magnification factors in X and Y in resizeLanczosArb_32f()!\n");
    //    return false;
    //}

    // pre-calculate the filter
    
    // compute kernel width and target size
    float kernel_width, kernel_height;
    if (scale_x < 1.0f) {
        kernel_width = ((float)a * 2) / scale_x;
    } else {
        kernel_width = (float)a * 2.0f;
    }
    if (scale_y < 1.0f) {
        kernel_height = ((float)a * 2.0f) / scale_y;
    } else {
        kernel_height = (float)a * 2.0f;
    }

    int hd = int(iplSrc->height * scale_y + 0.5f);
    int wd = int(iplSrc->width  *scale_x + 0.5f);

    // prepare weight and indices (X)
    int wnd_width = (int)ceil(kernel_width) + 2;
    int size = wd * wnd_width;
    float *u = new float[wd]; 
    int *left = new int[wd];
    float *sumparam_x = new float[wd];
    int *indices_x = new int[size];
    float *param_x = new float[size];
    int *indices_x1 = new int[size];
    float *param_x1 = new float[size];
    float *xmatr_x = new float[size];
    if (u == NULL || left == NULL || indices_x == NULL || param_x == NULL || xmatr_x == NULL || sumparam_x == NULL || indices_x1 == NULL|| param_x1 == NULL) {
        showErrMsg ("Fail to allocate buffer in resizeLanczosArb_32f()!\n");
        delete []u; delete []left; delete xmatr_x; delete []param_x; delete [] indices_x; delete sumparam_x; delete indices_x1; delete param_x1;
        return false;
    }
    
    float post1 = 0.5f * (1.0f - 1.0f/scale_x);  
    for (int i=0; i<wd; i++) {
        u[i] = (i+1)/scale_x + post1;   
    }
   

    float post2 = kernel_width/2;
    for (int i=0; i<wd; i++){
        left[i] = (int)floor(u[i] - post2);
    }
 
    for (int i = 0; i < wd; i++) {
        int i1 = i * wnd_width;
        for (int j = 0 ; j < wnd_width; j++){
            indices_x[i1+j] = left[i]+j;     //indices refers to the padded source image
            xmatr_x[i1+j] = indices_x[i1+j] - u[i];
        }
    }
    if (scale_x < 1) { 
        for (int i = 0; i < size; i++) {
            xmatr_x[i] *= scale_x;
        }
    }
    
    float eps = 2.2204e-16f;
    for (int i = 0; i <size; i++) {
        double pix = PI * xmatr_x[i]; 
        param_x[i] = (float)(( sin(pix) * sin(pix/a)+eps) /(pix * pix / a + eps)); //modified by CAO Shuang
        param_x[i] *= (abs(xmatr_x[i]) < a ? 1 : 0);
    }
    // calculate sum(param_x(i, :))
    for (int i = 0 ; i <wd; i++) {
        int i1 = i * wnd_width;
        sumparam_x[i] = 0;
        for (int j = 0; j < wnd_width; j++){
        sumparam_x[i] += param_x[i1+j]; 
        }
    }
    for (int i = 0 ; i <wd; i++) {
        int i1 = i * wnd_width;
        for (int j = 0; j < wnd_width; j++){
        param_x[i1+j] = param_x[i1+j] / sumparam_x[i]; 
        }
    }

    //to find all-zero columns
    int _buf_param_x [30] = {0};
    for (int i = 0; i < wnd_width; i++) {
        for (int j = 0; j<wd; j++) {
            int j1 = j*wnd_width+i;
            if (param_x[j1] != 0) {
                _buf_param_x [i] = 1;
                break;
            }
        } 
    }
    // to remove all-zero colums and corresponding indices
    int k = 0;
    for (int j = 0; j<wd; j++) {
        int j1 = j*wnd_width;
        k = 0;
        for (int i = 0; i < wnd_width; i++) {
            if (_buf_param_x[i] == 0) continue;
            param_x1[j1+k] = param_x[j1+i];
            indices_x1[j1+k] = max(min(indices_x[j1+i], iplSrc->width), 1);
            k++;
        }        
    }
    int num_x = k;
    //test 
    //for (int i = 0; i < 50; i++){
    //    for (int j = 0 ; j<6; j++){
    //        printf("%d\t", indices_x1[i*wnd_width+j]-1);   
    //    }
    //    printf("\n");
    //}


    delete [] u; delete [] left; delete [] xmatr_x; delete []param_x; delete [] indices_x; delete sumparam_x;
    //TODO

   // prepare weight and indices (Y)
    int wnd_height = (int)ceil(kernel_height) + 2;
    int size2 = hd * wnd_height;
    float *u2 = new float[hd]; 
    int *left2 = new int[hd];
    float *sumparam_y = new float[hd];
    int *indices_y = new int[size2];
    float *param_y = new float[size2];
    float *xmatr_y = new float[size2];
    int *indices_y1 = new int[size2];
    float *param_y1 = new float[size2];
    if (u2 == NULL || left2 == NULL || indices_y == NULL || param_y == NULL || xmatr_y == NULL || sumparam_y == NULL || indices_y1 == NULL || param_y1 == NULL) {
        showErrMsg ("Fatal error: fail to allocate buffer in resizeLanczosArb_32f()!\n");
        delete []u2; delete []left2; delete xmatr_y; delete []param_y; delete [] indices_y; delete sumparam_y; delete indices_y1; delete param_y1;
        return false;
    }

    float post3 =  0.5f * (1.0f - 1.0f / scale_y);
    for (int i=0; i<hd; i++) {
        u2[i] = (float)(i+1) / scale_y + post3;   
    }

    float post4 = kernel_height/2;
    for (int i=0; i<hd; i++){
        left2[i] = (int)floor(u2[i] - post4);
    }

    for (int i = 0; i < hd; i++) {
        int i1 = i * wnd_height;
        for (int j = 0 ; j < wnd_height; j++){
            indices_y[i1+j] = left2[i]+j;     //indices refers to the padded source image
            xmatr_y[i1+j] = indices_y[i1+j] - u2[i];
        }
    }
    if (scale_y < 1) { 
        for (int i = 0; i < size2; i++) {
            xmatr_y[i] *= scale_y;
        }
    }
    for (int i = 0; i <size2; i++) {
        double piy = PI * xmatr_y[i]; 
        param_y[i] = (float)(( sin(piy) * sin(piy/a) + eps ) /  (piy * piy / a+eps));
        param_y[i] *= (abs(xmatr_y[i]) < (float)a ? 1.0f : 0.0f);
    }
    
    // calculate sum(param_x(i, :))
    for (int i = 0 ; i < hd; i++) {
        int i1 = i * wnd_height;
        sumparam_y[i] = 0;
        for (int j = 0; j < wnd_height; j++){
        sumparam_y[i] += param_y[i1+j]; 
        }
    }
    for (int i = 0 ; i < hd; i++) {
        int i1 = i * wnd_height;
        for (int j = 0; j < wnd_height; j++){
        param_y[i1+j] = param_y[i1+j] / sumparam_y[i]; 
        }
    }
    // to find all-zero columns
    int _buf_param_y [30] = {0};
    for (int i = 0; i <wnd_height; i++) {
        for (int j = 0; j < hd; j++) {
            int j1 = j*wnd_height+i;
            if (param_y[j1] !=0) { 
                _buf_param_y[i] = 1;
                break;
            }
        }
    }
    // to remove all-zero colums and corresponding indices
    for (int j = 0; j<hd; j++) {
        int j1 = j * wnd_height;
        k=0;
        for (int i = 0; i < wnd_height; i++) {
            if (_buf_param_y[i] == 0) continue;
            param_y1[j1+k] = param_y[j1+i];
            indices_y1[j1+k] = max(min(indices_y[j1+i], iplSrc->height), 1);
            k++;
        }       
    }
    int num_y = k;

    delete []u2; delete []left2; delete xmatr_y; delete []param_y; delete [] indices_y; delete sumparam_y;
    //TODO
        
    //y direction
    int hs = (int)(scale_y * iplSrc->height + 0.5f);
    IplImage *iplIy = CImageUtility::createImage(iplSrc->width, hs,  iplSrc->depth, iplSrc->nChannels);
     if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczosArb_32f()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }
    
  // Iy(y,:,) = weight * Strip(:,:,); 
    float weight_y;
    for (int y = 0; y < iplIy->height; y++){
        int y1 = y * wnd_height;
        float *pIy = (float *)(iplIy->imageData + y * iplIy->widthStep);        
        for (int x = 0; x < iplSrc->width; x++) {
            pIy[x] = 0.0f;
            for (int yy = 0; yy < num_y; yy++) {
                weight_y = param_y1[y1+yy];
                int yy1 = indices_y1[y1+yy]-1;
                float *pSrc = (float *)(iplSrc->imageData + yy1 * iplSrc->widthStep);
                pIy[x] += weight_y * pSrc[x]; 
               // yy1++;
            }
        }
    }
    delete []indices_y1; delete []param_y1;
    //saveImage("orgLancozosiy.bmp",iplIy);  


    // x direction
    //Y(:,x) = Strip(:,:) * weight_x; 
    float weight_x;
    for (int y = 0; y < iplDst->height; y++) {
        float *pIy = (float *)(iplIy->imageData + y * iplIy->widthStep); 
        float *pY = (float *)(iplDst->imageData + y * iplDst->widthStep);      
        for (int x = 0; x < iplDst->width; x++){  //column
            int x1 = x * wnd_width;               
             pY[x] = 0.0f;
             for (int xx = 0; xx < num_x; xx++){
                     weight_x = param_x1[x1+xx];
                     int xx1 = indices_x1[x1+xx]-1;
                     pY[x] += pIy[xx1]*weight_x;
             } 
             pY[x] = clip_0_255(pY[x]);
        }         
    }
    safeReleaseImage(&iplIy); 
    delete []indices_x1; delete []param_x1;
    
    return true;
 }

 bool CImageUtility::resizeLanczos2_4to3_32f(IplImage *iplSrc, IplImage *iplDst)
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
        showErrMsg( "Invalid inputs in resizeLanczos2_4to3_32f()!\n");
        return false;
    }
    if (iplSrc->width*3 != iplDst->width*4 || iplSrc->height*3 != iplDst->height*4) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos2_4to3_32f()!\n");
        return false;
    }

    IplImage *iplSrcPad = padding(iplSrc, 2, 2, 2, 2);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos2_4to3_32f()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }

    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 3*iplSrc->height/4,  iplSrc->depth, iplSrc->nChannels);
    if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos2_4to3_32f()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }
    for (int y = 0, yy = 0; y < iplIy->height; y+=3, yy+=4){
        float *pIy0 = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pIy1 = (float *)(iplIy->imageData + (y+1) * iplIy->widthStep);
        float *pIy2 = (float *)(iplIy->imageData + (y+2) * iplIy->widthStep);
        float *pSrc0 = (float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        float *pSrc1 = (float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        float *pSrc2 = (float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        float *pSrc3 = (float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        float *pSrc4 = (float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
        float *pSrc5 = (float *)(iplSrcPad->imageData + (yy+5) * iplSrcPad->widthStep);
        float *pSrc6 = (float *)(iplSrcPad->imageData + (yy+6) * iplSrcPad->widthStep);
        float *pSrc7 = (float *)(iplSrcPad->imageData + (yy+7) * iplSrcPad->widthStep);

       // Iy(y,:,) = weight * Strip(:,:,); 
        for (int x = 0; x < iplSrcPad->width; x++) {
            pIy0[x] = - 0.029300f * pSrc0[x] + 0.073894f * pSrc1[x] + 0.720225f * pSrc2[x] + 0.296425f * pSrc3[x] - 0.061245f * pSrc4[x];                 
            pIy1[x] = - 0.003186f * pSrc1[x] - 0.044499f * pSrc2[x] + 0.547685f * pSrc3[x] + 0.547685f * pSrc4[x] - 0.044499f * pSrc5[x] - 0.003186f * pSrc6[x];
            pIy2[x] = - 0.061245f * pSrc3[x] + 0.296425f * pSrc4[x] + 0.720225f * pSrc5[x] + 0.073894f * pSrc6[x] - 0.029300f * pSrc7[x];
        }
    }
    safeReleaseImage(&iplSrcPad);

    // x direction
    for (int y = 0; y < iplDst->height; y++) {       
        float *pIy = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pY = (float *)(iplDst->imageData + y * iplDst->widthStep);          
        //Y(:,x) = Strip(:,:) * weight_x;
        for (int x = 0, xx = 0; x < iplDst->width; x+=3, xx+=4){  //column
            pY[x] = clip_0_255(- 0.029300f * pIy[xx] + 0.073894f * pIy[xx+1] + 0.720225f * pIy[xx+2] + 0.296425f * pIy[xx+3] - 0.061245f * pIy[xx+4]);                 
            pY[x+1] = clip_0_255(- 0.003186f * pIy[xx+1] - 0.044499f * pIy[xx+2] + 0.547685f * pIy[xx+3] + 0.547685f * pIy[xx+4] - 0.044499f * pIy[xx+5] - 0.003186f * pIy[xx+6]);
            pY[x+2] = clip_0_255(- 0.061245f * pIy[xx+3] + 0.296425f * pIy[xx+4] + 0.720225f * pIy[xx+5] + 0.073894f * pIy[xx+6] - 0.029300f * pIy[xx+7]);                
        }         
    }
 
   safeReleaseImage(&iplIy);

   return true;
 }

 bool CImageUtility::resizeLanczos2_2to3_32f(IplImage *iplSrc, IplImage *iplDst)
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
        showErrMsg( "Invalid inputs in resizeLanczos2_2to3_32f()!\n");
        return false;
    }
    if (iplSrc->width*3 != iplDst->width*2 || iplSrc->height*3 != iplDst->height*2) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos2_2to3_32f()!\n");
        return false;
    }

    IplImage *iplSrcPad = padding(iplSrc, 2, 2, 2, 2);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos2_2to3_32f()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }
    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 3*iplSrc->height/2, iplSrc->depth, iplSrc->nChannels);
     if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos2_2to3_32f()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }

    for (int y = 0, yy=0; y < iplIy->height; y+=3, yy+=2){
        float *pIy0 = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pIy1 = (float *)(iplIy->imageData + (y+1) * iplIy->widthStep);
        float *pIy2 = (float *)(iplIy->imageData + (y+2) * iplIy->widthStep);
        float *pSrc0 = (float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        float *pSrc1 = (float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        float *pSrc2 = (float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        float *pSrc3 = (float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        float *pSrc4 = (float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
        float *pSrc5 = (float *)(iplSrcPad->imageData + (yy+5) * iplSrcPad->widthStep);
       // Iy(y,:,) = weight * Strip(:,:,); 
        for (int x = 0; x < iplSrcPad->width; x++) {                           
            pIy0[x] = -0.007761f * pSrc0[x] + 0.140190f * pSrc1[x] + 0.939097f * pSrc2[x] - 0.071525f * pSrc3[x];
            pIy1[x] = -0.062500f * pSrc1[x] + 0.562500f * pSrc2[x] + 0.562500f * pSrc3[x] - 0.062500f * pSrc4[x];
            pIy2[x] = -0.071525f * pSrc2[x] + 0.939097f * pSrc3[x] + 0.140190f * pSrc4[x] - 0.007761f * pSrc5[x];  
        }
    }
    safeReleaseImage(&iplSrcPad);

    // x direction
     for (int y = 0; y < iplDst->height; y++) {
         float *pIy = (float *)(iplIy->imageData + y * iplIy->widthStep);
         float *pY = (float *)(iplDst->imageData + y * iplDst->widthStep);   
         for (int x = 0, xx = 0; x < iplDst->width; x+=3, xx+=2){  //column             
            pY[x] = clip_0_255(-0.007761f * pIy[xx] + 0.140190f * pIy[xx+1] + 0.939097f * pIy[xx+2] - 0.071525f * pIy[xx+3]);
            pY[x+1] = clip_0_255(-0.062500f * pIy[xx+1] + 0.562500f * pIy[xx+2] + 0.562500f * pIy[xx+3] - 0.062500f * pIy[xx+4]);     
            pY[x+2] = clip_0_255(-0.071525f * pIy[xx+2] + 0.939097f * pIy[xx+3] + 0.140190f * pIy[xx+4] - 0.007761f * pIy[xx+5]);
        }         
    } 
    safeReleaseImage(&iplIy);

    return true;
 }

 bool CImageUtility::resizeLanczos2_1to2_32f(IplImage *iplSrc, IplImage *iplDst)
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
        showErrMsg( "Invalid inputs in resizeLanczos2_1to2_32f()!\n");
        return false;
    }
    if (iplSrc->width*2 != iplDst->width || iplSrc->height*2 != iplDst->height) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos2_1to2_32f()!\n");
        return false;
    }

    IplImage *iplSrcPad = padding(iplSrc, 2, 2, 2, 2);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos2_1to2_32f()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }

    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 2*iplSrc->height,  iplSrc->depth, iplSrc->nChannels);
    if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos2_1to2_32f()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }

    for (int y = 0, yy = 0; y < iplIy->height; y+=2, yy++){
        float *pIy0 = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pIy1 = (float *)(iplIy->imageData + (y+1) * iplIy->widthStep);     
        float *pSrc0 = (float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        float *pSrc1 = (float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        float *pSrc2 = (float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        float *pSrc3 = (float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        float *pSrc4 = (float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
       // Iy(y,:,) = weight * Strip(:,:,); 
        for (int x = 0; x < iplIy->width; x++) {
            pIy0[x] = - 0.017727f * pSrc0[x] + 0.233000f * pSrc1[x] + 0.868607f * pSrc2[x] - 0.083880f * pSrc3[x];                 
            pIy1[x] = - 0.083880f * pSrc1[x] + 0.868607f * pSrc2[x] + 0.233000f * pSrc3[x] - 0.017727f * pSrc4[x]; 
        }
    }
    safeReleaseImage(&iplSrcPad);
    
    // x direction
    for (int y = 0; y < iplDst->height; y++) {
        float *pIy = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pY = (float *)(iplDst->imageData + y * iplDst->widthStep); 
        for (int x = 0, xx = 0; x < iplDst->width; x+=2, xx++){  //column 
            pY[x] = clip_0_255(- 0.017727f * pIy[xx] + 0.233000f * pIy[xx+1] + 0.868607f * pIy[xx+2] - 0.083880f * pIy[xx+3]);
            pY[x+1] = clip_0_255(- 0.083880f * pIy[xx+1] + 0.868607f * pIy[xx+2] + 0.233000f * pIy[xx+3] - 0.017727f * pIy[xx+4]);     
        }         
    }	
    safeReleaseImage(&iplIy);

    return true;
 }

 bool CImageUtility::resizeLanczos2_1to3_32f(IplImage *iplSrc, IplImage *iplDst)
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
        showErrMsg( "Invalid inputs in resizeLanczos2_1to3_32f()!\n");
        return false;
    }
    if (iplSrc->width*3 != iplDst->width || iplSrc->height*3 != iplDst->height) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos2_1to3_32f()!\n");
        return false;
    }

    IplImage *iplSrcPad = padding(iplSrc, 2, 2, 2, 2);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos2_1to3_32f()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }
    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 3*iplSrc->height,  iplSrc->depth, iplSrc->nChannels); 
     if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos2_1to3_32f()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }
    for (int y = 0, yy = 0; y < iplIy->height; y+=3, yy++){
        float *pIy0 = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pIy1 = (float *)(iplIy->imageData + (y+1) * iplIy->widthStep);
        float *pIy2 = (float *)(iplIy->imageData + (y+2) * iplIy->widthStep);
        float *pSrc0 = (float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        float *pSrc1 = (float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        float *pSrc2 = (float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        float *pSrc3 = (float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        float *pSrc4 = (float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
       // Iy(y,:,) = weight * Strip(:,:,); 
        for (int x = 0; x < iplIy->width; x++) {
            pIy0[x] = -0.031134f * pSrc0[x] + 0.337038f * pSrc1[x] + 0.778356f * pSrc2[x] - 0.084259f * pSrc3[x];                 
            pIy1[x] = pSrc2[x];
            pIy2[x] = -0.084259f * pSrc1[x] + 0.778356f * pSrc2[x] + 0.337038f * pSrc3[x] - 0.031134f * pSrc4[x];
        }
    }
    safeReleaseImage(&iplSrcPad);

    // x direction
    for (int y = 0; y < iplDst->height; y++) {
        float *pIy = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pY = (float *)(iplDst->imageData + y * iplDst->widthStep);   
        
        for (int x = 0, xx = 0; x < iplDst->width; x+=3, xx++){  //column                       
            pY[x] = clip_0_255 (-0.031134f * pIy[xx] + 0.337038f * pIy[xx+1] + 0.778356f * pIy[xx+2] - 0.084259f * pIy[xx+3]);
            pY[x+1] = clip_0_255 (pIy[xx+2]);
            pY[x+2] = clip_0_255 (-0.084259f * pIy[xx+1] + 0.778356f * pIy[xx+2] + 0.337038f * pIy[xx+3] - 0.031134f * pIy[xx+4]);                  
        }         
    }
    
    safeReleaseImage(&iplIy);

    return true;
 }

 bool CImageUtility::resizeLanczos3_4to3_32f(IplImage *iplSrc, IplImage *iplDst)
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
        showErrMsg( "Invalid inputs in resizeLanczos3_4to3_32f()!\n");
        return false;
    }
    if (iplSrc->width*3 != iplDst->width*4 || iplSrc->height*3 != iplDst->height*4) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos3_4to3_32f()!\n");
        return false;
    }

    IplImage *iplSrcPad = padding(iplSrc, 3, 3, 3, 3);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos3_4to3_32f()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }
    
    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 3*iplSrc->height/4,  iplSrc->depth, iplSrc->nChannels);
    if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos3_4to3_32f()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }

    for (int y = 0, yy = 0; y < iplIy->height; y+=3, yy+=4){
        float *pIy0 = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pIy1 = (float *)(iplIy->imageData + (y+1) * iplIy->widthStep);
        float *pIy2 = (float *)(iplIy->imageData + (y+2) * iplIy->widthStep);
        float *pSrc0 = (float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        float *pSrc1 = (float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        float *pSrc2 = (float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        float *pSrc3 = (float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        float *pSrc4 = (float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
        float *pSrc5 = (float *)(iplSrcPad->imageData + (yy+5) * iplSrcPad->widthStep);
        float *pSrc6 = (float *)(iplSrcPad->imageData + (yy+6) * iplSrcPad->widthStep);
        float *pSrc7 = (float *)(iplSrcPad->imageData + (yy+7) * iplSrcPad->widthStep);
        float *pSrc8 = (float *)(iplSrcPad->imageData + (yy+8) * iplSrcPad->widthStep);
        float *pSrc9 = (float *)(iplSrcPad->imageData + (yy+9) * iplSrcPad->widthStep);

       // Iy(y,:,) = weight * Strip(:,:,); 
        for (int x = 0; x < iplSrcPad->width; x++) {
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
        float *pIy = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pY = (float *)(iplDst->imageData + y * iplDst->widthStep);
        for (int x = 0, xx = 0; x < iplDst->width; x+=3, xx+=4){  //column
            pY[x] = clip_0_255(0.022792f * pIy[xx] - 0.079290f * pIy[xx+1] + 0.090643f * pIy[xx+2] + 0.730737f * pIy[xx+3] + 0.329114f * pIy[xx+4] - 0.110744f * pIy[xx+5] + 0.015369f * pIy[xx+6] + 0.001381f * pIy[xx+7]);                 
            pY[x+1] = clip_0_255(0.011738f * pIy[xx+1] - 0.023007f * pIy[xx+2] - 0.063909f * pIy[xx+3] + 0.575177f * pIy[xx+4] + 0.575177f * pIy[xx+5] - 0.063909f * pIy[xx+6] - 0.023007f * pIy[xx+7] + 0.011738f * pIy[xx+8]);
            pY[x+2] = clip_0_255(0.001381f * pIy[xx+2] + 0.015369f * pIy[xx+3] - 0.110744f * pIy[xx+4] + 0.329114f * pIy[xx+5] + 0.730737f * pIy[xx+6] + 0.090643f * pIy[xx+7] - 0.079290f * pIy[xx+8] + 0.022792f * pIy[xx+9]);                
        }         
    }
 
    safeReleaseImage(&iplIy);

    return true;
 }

 bool CImageUtility::resizeLanczos3_2to3_32f(IplImage *iplSrc, IplImage *iplDst)
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
        showErrMsg( "Invalid inputs in resizeLanczos3_2to3_32f()!\n");
        return false;
    }
    if (iplSrc->width*3 != iplDst->width*2 || iplSrc->height*3 != iplDst->height*2) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos3_2to3_32f()!\n");
        return false;
    }

    IplImage *iplSrcPad = padding(iplSrc, 3, 3, 3, 3);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos3_2to3_32f()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }

    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 3*iplSrc->height/2,  iplSrc->depth, iplSrc->nChannels);
    if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos3_2to3_32f()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }
    
    for (int y = 0, yy = 0; y < 3*iplSrc->height/2; y+=3, yy+=2){
        float *pIy0 = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pIy1 = (float *)(iplIy->imageData + (y+1) * iplIy->widthStep);
        float *pIy2 = (float *)(iplIy->imageData + (y+2) * iplIy->widthStep);
        float *pSrc0 = (float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        float *pSrc1 = (float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        float *pSrc2 = (float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        float *pSrc3 = (float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        float *pSrc4 = (float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
        float *pSrc5 = (float *)(iplSrcPad->imageData + (yy+5) * iplSrcPad->widthStep);
        float *pSrc6 = (float *)(iplSrcPad->imageData + (yy+6) * iplSrcPad->widthStep);
        float *pSrc7 = (float *)(iplSrcPad->imageData + (yy+7) * iplSrcPad->widthStep);

       // Iy(y,:,) = weight * Strip(:,:,); 
        for (int x = 0; x < iplSrcPad->width; x++) {
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
        float *pIy = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pY = (float *)(iplDst->imageData + y * iplDst->widthStep);  
        for (int x = 0, xx = 0; x < iplDst->width; x+=3, xx+=2){  //column    
            pY[x] = clip_0_255(0.003293f * pIy[xx] - 0.042558f * pIy[xx+1] + 0.167918f * pIy[xx+2] + 0.951600f * pIy[xx+3] - 0.105093f * pIy[xx+4] + 0.024840f * pIy[xx+5]);
            pY[x+1] = clip_0_255(0.024457f * pIy[xx+1] - 0.135870f * pIy[xx+2] + 0.611413f * pIy[xx+3] + 0.611413f * pIy[xx+4] - 0.135870f * pIy[xx+5] + 0.024457f * pIy[xx+6]);
            pY[x+2] = clip_0_255(0.024840f * pIy[xx+2] - 0.105093f * pIy[xx+3] + 0.951600f * pIy[xx+4] + 0.167919f * pIy[xx+5] - 0.042558f * pIy[xx+6] + 0.003293f * pIy[xx+7]);                   
        }         
    }
 
    safeReleaseImage(&iplIy);
    
    return true;
 }

 bool CImageUtility::resizeLanczos3_1to2_32f(IplImage *iplSrc, IplImage *iplDst)
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
        showErrMsg( "Invalid inputs in resizeLanczos3_1to2_32f()!\n");
        return false;
    }
    if (iplSrc->width*2 != iplDst->width || iplSrc->height*2 != iplDst->height) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos3_1to2_32f()!\n");
        return false;
    }

    IplImage *iplSrcPad = padding(iplSrc, 3, 3, 3, 3);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos3_1to2_32f()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }

    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 2*iplSrc->height,  iplSrc->depth, iplSrc->nChannels);
    if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos3_1to2_32f()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }

    for (int y = 0, yy = 0; y < iplIy->height; y+=2, yy++){
        float *pIy0 = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pIy1 = (float *)(iplIy->imageData + (y+1) * iplIy->widthStep);
        float *pSrc0 = (float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        float *pSrc1 = (float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        float *pSrc2 = (float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        float *pSrc3 = (float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        float *pSrc4 = (float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
        float *pSrc5 = (float *)(iplSrcPad->imageData + (yy+5) * iplSrcPad->widthStep);
        float *pSrc6 = (float *)(iplSrcPad->imageData + (yy+6) * iplSrcPad->widthStep);
       // Iy(y,:,) = weight * Strip(:,:,); 
        for (int x = 0; x < iplSrcPad->width; x++) {
            pIy0[x] = 0.007378f * pSrc0[x] - 0.067997f * pSrc1[x] + 0.271011f * pSrc2[x] + 0.892771f * pSrc3[x] - 0.133275f * pSrc4[x] + 0.030112f * pSrc5[x];                 
            pIy1[x] = 0.030112f * pSrc1[x] - 0.133275f * pSrc2[x] + 0.892771f * pSrc3[x] + 0.271011f * pSrc4[x] - 0.067997f * pSrc5[x] + 0.007378f * pSrc6[x];
        }
    }
    safeReleaseImage(&iplSrcPad);
    //saveImage("orgLancozosiy.bmp",iplIy);

    // x direction
    //Y(:,x) = Strip(:,:) * weight_x;
    for (int y = 0; y < iplDst->height; y++) {
        float *pIy = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pY = (float *)(iplDst->imageData + y * iplDst->widthStep);     
        for (int x = 0, xx = 0; x < iplDst->width; x+=2, xx++){  //column
            pY[x] = clip_0_255(0.007378f * pIy[xx] - 0.067997f * pIy[xx+1] + 0.271011f * pIy[xx+2] + 0.892771f * pIy[xx+3] - 0.133275f * pIy[xx+4] + 0.030112f * pIy[xx+5]);
            pY[x+1] = clip_0_255(0.030112f * pIy[xx+1] - 0.133275f * pIy[xx+2] + 0.892771f * pIy[xx+3] + 0.271011f * pIy[xx+4] - 0.067997f * pIy[xx+5] + 0.007378f * pIy[xx+6]);                   
        }         
    }

    safeReleaseImage(&iplIy);

    return true;
 }

 bool CImageUtility::resizeLanczos3_1to3_32f(IplImage *iplSrc, IplImage *iplDst)
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
        showErrMsg( "Invalid inputs in resizeLanczos3_1to3_32f()!\n");
        return false;
    }
    if (iplSrc->width*3 != iplDst->width || iplSrc->height*3 != iplDst->height) {
        showErrMsg("The source and destination sizes do not match the magnification factor in resizeLanczos3_1to3_32f()!\n");
        return false;
    }

    IplImage *iplSrcPad = padding(iplSrc, 3, 3, 3, 3);
    if (iplSrcPad == NULL) {
        showErrMsg ("Fatal error: fail to pad image in resizeLanczos3_1to3_32f()!\n");
        safeReleaseImage(&iplSrcPad);
        return false;
    }

    //y direction
    IplImage *iplIy = CImageUtility::createImage(iplSrcPad->width, 3*iplSrc->height,  iplSrc->depth, iplSrc->nChannels);
    if (iplIy == NULL) {
        showErrMsg ("Fatal error: fail to allocate image in resizeLanczos3_1to3_32f()!\n");
        safeReleaseImage(&iplIy);
        return false;
    }

    for (int y = 0, yy = 0; y < iplIy->height; y+=3, yy++){
        float *pIy0 = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pIy1 = (float *)(iplIy->imageData + (y+1) * iplIy->widthStep);
        float *pIy2 = (float *)(iplIy->imageData + (y+2) * iplIy->widthStep);
        float *pSrc0 = (float *)(iplSrcPad->imageData + yy * iplSrcPad->widthStep);
        float *pSrc1 = (float *)(iplSrcPad->imageData + (yy+1) * iplSrcPad->widthStep);
        float *pSrc2 = (float *)(iplSrcPad->imageData + (yy+2) * iplSrcPad->widthStep);
        float *pSrc3 = (float *)(iplSrcPad->imageData + (yy+3) * iplSrcPad->widthStep);
        float *pSrc4 = (float *)(iplSrcPad->imageData + (yy+4) * iplSrcPad->widthStep);
        float *pSrc5 = (float *)(iplSrcPad->imageData + (yy+5) * iplSrcPad->widthStep);
        float *pSrc6 = (float *)(iplSrcPad->imageData + (yy+6) * iplSrcPad->widthStep);
        
        // Iy(y,:,) = weight * Strip(:,:,); 
        for (int x = 0; x < iplSrcPad->width; x++) {
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
        float *pIy = (float *)(iplIy->imageData + y * iplIy->widthStep);
        float *pY = (float *)(iplDst->imageData + y * iplDst->widthStep);      
       
        for (int x = 0, xx = 0; x < iplDst->width; x+=3, xx++){  //column              
            pY[x] = clip_0_255(0.012717f * pIy[xx] - 0.093738f * pIy[xx+1] + 0.382396f * pIy[xx+2] + 0.813875f * pIy[xx+3] - 0.146466f * pIy[xx+4] + 0.031216f * pIy[xx+5]);
            pY[x+1] = clip_0_255(pIy[xx+3]);
            pY[x+2] = clip_0_255(0.031216f * pIy[xx+1] - 0.146466f * pIy[xx+2] + 0.813875f * pIy[xx+3] + 0.382396f * pIy[xx+4] - 0.093738f * pIy[xx+5] + 0.012717f * pIy[xx+6]);                   
        }         
    } 
    safeReleaseImage(&iplIy);

    return true;
 }
