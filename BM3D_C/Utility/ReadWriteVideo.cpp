
#include "stdafx.h"
#include "ReadWriteVideo.h"

//#define _CRTDBG_MAP_ALLOC

CReadWriteVideo::CReadWriteVideo() 
{
#ifdef _EXPSR1_USE_FFMPEG
	m_bFFMpegInitialized = false;
#endif // #ifdef _EXPSR1_USE_FFMPEG
}

CReadWriteVideo::~CReadWriteVideo() 
{
	//_CrtDumpMemoryLeaks();
}

void CReadWriteVideo::message(char stMsg[])
{
	printf("%s", stMsg);
}

ReadWriteVideoHandle *CReadWriteVideo::openVideoRead(char *stFilename, bool bDisableFFMpeg) 
// Open a video file for read 
// For YUV file (should be an index file *.idx), use customized YUV reader.
// For other file, when bDisableFFMpeg is false, try ffmpeg first, then the OpenCV; otehrwise
// using OpenCV
// Develop based on reference code for ffmpeg, by Luhong Liang, IC-ASD, ASTRI, Nov. 30, 2011
{ 
	if (stFilename == NULL || stFilename[0] == 0) {
		printf("Invalide input file name found in CReadWriteVideo::openVideoRead()!\n");
		return NULL;
	}

	ReadWriteVideoHandle *pHandle = new ReadWriteVideoHandle;
	if (pHandle == NULL) {
		printf("Fail to allocate a video handle in CReadWriteVideo::openVideoRead()!\n");
		return NULL;
	}

	pHandle->initHandle();
	strncpy(pHandle->stFilename, stFilename, 255);

#ifdef _EXPSR1_USE_FFMPEG
	if (!bDisableFFMpeg && !m_bFFMpegInitialized) {
		m_objFFMPEG.init();
		m_bFFMpegInitialized = true;
	}
#endif // #ifdef _EXPSR1_USE_FFMPEG

	// open source video
	//IplImage *iplSrcY = NULL, *iplSrcU = NULL, *iplSrcV = NULL;
	char stExt[32];
	CImageUtility::getFileExt(stFilename, stExt);
	if (strcmp(strlwr(stExt), "idx") == 0) {
        // create and assign primary decoder
		pHandle->pYUV = new CReadWriteYUV();
		if (pHandle->pYUV == NULL) {
			printf("Fail to create an YUV object in CReadWriteVideo::openVideoRead()!\n");
			closeVideo(&pHandle);
			return NULL;
		}	
        // open and load index file
		if (pHandle->pYUV->openIDX(stFilename, "r")) {
			pHandle->fmt.width = pHandle->pYUV->getWidth();
			pHandle->fmt.height = pHandle->pYUV->getHeight();
			pHandle->fmt.pix_fmt = cvtPixelFormat_YUV2FFMPEG(pHandle->pYUV->getFormat());
			if (pHandle->fmt.pix_fmt < 0) {
				closeVideo(&pHandle);
				return NULL;
			}
			pHandle->fmt.pix_fmt_rlt = cvtPixelFormat_YUV2FFMPEG(pHandle->pYUV->getResultFormat());
			pHandle->fmt.frame_rate = pHandle->pYUV->getFrameRate();
			pHandle->fmt.bit_rate = 0;
		} else {
			closeVideo(&pHandle);
			return NULL;
		}
        pHandle->type = USE_IDX_READ;
	} else {
#ifdef _EXPSR1_USE_FFMPEG
		if (!bDisableFFMpeg && m_objFFMPEG.openRead(stFilename, pHandle->hFFMpeg))  {	// try FFMPEG firstly
			pHandle->fmt.width = m_objFFMPEG.getWidth(pHandle->hFFMpeg);
			pHandle->fmt.height = m_objFFMPEG.getHeight(pHandle->hFFMpeg);

			pHandle->fmt.pix_fmt = m_objFFMPEG.getFormat(pHandle->hFFMpeg);
			pHandle->fmt.frame_rate = m_objFFMPEG.getFrameRate(pHandle->hFFMpeg);
			pHandle->fmt.bit_rate = m_objFFMPEG.getBitRate(pHandle->hFFMpeg);

			pHandle->type = USE_FFMPEG_READ;
		} else 
#endif // #ifdef _EXPSR1_USE_FFMPEG
		{							// try OpenCV		
#ifdef _SR_USE_OPENCV
			pHandle->pOpenCVCap = cvCaptureFromFile( stFilename );
			if (pHandle->pOpenCVCap == NULL) {
				printf("Fail to open video file %s!\n", stFilename);
				closeVideo(&pHandle);
				return NULL;
			}
			pHandle->fmt.width = (int)cvGetCaptureProperty( pHandle->pOpenCVCap, CV_CAP_PROP_FRAME_WIDTH );
			pHandle->fmt.height = (int)cvGetCaptureProperty( pHandle->pOpenCVCap, CV_CAP_PROP_FRAME_HEIGHT );
			pHandle->fmt.pix_fmt = PIX_FMT_BGR24;		// no pixel format information, can only retrieve BGR image
			pHandle->fmt.frame_rate = (float)cvGetCaptureProperty( pHandle->pOpenCVCap, CV_CAP_PROP_FPS );
			//long codec = (long)cvGetCaptureProperty( pHandle->pOpenCVCap, CV_CAP_PROP_FOURCC );
			//char stCodec[16];	strncpy(stCodec, (char *)(&codec), 8); stCodec[4] = 0;
			pHandle->fmt.bit_rate = 0;

			pHandle->type = USE_OPENCV_READ;
#endif      // #ifdef _SR_USE_OPENCV
		} 
	}

	// allocate buffer for source video
 #ifdef _EXPSR1_USE_FFMPEG
	if (!m_objFFMPEG.createImages( pHandle->fmt.pix_fmt, pHandle->fmt.width, pHandle->fmt.height, 
								   &pHandle->iplImage, &pHandle->iplImageU, &pHandle->iplImageV )) {
#else
	if (!createImages( pHandle->fmt.pix_fmt, pHandle->fmt.width, pHandle->fmt.height, 
								   &pHandle->iplImage, &pHandle->iplImageU, &pHandle->iplImageV )) {
#endif //  #ifdef _EXPSR1_USE_FFMPEG
	   closeVideo(&pHandle);
	   return NULL;
	}

	return pHandle;
}

ReadWriteVideoHandle *CReadWriteVideo::openVideoWrite(char *stFilename, VideoFormat out_fmt)
// Open a video file for write
// NOTE: currently, this function only support uncompressed AVI, YUV (indexed), bmp, MPEG-1 and MPEG-2 video
// can compress WMV and RM format but can not correctly decoded (possibly the wrapper prolbem?)
// can not compress as H.264 because the ffmpeg used does not support H.264 encoding
// Develop based on reference code for ffmpeg, by Luhong Liang, IC-ASD, ASTRI, Nov. 30, 2011
{
	if (stFilename == NULL || stFilename[0] == 0) {
		printf("Invalide input file name found in CReadWriteVideo::openVideoWrite()!\n");
		return false;
	}

	ReadWriteVideoHandle *pHandle = new ReadWriteVideoHandle;
	if (pHandle == NULL) {
		printf("Fail to allocate a video handle in CReadWriteVideo::openVideoWrite()!\n");
		return NULL;
	}

	pHandle->initHandle();
	strncpy(pHandle->stFilename, stFilename, 255);

	// create  video
	//bool bOpenError = false;
	switch (out_fmt.codec) {
		case CODEC_IDX_YUV:
			// indexed YUV format
			if (out_fmt.pix_fmt != PIX_FMT_YUV420P && out_fmt.pix_fmt != PIX_FMT_YUYV422 && out_fmt.pix_fmt != PIX_FMT_UYVY422 ) {
				printf("Warning: only support YUV420P,YUYV422, UYVY422 format in CReadWriteVideo::openVideoWrite()!\n");
				printf("         Set pixel format to YUV420P.\n");
				out_fmt.pix_fmt = PIX_FMT_YUV420P;
			}
			pHandle->pYUV = new CReadWriteYUV();
			if (pHandle->pYUV == NULL) {
				printf("Fail to create an YUV object in CReadWriteVideo::openVideoWrite()!\n");
				closeVideo(&pHandle);
				return NULL;
			}
			if (!pHandle->pYUV->openIDX(stFilename, out_fmt.width, out_fmt.height, cvtPixelFormat_FFMPEG2YUV(out_fmt.pix_fmt), "w", out_fmt.start_frame_num) ) {
				closeVideo(&pHandle);
				return NULL;
			}
			pHandle->type = USE_IDX_WRITE;
			break;

#ifdef _EXPSR1_USE_FFMPEG
        case CODEC_UNCOMP_AVI:
			out_fmt.codec = CODEC_ID_RAWVIDEO;
			if ( !m_bFFMpegInitialized) {
				m_objFFMPEG.init();
				m_bFFMpegInitialized = true;
			}
			if (!m_objFFMPEG.openWrite(stFilename, out_fmt, pHandle->hFFMpeg)) {
				closeVideo(&pHandle);
				return NULL;
			}
			pHandle->type = USE_FFMPEG_WRITE;
            break;
#else
#ifdef _SR_USE_OPENCV   // by Luhong, 11/03/2015
		case CODEC_UNCOMP_AVI:
            if (out_fmt.pix_fmt != PIX_FMT_BGR24) {
				printf("Warning: set pixel format to BGR24 in CReadWriteVideo::openVideoWrite()!\n");
				out_fmt.pix_fmt = PIX_FMT_BGR24;	// this format is just interface to external, not control the pixel format in codec!
			}
			pHandle->pOpenCVWriter = cvCreateVideoWriter(stFilename, 0, out_fmt.frame_rate,  cvSize(out_fmt.width, out_fmt.height), 1);// uncompressed AVI
			if (pHandle->pOpenCVWriter == NULL) {
				closeVideo(&pHandle);
				return NULL;
			}
			pHandle->type = USE_OPENCV_WRITE;
            break;
#endif  // #ifdef _SR_USE_OPENCV
#endif //#ifdef _EXPSR1_USE_FFMPEG
		case CODEC_BMP_BGR:
		case CODEC_JPG_BGR:
			out_fmt.pix_fmt = PIX_FMT_BGR24;
			pHandle->type = USE_IMAGE_WRITE;
			break;
		case CODEC_BMP_Y:
			out_fmt.pix_fmt = PIX_FMT_GRAY8;
			pHandle->type = USE_IMAGE_WRITE;
			break;
#ifdef _EXPSR1_USE_FFMPEG
		case CODEC_ID_MPEG2VIDEO:
			// MPEG-2
			if ( !m_bFFMpegInitialized) {
				m_objFFMPEG.init();
				m_bFFMpegInitialized = true;
			}
			if (out_fmt.frame_rate != 25.0f && out_fmt.frame_rate != 30.0f && out_fmt.frame_rate != 50.0f && out_fmt.frame_rate != 60.0f) {
				if (out_fmt.frame_rate < 27.5f) {
                    out_fmt.frame_rate = 25.0f;
                } else if (out_fmt.frame_rate < 40.0f) {
                    out_fmt.frame_rate = 30.0f;
                } else if (out_fmt.frame_rate < 55.0f) {
                    out_fmt.frame_rate = 50.0f;
                } else {
                    out_fmt.frame_rate = 60.0f;
                }
                printf("Warning: modify frame rate to %.3f fps in CReadWriteVideo::openVideoWrite()!\n", out_fmt.frame_rate);
			}
			if (out_fmt.pix_fmt != PIX_FMT_YUV420P && out_fmt.pix_fmt != PIX_FMT_YUV422P) {		// MPEG-2 only support 420p and 422p
				printf("Warning: modified pixel format to 420P for MPEG-2 codec in CReadWriteVideo::openVideoWrite()!\n");
				out_fmt.pix_fmt = PIX_FMT_YUV420P;
			}
			if (!m_objFFMPEG.openWrite(stFilename, out_fmt, pHandle->hFFMpeg)) {
				closeVideo(&pHandle);
				return NULL;
			}
			pHandle->type = USE_FFMPEG_WRITE;
			break;
        case CODEC_ID_MPEG4:
			// MPEG-4
			if ( !m_bFFMpegInitialized) {
				m_objFFMPEG.init();
				m_bFFMpegInitialized = true;
			}
			if (out_fmt.pix_fmt != PIX_FMT_YUV420P && out_fmt.pix_fmt != PIX_FMT_YUV422P) {		// MPEG-2 only support 420p and 422p
				printf("Warning: modified pixel format to 420P for MPEG-2 codec in CReadWriteVideo::openVideoWrite()!\n");
				out_fmt.pix_fmt = PIX_FMT_YUV420P;
			}
			if (!m_objFFMPEG.openWrite(stFilename, out_fmt, pHandle->hFFMpeg)) {
				closeVideo(&pHandle);
				return NULL;
			}
			pHandle->type = USE_FFMPEG_WRITE;
			break;
		case CODEC_ID_MJPEG:
			// MJPEG
			if ( !m_bFFMpegInitialized) {
				m_objFFMPEG.init();
				m_bFFMpegInitialized = true;
			}
			//if (out_fmt.frame_rate != 25 && out_fmt.frame_rate != 30) {
			//	printf("Warning: modified frame rate to 25fps for MJPEG codec in CReadWriteVideo::openVideoWrite()!\n");
			//	out_fmt.frame_rate = 25;		// NOTE: MJPEG support 25fps and 30fps!
			//}
			if (out_fmt.pix_fmt != PIX_FMT_YUV420P && out_fmt.pix_fmt != PIX_FMT_YUV422P) {		// MJPEG only support 420p and 422p
				printf("Warning: modified pixel format to 420P for MJPEG codec in CReadWriteVideo::openVideoWrite()!\n");
				out_fmt.pix_fmt = PIX_FMT_YUV420P;
			}
			if (!m_objFFMPEG.openWrite(stFilename, out_fmt, pHandle->hFFMpeg)) {
				closeVideo(&pHandle);
				return NULL;
			}
			pHandle->type = USE_FFMPEG_WRITE;
			break;
		case CODEC_ID_WMV2:
			// WMV
			closeVideo(&pHandle);
			return NULL;
			break;
		case CODEC_ID_H264:
			// h.264/AVC
			closeVideo(&pHandle);
			return NULL;
			break;
		case CODEC_ID_RV20:
			// real media
			closeVideo(&pHandle);
			return NULL;
			break;
#endif // #ifdef _EXPSR1_USE_FFMPEG
		default:
			printf("Unsupported video format!\n");
			return NULL;
			closeVideo(&pHandle);
			break;
	}
	pHandle->fmt = out_fmt;

	// allocate buffer for source video
	//IplImage *iplImageY, *iplImageU, *iplImageV;
	if (!createImages( pHandle->fmt.pix_fmt, pHandle->fmt.width, pHandle->fmt.height, 
								    &pHandle->iplImage, &pHandle->iplImageU, &pHandle->iplImageV)) {
	   closeVideo(&pHandle);
	   return NULL;
	}
	//pHandle->iplImage = iplImageY;
	//pHandle->iplImageU = iplImageU;
	//pHandle->iplImageV = iplImageV;

	return pHandle;
}

bool CReadWriteVideo::decodeFrame(ReadWriteVideoHandle *pHandle, bool bRead4Jump)
// Read one frame from video file
// If bRead4Jump is true, it will not really read the YUV data but just add the frame counter
// Develop based on reference code for ffmpeg, by Luhong Liang, IC-ASD, ASTRI, Nov. 30, 2011
{
	if (pHandle == NULL) {
		printf("Invalid video handle found in CReadWriteVideo::decodeFrame()!\n");
		return false;
	}
	
	int nFrames;
	switch (pHandle->type) {
#ifdef _EXPSR1_USE_FFMPEG
		case USE_FFMPEG_READ:
			if (!m_objFFMPEG.getFrame(pHandle->hFFMpeg, pHandle->iplImage, pHandle->iplImageU, pHandle->iplImageV)) {
				return false;
			}
			break;
#endif // #ifdef _EXPSR1_USE_FFMPEG
#ifdef _SR_USE_OPENCV       // by Luhong, 11/03/2015
		case USE_OPENCV_READ:
			// load image
			nFrames = cvGrabFrame( pHandle->pOpenCVCap );
			if (nFrames < 1) {
				return false;
			}
			break;
#endif
		case USE_IDX_READ:
			if (!bRead4Jump) {
				if (!pHandle->pYUV->readFrame(pHandle->nFrameCounter, pHandle->iplImage, pHandle->iplImageU, pHandle->iplImageV)) {
					return false;
				}
			}
			break;
		default:
			printf("Not in video reading mode in CReadWriteVideo::decodeFrame()!\n");
			return false;
	}
	pHandle->nFrameCounter ++;

	return true;
}

bool CReadWriteVideo::retrieveFrame(ReadWriteVideoHandle *pHandle, IplImage **ppImageY, IplImage **ppImageU, IplImage **ppImageV)
// retrieve one frame in the internal buffer, for video read opertion only.
// NOTE: This function returns the pointers to the internal image buffers. Don't release those images.
// For packed images, (*ppImageY) will be a 3-channel image and (*ppImageU) and (*ppImageV) are NULLs
// For grey images, (*ppImageY) will be a 1-channel image and (*ppImageU) and (*ppImageV) are NULLs
// For planar images, all outputs are 1-channel images 
// Develop based on reference code for ffmpeg, by Luhong Liang, IC-ASD, ASTRI, Nov. 30, 2011
{
	if (pHandle == NULL) {
		printf("Invalid video handle found in CReadWriteVideo::retrieveFrame()!\n");
		return false;
	}

	if (ppImageY == NULL || ppImageU == NULL || ppImageU == NULL) {
		printf("Invalid input argument found in CReadWriteVideo::retrieveFrame()!\n");
		return false;
	}

	IplImage *iplSrc = NULL;	// for opencv
	switch (pHandle->type) {
#ifdef _SR_USE_OPENCV   // by Luhong, 11/03/2015
		case  USE_OPENCV_READ:
            //The function cvRetrieveFrame returns the pointer to the image grabbed with the GrabFrame
            // function. The returned image should not be released or modified by the user. In the event of an
            // error, the return value may be NULL.
			iplSrc = cvRetrieveFrame( pHandle->pOpenCVCap );
			if (iplSrc == NULL) {
					printf("Fail to retrieve frame in CReadWriteVideo::retrieveFrame()!\n");
					return false;
			}
			cvCopy(iplSrc, pHandle->iplImage);
            (*ppImageY) = pHandle->iplImage;
			break;
#endif      // #ifdef _SR_USE_OPENCV
		case USE_FFMPEG_READ:
		case USE_IDX_READ:
			(*ppImageY) = pHandle->iplImage;
			(*ppImageU) = pHandle->iplImageU;
			(*ppImageV) = pHandle->iplImageV;
			break;
		default:
			printf("Video handle is not in reading mode in CReadWriteVideo::retrieveFrame()!\n");
			return false;
	}

	return true;
}

bool CReadWriteVideo::getFrameBuffer(ReadWriteVideoHandle *pHandle, IplImage **ppImageY, IplImage **ppImageU, IplImage **ppImageV)
// Get points to the internal image buffer, for video save opertion only.
// NOTE: This function returns the pointers to the internal image buffers. Don't release those images.
// For packed images, (*ppImageY) will be a 3-channel image and (*ppImageU) and (*ppImageV) are NULLs
// For grey images, (*ppImageY) will be a 1-channel image and (*ppImageU) and (*ppImageV) are NULLs
// For planar images, all outputs are 1-channel images 
// Develop based on reference code for ffmpeg, by Luhong Liang, IC-ASD, ASTRI, Nov. 30, 2011
{
	if (pHandle == NULL) {
		printf("Invalid video handle found in CReadWriteVideo::getFrameBuffer()!\n");
		return false;
	}

	if (ppImageY == NULL || ppImageU == NULL || ppImageU == NULL) {
		printf("Invalid input argument found in CReadWriteVideo::getFrameBuffer()!\n");
		return false;
	}

	if (pHandle->type != USE_OPENCV_WRITE && pHandle->type != USE_FFMPEG_WRITE && 
		pHandle->type != USE_IDX_WRITE && pHandle->type != USE_IMAGE_WRITE) {
			printf("Video handle is not in writing mode in CReadWriteVideo::retrievalFrame()!\n");
			return false;
	}

	(*ppImageY) = pHandle->iplImage;
	(*ppImageU) = pHandle->iplImageU;
	(*ppImageV) = pHandle->iplImageV;

	return true;
}

bool CReadWriteVideo::writeFrame(ReadWriteVideoHandle *pHandle, int nFrameID)
// Write the data in the internal buffer to video. The pointers to the internal buffer can be get by getFrameBuffer(), and
// the buffer should be filled by caller before using this operation.
// Argument:
//    pHandle -- handle of the video file
//    nFrameID -- determine the index number in the suffix of the frame picture files. 
//                When nFrameID is -1, the index is determined by the internal counter and the 
//                start frame ID in the video handle (i.e. pHandle->nFrameCounter + pHandle->fmt.start_frame_num);
//                otherwise the index is determined by this parameter and the 
//                start frame ID in the video handle (i.e. nFrameID + pHandle->fmt.start_frame_num).
//                When the output is a set of image frame files. Generally, the output file name is
//                "xxxx####xxx.xxx", where '#'s represent the position of the frame
//                number, and 'x' are valid filename characters other than '#'. 
//                The frame number will replace the '#'s in the resultant filename.
//                The number of '#' determine the length of the frame number.
//                For example, the input file name is "Test_####_VEUHD.bmp", if 
//                the frame number is 16, the resultant file name is "Test_0016_VEUHD.bmp".
//                If the character '#' does not appear, the frame number will be placed
//                at the end of the file name with 6 characters. For example, if the input
//                file name is "Test.bmp", the resultant file will be "Test000016.bmp".
// , Nov. 30, 2011
{
	if (pHandle == NULL) {
		printf("Invalid video handle found in CReadWriteVideo::getFrameBuffer()!\n");
		return false;
	}

	if (pHandle->type != USE_OPENCV_WRITE && pHandle->type != USE_FFMPEG_WRITE && 
		pHandle->type != USE_IDX_WRITE && pHandle->type != USE_IMAGE_WRITE) {
			printf("Video handle is not in writing mode in CReadWriteVideo::retrievalFrame()!\n");
			return false;
	}

	// write frame
	char stTemp[256];//, stSuffix[32];
	switch (pHandle->fmt.codec) {
			case CODEC_IDX_YUV:
				// indexed YUV format
				if (!pHandle->pYUV->writeFrame(pHandle->iplImage, pHandle->iplImageU, pHandle->iplImageV))
					return false;
				break;
#ifdef _SR_USE_OPENCV   // by Luhong, 11/03/2015
			case CODEC_UNCOMP_AVI:
				// uncompressed AVI
				if (cvWriteFrame(pHandle->pOpenCVWriter, pHandle->iplImage) < 1)
					return false;
				break;
#endif      // #ifdef _SR_USE_OPENCV
			case CODEC_BMP_BGR:
			case CODEC_JPG_BGR:
				// pixel format conversion
                if (nFrameID >= 0) {
                    CImageUtility::addFrameNum(pHandle->stFilename, nFrameID+pHandle->fmt.start_frame_num, stTemp);
                    //sprintf(stSuffix, "_%06d", nFrameID);           // support 2 hours in general
                } else {
                    CImageUtility::addFrameNum(pHandle->stFilename, pHandle->nFrameCounter+pHandle->fmt.start_frame_num, stTemp);
                    //sprintf(stSuffix, "_%06d", pHandle->nFrameCounter);
                }
				//CImageUtility::addFileSuffix(pHandle->stFilename, stSuffix, stTemp);
				if (!CImageUtility::saveImage(stTemp, pHandle->iplImage))
					return false;
				break;
			case CODEC_BMP_Y:
				// pixel format conversion
                if (nFrameID >= 0) {
                    CImageUtility::addFrameNum(pHandle->stFilename, nFrameID+pHandle->fmt.start_frame_num, stTemp);
                    //sprintf(stSuffix, "_%06d", nFrameID);           // support 2 hours in general
                } else {
                    CImageUtility::addFrameNum(pHandle->stFilename, pHandle->nFrameCounter+pHandle->fmt.start_frame_num, stTemp);
                    //sprintf(stSuffix, "_%06d", pHandle->nFrameCounter);
                }
				//CImageUtility::addFileSuffix(pHandle->stFilename, stSuffix, stTemp);
				if (!CImageUtility::saveImage(stTemp, pHandle->iplImage))
					return false;
				break;
#ifdef _EXPSR1_USE_FFMPEG
			case CODEC_ID_RAWVIDEO:
				// uncompressed AVI
				if (!m_objFFMPEG.writeFrame(pHandle->hFFMpeg, pHandle->iplImage, pHandle->iplImageU, pHandle->iplImageV))
					return false;
				break;
			case CODEC_ID_MPEG2VIDEO:
				// MPEG-2
				if (!m_objFFMPEG.writeFrame(pHandle->hFFMpeg, pHandle->iplImage, pHandle->iplImageU, pHandle->iplImageV))
					return false;
				break;
			case CODEC_ID_MPEG4:
				// MPEG-4
				if (!m_objFFMPEG.writeFrame(pHandle->hFFMpeg, pHandle->iplImage, pHandle->iplImageU, pHandle->iplImageV))
					return false;
				break;
			case CODEC_ID_MJPEG:
				// MJPEG
				if (!m_objFFMPEG.writeFrame(pHandle->hFFMpeg, pHandle->iplImage, pHandle->iplImageU, pHandle->iplImageV))
					return false;
				break;
			case CODEC_ID_WMV2:
				// WMV
				return false;
				break;
			case CODEC_ID_H264:
				// h.264/AVC
				return false;
				break;
			case CODEC_ID_RV20:
				// real media
				return false;
				break;
#endif // #ifdef _EXPSR1_USE_FFMPEG
			default:
				return false;
				break;
		}

	pHandle->nFrameCounter ++;

	return true;
}

PixelFormat CReadWriteVideo::getFormat(ReadWriteVideoHandle *pHandle)
{
	if (pHandle == NULL) 
		return PIX_FMT_NONE;
	else 
		return pHandle->fmt.pix_fmt;
}

bool CReadWriteVideo::isChroma420(ReadWriteVideoHandle *pHandle)
// if it is YUV 4:2:0 or similar formats
// by Luhong Liang
// May 26, 2015
{
	if (pHandle == NULL) {
		return false;
    } else {
		if (pHandle->fmt.pix_fmt == PIX_FMT_YUV420P ||
            pHandle->fmt.pix_fmt == PIX_FMT_YUVJ420P ||
            pHandle->fmt.pix_fmt == PIX_FMT_NV12 ||
            pHandle->fmt.pix_fmt == PIX_FMT_NV21 ||
            pHandle->fmt.pix_fmt == PIX_FMT_YUVA420P ||
            pHandle->fmt.pix_fmt == PIX_FMT_YUV420P16LE ||
            pHandle->fmt.pix_fmt == PIX_FMT_YUV420P16BE) {
            return true;
        } else {
            return false;
        }
    }
}

bool CReadWriteVideo::isChroma422(ReadWriteVideoHandle *pHandle)
// if it is YUV 4:2:2 or similar formats
// by Luhong Liang
// May 26, 2015
{
	if (pHandle == NULL) {
		return false;
    } else {
		if (pHandle->fmt.pix_fmt == PIX_FMT_YUYV422 ||
            pHandle->fmt.pix_fmt == PIX_FMT_YUV422P ||
            pHandle->fmt.pix_fmt == PIX_FMT_YUVJ422P ||
            pHandle->fmt.pix_fmt == PIX_FMT_UYVY422 ||
            pHandle->fmt.pix_fmt == PIX_FMT_YUV422P16LE ||
            pHandle->fmt.pix_fmt == PIX_FMT_YUV422P16BE) {
            return true;
        } else {
            return false;
        }
    }
}

PixelFormat CReadWriteVideo::getResultFormat(ReadWriteVideoHandle *pHandle)
{
	if (pHandle == NULL) 
		return PIX_FMT_NONE;
	else 
		return pHandle->fmt.pix_fmt_rlt;
}

int CReadWriteVideo::getWidth(ReadWriteVideoHandle *pHandle)
{
	if (pHandle == NULL)
		return -1;
	else
		return pHandle->fmt.width;
}

int CReadWriteVideo::getHeight(ReadWriteVideoHandle *pHandle)
{
	if (pHandle == NULL)
		return -1;
	else
		return pHandle->fmt.height;
}

int CReadWriteVideo::getBitRate(ReadWriteVideoHandle *pHandle)
{
	if (pHandle == NULL)
		return -1;
	else
		return pHandle->fmt.bit_rate;
}

float CReadWriteVideo::getFrameRate(ReadWriteVideoHandle *pHandle)
{
	if (pHandle == NULL)
		return -1;
	else
		return pHandle->fmt.frame_rate;
}

bool CReadWriteVideo::getStartEndFrame(ReadWriteVideoHandle *pHandle,int &start, int &end, int &start_id)
{
	if (pHandle == NULL || pHandle->pYUV == NULL) {
		start = 0;
		end = -1;
		return false;
	}

	start = pHandle->pYUV->getStartFrame();
	end = pHandle->pYUV->getEndFrame();
    start_id = pHandle->pYUV->getStartIDShown();
	return true;
}

bool CReadWriteVideo::scaling( IplImage *iplSrcY, IplImage *iplSrcU, IplImage *iplSrcV, PixelFormat src_pix_fmt,
				                                     IplImage *iplDstY, IplImage *iplDstU, IplImage *iplDstV, PixelFormat dst_pix_fmt, int flags)
// Scale image frame using FFMPEG swscale.lib, providing fast copy for images with same format and size
// Arguments:
//			iplSrcY -- luma plane of planar image (should be 1 channel) or packed image (should be 3 channels)
//			iplSrcU -- chroma plane of planar image (should be 1 channel), should be NULL for grey or packed image
//			iplSrcV -- chroma plane of planar image (should be 1 channel), should be NULL for grey or packed image
//			src_pix_fmt -- indicates the format of source image, defined by FFMPEG
//			iplDstY -- luma plane of planar image (should be 1 channel) or packed image (should be 3 channels)
//			iplDstU -- chroma plane of planar image (should be 1 channel), should be NULL for grey or packed image
//			iplDstV -- chroma plane of planar image (should be 1 channel), should be NULL for grey or packed image
//			dst_pix_fmt -- indicates the format of destination image, defined by FFMPEG
//			flags -- flag indicate the scaling approach, defined by FFMPEG
// flags: SWS_POINT |SWS_AREA|SWS_BILINEAR|SWS_FAST_BILINEAR|SWS_BICUBIC|SWS_X|SWS_GAUSS|SWS_LANCZOS|SWS_SINC|SWS_SPLINE|SWS_BICUBLIN
// Develop based on reference code for ffmpeg, by Luhong Liang, IC-ASD, ASTRI, Nov. 30, 2011
{
	if (iplSrcY == NULL || iplDstY == NULL) {
		printf("Invalid input/output image in CReadWriteVideo::scaling()!\n");
		return false;
	}

	// check format
	bool bError = false;
	switch (src_pix_fmt) {
		case PIX_FMT_YUV420P:
        case PIX_FMT_YUVJ420P:      //< planar YUV 4:2:0, 12bpp, full scale (JPEG)
		case PIX_FMT_YUV422P:
        case PIX_FMT_YUVJ422P:      // planar YUV 4:2:2, 16bpp, full scale (JPEG)
		case PIX_FMT_YUYV422:		// all YUV files include the one in packed format are stored internally in planar
		case PIX_FMT_UYVY422:	
		case PIX_FMT_YUV444P:
        case PIX_FMT_YUVJ444P:      //< planar YUV 4:4:4, 24bpp, full scale (JPEG)
			if (iplSrcY->nChannels != 1 || iplSrcU == NULL || iplSrcU->nChannels != 1 || iplSrcV == NULL || iplSrcV->nChannels != 1)
				bError = true;
			break;
		case PIX_FMT_GRAY8:
			if (iplSrcY->nChannels != 1 || iplSrcU != NULL || iplSrcV != NULL) 
				bError = true;
			break;
		case PIX_FMT_RGB24:
		case PIX_FMT_BGR24:
			if (iplSrcY->nChannels != 3 || iplSrcU != NULL || iplSrcV != NULL)
				bError = true;
			break;
		default:
			// TODO more checks
			break;
	}

	switch (dst_pix_fmt) {
		case PIX_FMT_YUV420P:
		case PIX_FMT_YUV422P:
		case PIX_FMT_YUYV422:		// all YUV files include the one in packed format are stored internally in planar
		case PIX_FMT_UYVY422:
		case PIX_FMT_YUV444P:
			if (iplDstY->nChannels != 1 || iplDstU == NULL || iplDstU->nChannels != 1 || iplDstV == NULL || iplDstV->nChannels != 1)
				bError = true;
			break;
		case PIX_FMT_GRAY8:
			if (iplDstY->nChannels != 1 || iplDstU != NULL || iplDstV != NULL) 
				bError = true;
			break;
		case PIX_FMT_RGB24:
		case PIX_FMT_BGR24:
			if (iplDstY->nChannels != 3 || iplDstU != NULL || iplDstV != NULL)
				bError = true;
			break;
		default:
			// TODO more checks
			break;
	}

	if (bError) {
		printf("Invalid/unsupported input/output image in CReadWriteVideo::scaling()!\n");
		return false;
	}
	
	// scaling and format conversion
	if (iplSrcY->width == iplDstY->width  && iplSrcY->height == iplDstY->height && dst_pix_fmt == src_pix_fmt) {
        CImageUtility::copy(iplSrcY, iplDstY);
		if (iplSrcU != NULL && iplSrcV != NULL && iplDstU != NULL && iplDstV != NULL) {
			CImageUtility::copy(iplSrcU, iplDstU);
			CImageUtility::copy(iplSrcV, iplDstV);
		}
		return true;
	}

#ifdef _EXPSR1_USE_FFMPEG
	if (!m_objFFMPEG.scaling(iplSrcY, iplSrcU, iplSrcV,src_pix_fmt, iplDstY, iplDstU, iplDstV, dst_pix_fmt, flags)) return false;
#else  // #ifdef _EXPSR1_USE_FFMPEG
	if (src_pix_fmt == PIX_FMT_YUV420P || src_pix_fmt == PIX_FMT_YUVJ420P ||
        src_pix_fmt == PIX_FMT_YUV422P || src_pix_fmt == PIX_FMT_YUVJ422P ||
        src_pix_fmt == PIX_FMT_YUYV422 || src_pix_fmt == PIX_FMT_UYVY422 ||
        src_pix_fmt == PIX_FMT_YUV444P || src_pix_fmt == PIX_FMT_YUVJ444P) {
		if (dst_pix_fmt == PIX_FMT_BGR24) {
			if (!CImageUtility::cvtYUVtoBGR_8U(iplSrcY, iplSrcU, iplSrcV, iplDstY, CHROMA_YUVMS)) {
				return false;
			}
		} else if (dst_pix_fmt == PIX_FMT_RGB24) {
			if (!CImageUtility::cvtYUVtoRGB_8U(iplSrcY, iplSrcU, iplSrcV, iplDstY, CHROMA_YUVMS)) {
				return false;
			}
		} else if (dst_pix_fmt == PIX_FMT_YUV420P || dst_pix_fmt == PIX_FMT_YUV422P ||
                   dst_pix_fmt == PIX_FMT_YUV444P) {
			if (!CImageUtility::scaling(iplSrcY, iplSrcU, iplSrcV, iplDstY, iplDstU, iplDstV)) {
				return false;
			}		
		} else {
			printf("Unsupported output video format in CReadWriteVideo::scaling()!\n");
			return false;
		}
	} else {
		printf("Unsupported input video format in CReadWriteVideo::scaling()!\n");
		return false;
	}
#endif // #ifdef _EXPSR1_USE_FFMPEG

	return true;
}


bool CReadWriteVideo::closeVideo(ReadWriteVideoHandle **ppHandle)
{
	if (ppHandle != NULL && (*ppHandle)!= NULL) {
		switch ((*ppHandle)->type) {
#ifdef _EXPSR1_USE_FFMPEG
			case USE_FFMPEG_READ:
				m_objFFMPEG.closeRead((*ppHandle)->hFFMpeg);
				break;
			case USE_FFMPEG_WRITE:
				m_objFFMPEG.closeWrite((*ppHandle)->hFFMpeg);
				break;
#endif // #ifdef _EXPSR1_USE_FFMPEG
#ifdef _SR_USE_OPENCV   // by Luhong, 11/03/2015
			case USE_OPENCV_READ:
				cvReleaseCapture( &(*ppHandle)->pOpenCVCap );
				break;
			case USE_OPENCV_WRITE:
				cvReleaseVideoWriter( &(*ppHandle)->pOpenCVWriter );
				break;
#endif // #ifdef _SR_USE_OPENCV
			case USE_IDX_READ:
			case USE_IDX_WRITE:
				(*ppHandle)->pYUV->close();
				delete (*ppHandle)->pYUV;
				break;
			case USE_IMAGE_WRITE:
			default:
				break;
		}

		if ((*ppHandle)->iplImage != NULL) CImageUtility::safeReleaseImage(&(*ppHandle)->iplImage);
		if ((*ppHandle)->iplImageU != NULL) CImageUtility::safeReleaseImage(&(*ppHandle)->iplImageU);
		if ((*ppHandle)->iplImageV != NULL) CImageUtility::safeReleaseImage(&(*ppHandle)->iplImageV);

		delete (*ppHandle);
		(*ppHandle) = NULL;
	}

	return true;
}

PixelFormat CReadWriteVideo::cvtPixelFormat_YUV2FFMPEG(YUVFormat fmt) 
{
	switch (fmt) {
		case IDX_YUV420P:
			return PIX_FMT_YUV420P;
		case IDX_YUV422P:
			return PIX_FMT_YUV422P;
		case IDX_YUYV422:
			return PIX_FMT_YUYV422;
		case IDX_UYVY422:
			return PIX_FMT_UYVY422;
		case IDX_YUV444P:
        case IDX_YUVMS444:
			return PIX_FMT_YUV444P;
		case IDX_YUVGrey:
			return PIX_FMT_GRAY8;
		default:
			printf("Unsupport pixel format!\n");
			return PIX_FMT_NONE;
	}
}

YUVFormat CReadWriteVideo::cvtPixelFormat_FFMPEG2YUV(PixelFormat fmt) 
{
	switch (fmt) {
		case PIX_FMT_YUV420P:
        case PIX_FMT_YUVJ420P:      //< planar YUV 4:2:0, 12bpp, full scale (JPEG)
			return IDX_YUV420P;
		case PIX_FMT_YUV422P:
        case PIX_FMT_YUVJ422P:      // planar YUV 4:2:2, 16bpp, full scale (JPEG)
			return IDX_YUV422P;
		case PIX_FMT_YUV444P:
        case PIX_FMT_YUVJ444P:      //< planar YUV 4:4:4, 24bpp, full scale (JPEG)
			return IDX_YUV444P;
		case PIX_FMT_GRAY8:
			return IDX_YUVGrey;
		case PIX_FMT_UYVY422:
			return IDX_UYVY422C;
		case PIX_FMT_YUYV422:
			return IDX_YUYV422;
		default:
			printf("Unsupport pixel format!\n");
			return IDX_YUV_Unknown;
	}
}

int CReadWriteVideo::getCodecFromExt(char stExt[])
{
	if (strcmp(strlwr(stExt), "idx") == 0) {
		return CODEC_IDX_YUV;
	} else if (strcmp(strlwr(stExt), "bmp") == 0) {
		return CODEC_BMP_BGR;

#ifdef _USE_MY_DPX_DECODER
	} else if (strcmp(strlwr(stExt), "dpx") == 0) {
		return CODEC_BMP_BGR;
#endif  // #ifdef _USE_MY_DPX_DECODER

#ifdef  _SR_USE_OPENCV
	} else if (strcmp(strlwr(stExt), "avi") == 0) {
		return CODEC_UNCOMP_AVI;
	} else if (strcmp(strlwr(stExt), "jpg") == 0) {
		return CODEC_JPG_BGR;
	} else if (strcmp(strlwr(stExt), "jpeg") == 0) {
		return CODEC_JPG_BGR;
#endif      // #ifdef  _SR_USE_OPENCV;

#ifdef _EXPSR1_USE_FFMPEG
	} else if (strcmp(strlwr(stExt), "mpeg") == 0) {
		return CODEC_ID_MPEG2VIDEO;
	} else if (strcmp(strlwr(stExt), "mjpg") == 0) {
		return CODEC_ID_MJPEG;
	} else if (strcmp(strlwr(stExt), "mp4") == 0) {
		return CODEC_ID_MPEG4;
#endif // #ifdef _EXPSR1_USE_FFMPEG

	} else {
		// CODEC_ID_WMV2
		//CODEC_ID_H264
		//CODEC_ID_RV20
		//CODEC_BMP_Y
		printf("Unsupported video fromat!\n");
		return CODEC_ID_NONE;
	}
}

int CReadWriteVideo::getDefaultBitrate(char stExt[], int width, int height, float frame_rate)
{
    int codec_id = getCodecFromExt(stExt);
    return getDefaultBitRate(codec_id, width, height, frame_rate);
}

int CReadWriteVideo::getDefaultBitRate(int codec_id, int width, int height, float frame_rate)
{
    int bit_rate = 0;
    if (frame_rate > 60.0f) {
        printf("Only support frame rate less than or equal to 60!\n");
        frame_rate = 60.0f;
    } 
    if (frame_rate < 10.0f) {
        printf("Only support frame rate larger than or equal to 10!\n");
        frame_rate = 10.0f;
    } 
	switch(codec_id) {
        case CODEC_IDX_YUV:
            bit_rate = (int)((width * height + (width * height) / 2) * frame_rate);
            break;
        case CODEC_UNCOMP_AVI:
            bit_rate = (int)((width * height + (width * height) / 2) * frame_rate);
            break;
        case CODEC_BMP_BGR:
            bit_rate = (int)(width * height * 3 * frame_rate);
            break;
        case CODEC_JPG_BGR:
            bit_rate = (int)((width * height / 2) * frame_rate);
            break;
#ifdef _EXPSR1_USE_FFMPEG
        case CODEC_ID_MPEG2VIDEO:
            if ((width * height) < (1280*720)) {
                bit_rate = 8000000;
            } else {
                bit_rate = (int)(((width * height) / (1280*720)) * 8000000);
            }
            break;
        case CODEC_ID_MPEG4:
            if ((width * height) < (1280*720)) {
                bit_rate = 5000000;
            } else {
                bit_rate = (int)(((width * height) / (1280*720)) * 5000000);
            }
            break;
        case CODEC_ID_MJPEG:
            bit_rate = (int)((width * height / 2) * frame_rate);
            break;
#endif // #ifdef _EXPSR1_USE_FFMPEG
        default:
		// CODEC_ID_WMV2
		//CODEC_ID_H264
		//CODEC_ID_RV20
		//CODEC_BMP_Y
		printf("Unsupported video fromat!\n");
	}

    return bit_rate;
}

bool CReadWriteVideo::isPlanar(PixelFormat pix_fmt)
{
#ifdef _EXPSR1_USE_FFMPEG
	return m_objFFMPEG.isPlanar(pix_fmt);
#else
	switch (pix_fmt) {
		case PIX_FMT_YUV420P:   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
			return true;
		case PIX_FMT_YUYV422:   ///< packed YUV 4:2:2: 16bpp, Y0 Cb Y1 Cr
		case PIX_FMT_RGB24:     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
		case PIX_FMT_BGR24:     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
			return false;
		case PIX_FMT_YUV422P:   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
		case PIX_FMT_YUV444P:   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
		case PIX_FMT_YUV410P:   ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
		case PIX_FMT_YUV411P:   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
		case PIX_FMT_GRAY8:     ///<        Y        ,  8bpp
		case PIX_FMT_MONOWHITE: ///<        Y        ,  1bpp, 0 is white, 1 is black, in each byte pixels are ordered from the msb to the lsb
		case PIX_FMT_MONOBLACK: ///<        Y        ,  1bpp, 0 is black, 1 is white, in each byte pixels are ordered from the msb to the lsb
		case PIX_FMT_PAL8:      ///< 8 bit with PIX_FMT_RGB32 palette
		case PIX_FMT_YUVJ420P:  ///< planar YUV 4:2:0, 12bpp, full scale (JPEG)
		case PIX_FMT_YUVJ422P:  ///< planar YUV 4:2:2, 16bpp, full scale (JPEG)
		case PIX_FMT_YUVJ444P:  ///< planar YUV 4:4:4, 24bpp, full scale (JPEG)
			return true;
		case PIX_FMT_XVMC_MPEG2_MC:///< XVideo Motion Acceleration via common packet passing
		case PIX_FMT_XVMC_MPEG2_IDCT:
		case PIX_FMT_UYVY422:   ///< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
		case PIX_FMT_UYYVYY411: ///< packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
		case PIX_FMT_BGR8:      ///< packed RGB 3:3:2,  8bpp, (msb)2B 3G 3R(lsb)
		case PIX_FMT_BGR4:      ///< packed RGB 1:2:1 bitstream,  4bpp, (msb)1B 2G 1R(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
		case PIX_FMT_BGR4_BYTE: ///< packed RGB 1:2:1,  8bpp, (msb)1B 2G 1R(lsb)
		case PIX_FMT_RGB8:      ///< packed RGB 3:3:2,  8bpp, (msb)2R 3G 3B(lsb)
		case PIX_FMT_RGB4:      ///< packed RGB 1:2:1 bitstream,  4bpp, (msb)1R 2G 1B(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
		case PIX_FMT_RGB4_BYTE: ///< packed RGB 1:2:1,  8bpp, (msb)1R 2G 1B(lsb)
			return false;
		case PIX_FMT_NV12:      ///< planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
		case PIX_FMT_NV21:      ///< as above, but U and V bytes are swapped
			return true;
		case PIX_FMT_ARGB:      ///< packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
		case PIX_FMT_RGBA:      ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
		case PIX_FMT_ABGR:      ///< packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
		case PIX_FMT_BGRA:      ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
			return false;
		case PIX_FMT_GRAY16BE:  ///<        Y        , 16bpp, big-endian
		case PIX_FMT_GRAY16LE:  ///<        Y        , 16bpp, little-endian
		case PIX_FMT_YUV440P:   ///< planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
		case PIX_FMT_YUVJ440P:  ///< planar YUV 4:4:0 full scale (JPEG)
		case PIX_FMT_YUVA420P:  ///< planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
		case PIX_FMT_VDPAU_H264:///< H.264 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
			return true;
		case PIX_FMT_VDPAU_MPEG1:///< MPEG-1 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
		case PIX_FMT_VDPAU_MPEG2:///< MPEG-2 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
		case PIX_FMT_VDPAU_WMV3:///< WMV3 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
		case PIX_FMT_VDPAU_VC1: ///< VC-1 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
		case PIX_FMT_RGB48BE:   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as big-endian
		case PIX_FMT_RGB48LE:   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as little-endian

		case PIX_FMT_RGB565BE:  ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), big-endian
		case PIX_FMT_RGB565LE:  ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), little-endian
		case PIX_FMT_RGB555BE:  ///< packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), big-endian, most significant bit to 0
		case PIX_FMT_RGB555LE:  ///< packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), little-endian, most significant bit to 0

		case PIX_FMT_BGR565BE:  ///< packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), big-endian
		case PIX_FMT_BGR565LE:  ///< packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), little-endian
		case PIX_FMT_BGR555BE:  ///< packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), big-endian, most significant bit to 1
		case PIX_FMT_BGR555LE:  ///< packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), little-endian, most significant bit to 1

		case PIX_FMT_VAAPI_MOCO: ///< HW acceleration through VA API at motion compensation entry-point, Picture.data[3] contains a vaapi_render_state struct which contains macroblocks as well as various fields extracted from headers
		case PIX_FMT_VAAPI_IDCT: ///< HW acceleration through VA API at IDCT entry-point, Picture.data[3] contains a vaapi_render_state struct which contains fields extracted from headers
		case PIX_FMT_VAAPI_VLD:  ///< HW decoding through VA API, Picture.data[3] contains a vaapi_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
			return false;

		case PIX_FMT_YUV420P16LE:  ///< planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
		case PIX_FMT_YUV420P16BE:  ///< planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
		case PIX_FMT_YUV422P16LE:  ///< planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
		case PIX_FMT_YUV422P16BE:  ///< planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
		case PIX_FMT_YUV444P16LE:  ///< planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
		case PIX_FMT_YUV444P16BE:  ///< planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
			return true;
		case PIX_FMT_VDPAU_MPEG4:  ///< MPEG4 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
		case PIX_FMT_DXVA2_VLD:    ///< HW decoding through DXVA2, Picture.data[3] contains a LPDIRECT3DSURFACE9 pointer

		case PIX_FMT_RGB444BE:  ///< packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), big-endian, most significant bits to 0
		case PIX_FMT_RGB444LE:  ///< packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), little-endian, most significant bits to 0
		case PIX_FMT_BGR444BE:  ///< packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), big-endian, most significant bits to 1
		case PIX_FMT_BGR444LE:  ///< packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), little-endian, most significant bits to 1
		case PIX_FMT_Y400A:     ///< 8bit gray, 8bit alpha
		case PIX_FMT_NB:        ///< number of pixel formats, DO NOT USE THIS if you want to link with shared libav* because the number of formats might differ between versions
			return false;
	}

	return false;
#endif // #ifdef _EXPSR1_USE_FFMPEG
}

bool CReadWriteVideo::isPackedYUV(PixelFormat pix_fmt)
{
#ifdef _EXPSR1_USE_FFMPEG
	return m_objFFMPEG.isPackedYUV(pix_fmt);
#else
	switch (pix_fmt) {
		case PIX_FMT_YUYV422:   ///< packed YUV 4:2:2: 16bpp, Y0 Cb Y1 Cr
		case PIX_FMT_UYVY422:   ///< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
		case PIX_FMT_UYYVYY411: ///< packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
			return true;
		default:
			return false;
	}

	return false;
#endif // #ifdef _EXPSR1_USE_FFMPEG
}

bool CReadWriteVideo::createImages(PixelFormat fmt, int width, int height, IplImage **ppImageY, IplImage **ppImageU, IplImage **ppImageV)
// create image according to pixel format
// For packed images, (*ppImageY) will be a 3-channel image and (*ppImageU) and (*ppImageV) are NULLs (for UYVY422C, still 3 images)
// For grey images, (*ppImageY) will be a 1-channel image and (*ppImageU) and (*ppImageV) are NULLs
// For planar images, all outputs are 1-channel images 
{
	// check arguments
	if (ppImageY == NULL || ppImageU == NULL || ppImageV == NULL) {
		message("Invalid input video handle or image buffer in CReadWriteVideo::createImages()!\n");
		return false;
	}

	*ppImageY = NULL;
	*ppImageU = NULL;
	*ppImageV = NULL;

	int nLumaW = width;
	int nLumaH = height;
	int nChromaW, nChromaH, nChannels=1;
	switch (fmt) {
		case PIX_FMT_YUV420P:
        case PIX_FMT_YUVJ420P:      //< planar YUV 4:2:0, 12bpp, full scale (JPEG)
			nChromaW = nLumaW / 2;
			nChromaH = nLumaH / 2;
			break;
		case PIX_FMT_YUV422P:	    // all YUV files include the one in packed format are stored internally in planar
        case PIX_FMT_YUVJ422P:      // planar YUV 4:2:2, 16bpp, full scale (JPEG)
		case PIX_FMT_YUYV422:
		case PIX_FMT_UYVY422:
			nChromaW = nLumaW / 2;
			nChromaH = nLumaH;
			break;
		case PIX_FMT_YUV444P:
        case PIX_FMT_YUVJ444P:      //< planar YUV 4:4:4, 24bpp, full scale (JPEG)
			nChromaW = nLumaW;
			nChromaH = nLumaH;
			break;
		case PIX_FMT_GRAY8:
			nChannels = 1;
			nChromaW = 0;
			nChromaH = 0;
			break;
		// planar formats
		case PIX_FMT_RGB24:
		case PIX_FMT_BGR24:
			nChannels = 3;
			nChromaW = 0;
			nChromaH = 0;
			break;
		case PIX_FMT_ARGB:      ///< packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
		case PIX_FMT_RGBA:      ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
		case PIX_FMT_ABGR:      ///< packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
		case PIX_FMT_BGRA:      ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
			nChannels = 4;
			nChromaW = 0;
			nChromaH = 0;
			break;
		default:
			message("Unsupported pixel format found in CReadWriteVideo::createImages()!\n");
			return false;
	}

	// allocate buffer for packed image
	if (!isPlanar(fmt) && !isPackedYUV(fmt)) {
		(*ppImageY) = CImageUtility::createImage(nLumaW, nLumaH, SR_DEPTH_8U, nChannels);
		if ((*ppImageY) == NULL) {
			message("Fail to allocate images in CReadWriteVideo::createImages()!\n");
			return false;
		}
		return true;
	}

	// allocate buffer for planar image
	(*ppImageY) = CImageUtility::createImage(nLumaW, nLumaH, SR_DEPTH_8U, 1);
	if ((*ppImageY) == NULL) {
		message("Fail to allocate images in CReadWriteVideo::createImages()!\n");
		return false;
	}
	if (nChromaW != 0 && nChromaH != 0) {
		(*ppImageU) = CImageUtility::createImage(nChromaW, nChromaH, SR_DEPTH_8U, 1);
		(*ppImageV) = CImageUtility::createImage(nChromaW, nChromaH, SR_DEPTH_8U, 1);
		if ((*ppImageU) == NULL || (*ppImageV) == NULL) {
			message("Fail to allocate images in CReadWriteVideo::createImages()!\n");
			if ((*ppImageY) != 0) CImageUtility::safeReleaseImage(ppImageY);
			if ((*ppImageU) != 0) CImageUtility::safeReleaseImage(ppImageU);
			if ((*ppImageV) != 0) CImageUtility::safeReleaseImage(ppImageV);
			return false;
		}
	}

	return true;
}
