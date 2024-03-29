// Example-based Super-resolution
// Luhong Liang, IC-ASD, ASTRI
// Nov. 27, 2011

#include "stdafx.h"
#include <string.h>

#ifdef _EXPSR1_USE_FFMPEG

#include "ReadWriteVideoFFMPEG.h"

#define _FFMPEG_ENCODER_OUT_BUFFER_SIZE	100000

CReadWriteVideoFFMPEG::CReadWriteVideoFFMPEG(void)
{
	m_bInitialized = false;
}

CReadWriteVideoFFMPEG::~CReadWriteVideoFFMPEG(void)
{
	m_bInitialized = false;
}

void CReadWriteVideoFFMPEG::message(char stMsg[])
{
	printf("%s", stMsg);
}

bool CReadWriteVideoFFMPEG::init(void)
{
	if (!m_bInitialized) {
		// must be called before using avcodec lib
		//avcodec_init();

		// This registers all available file formats and codecs with the library so they will be used automatically
		av_register_all();

		m_bInitialized = true;
	}

	return true;
}

bool CReadWriteVideoFFMPEG::openRead(char *stFilename, FFMpegVideoHandle &hVideo)
// Open a video file for read 
// Develop based on reference code for ffmpeg, by Luhong Liang, IC-ASD, ASTRI, Nov. 28, 2011
{
	// check status
	if (!m_bInitialized) {
		message("Codec not initialized found in CReadWriteVideoFFMPEG::openRead()!\n");
		return false;
	}
	if (stFilename == NULL || stFilename[0] == 0) {
		message("Invalide input file name found in CReadWriteVideoFFMPEG::openRead()!\n");
		return false;
	}
	hVideo.initHandle();

	// open file
	if(av_open_input_file(&(hVideo.pFormatCtx), stFilename, NULL, 0, NULL)!=0) {
		char stMessage[256];
		sprintf(stMessage, "Fail to open video file %s in CReadWriteVideoFFMPEG::openRead()!\n", stFilename);
		message(stMessage);
		return false;
	}

	// Retrieve stream information
	if(av_find_stream_info(hVideo.pFormatCtx)<0) {
		char stMessage[256];
		sprintf(stMessage, "Fail to find stream information in video file %s in CReadWriteVideoFFMPEG::openRead()!\n", stFilename);
		message(stMessage);
		closeRead(hVideo);
		return false;
	}

	// Dump information about file onto standard error
	//dump_format(pFormatCtx, 0, argv[1], 0);	// pFormatCtx->streams is just an array of pointers, of size pFormatCtx->nb_streams

	// Find the first video stream
	for(unsigned i=0; i<(hVideo.pFormatCtx)->nb_streams; i++) {
		if((hVideo.pFormatCtx)->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) {
			hVideo.videoStream=i;
			break;
		}
	}
	if(hVideo.videoStream == -1) {
		char stMessage[256];
		sprintf(stMessage, "Fail to find a video stream in file %s in CReadWriteVideoFFMPEG::openRead()!\n", stFilename);
		message(stMessage);
		closeRead(hVideo);
		return false;
	}

	// Get a pointer to the codec context for the video stream
	hVideo.pCodecCtx = (hVideo.pFormatCtx)->streams[hVideo.videoStream]->codec;		// "codec context" contains all the information about the codec that the stream is using

	// Find the decoder for the video stream
	AVCodec *pCodec;
	pCodec = avcodec_find_decoder(hVideo.pCodecCtx->codec_id);
	if(pCodec==NULL) {
		char stMessage[256];
		sprintf(stMessage, "No codec for video stream in file %s in CReadWriteVideoFFMPEG::openRead()!\n", stFilename);
		message(stMessage);
		closeRead(hVideo);
		return false;
	}
	
	// Open codec
	if(avcodec_open(hVideo.pCodecCtx, pCodec)<0) {
		char stMessage[256];
		sprintf(stMessage, "Can not open codec for video stream in file %s in CReadWriteVideoFFMPEG::openRead()!\n", stFilename);
		message(stMessage);
		closeRead(hVideo);
		return false;
	}

	// Allocate buffer to store the data
	hVideo.pFrame=avcodec_alloc_frame();
	if (hVideo.pFrame == NULL) {
		message("Fail to allocate buffer in CReadWriteVideoFFMPEG::openRead()!\n");
		closeRead(hVideo);
		return false;
	}

	return true;
}

PixelFormat CReadWriteVideoFFMPEG::getFormat(FFMpegVideoHandle hVideo) 
{
	if (!m_bInitialized || hVideo.pCodecCtx == NULL) {
		return PIX_FMT_NONE;
	}

	return hVideo.pCodecCtx->pix_fmt;
}

int CReadWriteVideoFFMPEG::getWidth(FFMpegVideoHandle hVideo) 
{
	if (!m_bInitialized || hVideo.pCodecCtx == NULL) {
		return -1;
	}

	return hVideo.pCodecCtx->width;
}

int CReadWriteVideoFFMPEG::getHeight(FFMpegVideoHandle hVideo) 
{
	if (!m_bInitialized || hVideo.pCodecCtx == NULL) {
		return -1;
	}

	return hVideo.pCodecCtx->height;
}

int CReadWriteVideoFFMPEG::getBitRate(FFMpegVideoHandle hVideo) 
{
	if (!m_bInitialized || hVideo.pCodecCtx == NULL) {
		return -1;
	}

	return hVideo.pCodecCtx->bit_rate;
}

float CReadWriteVideoFFMPEG::getFrameRate(FFMpegVideoHandle hVideo)
{
	if (!m_bInitialized || hVideo.pCodecCtx == NULL) {
		return -1;
	}

	if (hVideo.pCodecCtx->time_base.num < 1 || hVideo.pCodecCtx->time_base.den < 1)
		return 25.0f;

	float fRate = (float)hVideo.pCodecCtx->time_base.den / (float)hVideo.pCodecCtx->time_base.num;
	if (fRate < 0.01) 
		return 25.0f;

	return fRate;
}

bool CReadWriteVideoFFMPEG::createImages(FFMpegVideoHandle hVideo, IplImage **ppImageY, IplImage **ppImageU, IplImage **ppImageV)
{
	// check status
	if (!m_bInitialized) {
		message("Codec not initialized found in CReadWriteVideoFFMPEG::createImages()!\n");
		return false;
	}

	return createImages(getFormat(hVideo), hVideo.pCodecCtx->width, hVideo.pCodecCtx->height, ppImageY, ppImageU, ppImageV);
}

bool CReadWriteVideoFFMPEG::createImages(PixelFormat fmt, int width, int height, IplImage **ppImageY, IplImage **ppImageU, IplImage **ppImageV)
// create image according to pixel format
// For packed images, (*ppImageY) will be a 3-channel image and (*ppImageU) and (*ppImageV) are NULLs
// For grey images, (*ppImageY) will be a 1-channel image and (*ppImageU) and (*ppImageV) are NULLs
// For planar images, all outputs are 1-channel images 
{
	// check arguments
	if (ppImageY == NULL || ppImageU == NULL || ppImageV == NULL) {
		message("Invalid input video handle or image buffer in CReadWriteVideoFFMPEG::createImages()!\n");
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
        case PIX_FMT_YUVJ420P:  //< planar YUV 4:2:0, 12bpp, full scale (JPEG)
			nChromaW = nLumaW / 2;
			nChromaH = nLumaH / 2;
			break;
		case PIX_FMT_YUV422P:	// all YUV files include the one in packed format are stored internally in planar
		case PIX_FMT_YUYV422:
		case PIX_FMT_UYVY422:
        case PIX_FMT_YUVJ422P:  //< planar YUV 4:2:2, 16bpp, full scale (JPEG)
			nChromaW = nLumaW / 2;
			nChromaH = nLumaH;
			break;
		case PIX_FMT_YUV444P:
        case PIX_FMT_YUVJ444P:  //< planar YUV 4:4:4, 24bpp, full scale (JPEG)
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
			message("Unsupported pixel format found in CReadWriteVideoFFMPEG::createImages()!\n");
			return false;
	}

	// allocate buffer for packed image
	if (!isPlanar(fmt) && !isPackedYUV(fmt)) {
		(*ppImageY) = CImageUtility::createImage(nLumaW, nLumaH, SR_DEPTH_8U, nChannels);
		if ((*ppImageY) == NULL) {
			message("Fail to allocate images in CReadWriteVideoFFMPEG::createImages()!\n");
			return false;
		}
		return true;
	}

	// allocate buffer for planar image
	(*ppImageY) = CImageUtility::createImage(nLumaW, nLumaH, SR_DEPTH_8U, 1);
	if ((*ppImageY) == NULL) {
		message("Fail to allocate images in CReadWriteVideoFFMPEG::createImages()!\n");
		return false;
	}
	if (nChromaW != 0 && nChromaH != 0) {
		(*ppImageU) = CImageUtility::createImage(nChromaW, nChromaH, SR_DEPTH_8U, 1);
		(*ppImageV) = CImageUtility::createImage(nChromaW, nChromaH, SR_DEPTH_8U, 1);
		if ((*ppImageU) == NULL || (*ppImageV) == NULL) {
			message("Fail to allocate images in CReadWriteVideoFFMPEG::createImages()!\n");
			if ((*ppImageY) != 0) CImageUtility::safeReleaseImage(ppImageY);
			if ((*ppImageU) != 0) CImageUtility::safeReleaseImage(ppImageU);
			if ((*ppImageV) != 0) CImageUtility::safeReleaseImage(ppImageV);
			return false;
		}
	}

	return true;
}

bool CReadWriteVideoFFMPEG::getFrame(FFMpegVideoHandle hVideo, IplImage *iplImageY, IplImage *iplImageU, IplImage *iplImageV)
// Open a video file for read 
// Argument:
//		hVideo -- [I] formation of an opened video stream
//		iplImageY, iplImageU, iplImageV -- [O] Y, U, V plane of the frame
// Develop based on reference code for ffmpeg, by Luhong Liang, IC-ASD, ASTRI, Nov. 28, 2011
// May 19, 2014: support packet YUV422 format (YUYV and UYVY)
{
	// check status
	if (!m_bInitialized) {
		message("Codec not initialized found in CReadWriteVideoFFMPEG::getFrame()!\n");
		return false;
	}

	// check arguments
	PixelFormat pix_fmt = getFormat(hVideo);
	bool bIsPlanar = isPlanar(pix_fmt);
	if (iplImageY == NULL || bIsPlanar && (iplImageU == NULL || iplImageV == NULL) ||
		hVideo.pCodecCtx == NULL || hVideo.pFrame == NULL) {
		message("Invalid input video handle or image buffer in CReadWriteVideoFFMPEG::getFrame()!\n");
		return false;
	}
	int nLumaW = hVideo.pCodecCtx->width;
	int nLumaH = hVideo.pCodecCtx->height;
	int nChromaW, nChromaH, nLineSizeLuma;
	switch (pix_fmt) {
		case PIX_FMT_YUV420P:
        case PIX_FMT_YUVJ420P:      //< planar YUV 4:2:0, 12bpp, full scale (JPEG)
			nLineSizeLuma = nLumaW;
			nChromaW = nLumaW / 2;
			nChromaH = nLumaH / 2;
			break;
		case PIX_FMT_YUV422P:	    // all YUV files include the one in packed format are stored internally in planar
        case PIX_FMT_YUVJ422P:      // planar YUV 4:2:2, 16bpp, full scale (JPEG)
		case PIX_FMT_YUYV422:       //< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
        case PIX_FMT_UYVY422:       //< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
			nLineSizeLuma = nLumaW;
			nChromaW = nLumaW / 2;
			nChromaH = nLumaH;
			break;
		case PIX_FMT_YUV444P:
        case PIX_FMT_YUVJ444P:      //< planar YUV 4:4:4, 24bpp, full scale (JPEG)
			nLineSizeLuma = nLumaW;
			nChromaW = nLumaW;
			nChromaH = nLumaH;
			break;
		case PIX_FMT_GRAY8:
			nLineSizeLuma = nLumaW;
			nChromaW = 0;
			nChromaH = 0;
			break;
		// planar formats
		case PIX_FMT_RGB24:
		case PIX_FMT_BGR24:
			nLineSizeLuma = nLumaW * 3;
			nChromaW = 0;
			nChromaH = 0;
			break;
		case PIX_FMT_ARGB:      ///< packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
		case PIX_FMT_RGBA:      ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
		case PIX_FMT_ABGR:      ///< packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
		case PIX_FMT_BGRA:      ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
			nLineSizeLuma = nLumaW * 4;
			nChromaW = 0;
			nChromaH = 0;
			break;
		default:
			message("Unsupported pixel format found in CReadWriteVideoFFMPEG::getFrame()!\n");
			return false;
	}
	if (iplImageY->width != nLumaW || iplImageY->height != nLumaH || (nChromaW != 0 && (
		iplImageU->width != nChromaW || iplImageU->height != nChromaH ||
		iplImageV->width != nChromaW || iplImageV->height != nChromaH || 
		iplImageY->nChannels != 1 || iplImageU->nChannels != 1 || iplImageV->nChannels != 1)) ) {
		message("Mismatched video format and frame buffer found in CReadWriteVideoFFMPEG::getFrame()!\n");
		return false;
	}

	// get frame
	int frameFinished;
	AVPacket packet;
	while(av_read_frame(hVideo.pFormatCtx, &packet)>=0) {
		// Is this a packet from the video stream?
		if(packet.stream_index == hVideo.videoStream) {
			// Decode video frame
			// Technically a packet can contain partial frames or other bits of data, but ffmpeg's parser
			// ensures that the packets we get contain either complete or multiple frames. 
			// NOTE: Pls. check if avcodec_decode_video() is defined in avcodec-52.def!
			avcodec_decode_video(hVideo.pCodecCtx, hVideo.pFrame, &frameFinished, packet.data, packet.size);
    
			// Did we get a video frame?
			if(frameFinished) {
				// de-interlace
				if (hVideo.pFrame->interlaced_frame) {
					AVPicture deinterlacedPicture;
					AVPicture source;
					source.data[0] =  hVideo.pFrame->data[0];
					source.data[1] =  hVideo.pFrame->data[1];
					source.data[2] =  hVideo.pFrame->data[2];
					source.data[3] =  hVideo.pFrame->data[3];
					source.linesize[0] =  hVideo.pFrame->linesize[0];
					source.linesize[1] =  hVideo.pFrame->linesize[1];
					source.linesize[2] =  hVideo.pFrame->linesize[2];
					source.linesize[3] =  hVideo.pFrame->linesize[3];
					avpicture_alloc(&deinterlacedPicture, hVideo.pCodecCtx->pix_fmt, iplImageY->width, iplImageY->height);
					if (avpicture_deinterlace(&deinterlacedPicture, &source, hVideo.pCodecCtx->pix_fmt, iplImageY->width, iplImageY->height) < 0) {
						printf("Fail to de-interlace the video in CReadWriteVideoFFMPEG::getFrame()!\n");
						break;
					}

                    if (bIsPlanar) {
					    // copy video data for Y
					    for (int y=0; y<nLumaH; y++) {
						    char *pSrc = (char *)deinterlacedPicture.data[0] + y * deinterlacedPicture.linesize[0];
						    char *pDst = iplImageY->imageData + y * iplImageY->widthStep;
						    memcpy(pDst, pSrc, nLineSizeLuma);	// consider both 1- and 3-channel images
					    }
					    // copy video data for U, V
					    if (nChromaW != 0) {
						    for (int y=0; y<nChromaH; y++) {
							    char *pSrcU = (char *)deinterlacedPicture.data[1] + y * deinterlacedPicture.linesize[1];
							    char *pDstU = iplImageU->imageData + y * iplImageU->widthStep;
							    memcpy(pDstU, pSrcU, nChromaW);
							    char *pSrcV = (char *)deinterlacedPicture.data[2] + y * deinterlacedPicture.linesize[2];
							    char *pDstV = iplImageV->imageData + y * iplImageV->widthStep;
							    memcpy(pDstV, pSrcV, nChromaW);
						    }
					    }
                    } else {
                        // packet format
                        if (pix_fmt == PIX_FMT_YUYV422) {
					        // copy video data 
					        for (int y=0; y<nLumaH; y++) {
						        char *pSrc = (char *)hVideo.pFrame->data[0] + y * hVideo.pFrame->linesize[0];
						        char *pY = iplImageY->imageData + y * iplImageY->widthStep;
                                char *pU = iplImageU->imageData + y * iplImageU->widthStep;
                                char *pV = iplImageV->imageData + y * iplImageV->widthStep;
                                for (int x=0, cx=0; x<nLumaW; x+=2, cx++) {
                                    int xx = x * 2;
                                    pY[x] = pSrc[xx];
                                    pU[cx] = pSrc[xx+1];
                                    pY[x+1] = pSrc[xx+2];
                                    pV[cx] = pSrc[xx+3];
                                }
					        }
                        } else  if (pix_fmt == PIX_FMT_UYVY422) {
					        // copy video data 
					        for (int y=0; y<nLumaH; y++) {
						        char *pSrc = (char *)hVideo.pFrame->data[0] + y * hVideo.pFrame->linesize[0];
						        char *pY = iplImageY->imageData + y * iplImageY->widthStep;
                                char *pU = iplImageU->imageData + y * iplImageU->widthStep;
                                char *pV = iplImageV->imageData + y * iplImageV->widthStep;
                                for (int x=0, cx=0; x<nLumaW; x+=2, cx++) {
                                    int xx = x * 2;
                                    pU[cx] = pSrc[xx];
                                    pY[x] = pSrc[xx+1];
                                    pV[cx] = pSrc[xx+2];
                                    pY[x+1] = pSrc[xx+3];
                                }
					        }
                        } else  if (pix_fmt == PIX_FMT_UYVY422) {
					        // copy video data 
					        for (int y=0; y<nLumaH; y++) {
						        char *pSrc = (char *)hVideo.pFrame->data[0] + y * hVideo.pFrame->linesize[0];
						        char *pY = iplImageY->imageData + y * iplImageY->widthStep;
                                char *pU = iplImageU->imageData + y * iplImageU->widthStep;
                                char *pV = iplImageV->imageData + y * iplImageV->widthStep;
                                for (int x=0, cx=0; x<nLumaW; x+=2, cx++) {
                                    int xx = x * 2;
                                    pU[cx] = pSrc[xx];
                                    pY[x] = pSrc[xx+1];
                                    pV[cx] = pSrc[xx+2];
                                    pY[x+1] = pSrc[xx+3];
                                }
					        }
                        } else if (pix_fmt == PIX_FMT_BGR24 || pix_fmt == PIX_FMT_RGB24) {
					        // copy video data 
                            int max_line_size = min(hVideo.pFrame->linesize[0], nLumaW*3);
					        for (int y=0; y<nLumaH; y++) {
						        char *pSrc = (char *)hVideo.pFrame->data[0] + y * hVideo.pFrame->linesize[0];
						        char *pY = iplImageY->imageData + y * iplImageY->widthStep;
                                memcpy(pY, pSrc, max_line_size*sizeof(char));
					        }
                            //CImageUtility::saveImage("_Test.bmp", iplImageY);
                        }   else {
                            printf("Unsupported packet pixel format in CReadWriteVideoFFMPEG::getFrame()!\n");
                            av_free_packet(&packet);
                            return false;
                        }
                    }
					avpicture_free(&deinterlacedPicture);
				} else {        // progressive
                    if (bIsPlanar) {
					    // copy video data for Y
					    for (int y=0; y<nLumaH; y++) {
						    char *pSrc = (char *)hVideo.pFrame->data[0] + y * hVideo.pFrame->linesize[0];
						    char *pDst = iplImageY->imageData + y * iplImageY->widthStep;
						    memcpy(pDst, pSrc, nLineSizeLuma);	// consider both 1- and 3-channel images
					    }
					    // copy video data for U, V
					    if (nChromaW != 0) {
						    for (int y=0; y<nChromaH; y++) {
							    char *pSrcU = (char *)hVideo.pFrame->data[1] + y * hVideo.pFrame->linesize[1];
							    char *pDstU = iplImageU->imageData + y * iplImageU->widthStep;
							    memcpy(pDstU, pSrcU, nChromaW);
							    char *pSrcV = (char *)hVideo.pFrame->data[2] + y * hVideo.pFrame->linesize[2];
							    char *pDstV = iplImageV->imageData + y * iplImageV->widthStep;
							    memcpy(pDstV, pSrcV, nChromaW);
						    }
					    }
				    } else {
                        // packet format
                        if (pix_fmt == PIX_FMT_YUYV422) {
					        // copy video data 
					        for (int y=0; y<nLumaH; y++) {
						        char *pSrc = (char *)hVideo.pFrame->data[0] + y * hVideo.pFrame->linesize[0];
						        char *pY = iplImageY->imageData + y * iplImageY->widthStep;
                                char *pU = iplImageU->imageData + y * iplImageU->widthStep;
                                char *pV = iplImageV->imageData + y * iplImageV->widthStep;
                                for (int x=0, cx=0; x<nLumaW; x+=2, cx++) {
                                    int xx = x * 2;
                                    pY[x] = pSrc[xx];
                                    pU[cx] = pSrc[xx+1];
                                    pY[x+1] = pSrc[xx+2];
                                    pV[cx] = pSrc[xx+3];
                                }
					        }
                        } else if (pix_fmt == PIX_FMT_BGR24 || pix_fmt == PIX_FMT_RGB24) {
					        // copy video data 
                            int max_line_size = min(hVideo.pFrame->linesize[0], nLumaW*3);
					        for (int y=0; y<nLumaH; y++) {
						        char *pSrc = (char *)hVideo.pFrame->data[0] + y * hVideo.pFrame->linesize[0];
						        char *pY = iplImageY->imageData + y * iplImageY->widthStep;
                                memcpy(pY, pSrc, max_line_size*sizeof(char));
					        }
                            //CImageUtility::saveImage("_Test.bmp", iplImageY);
                        }   else {
                            printf("Unsupported packet pixel format in CReadWriteVideoFFMPEG::getFrame()!\n");
                            av_free_packet(&packet);
                            return false;
                        }
                    }
                }

				av_free_packet(&packet);
				return true;
			}
		}
		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}

	return false;
}

bool CReadWriteVideoFFMPEG::closeRead(FFMpegVideoHandle hVideo)
// close a video file for read 
// Develop based on reference code for ffmpeg, by Luhong Liang, IC-ASD, ASTRI, Nov. 28, 2011
{
	// check status
	if (!m_bInitialized) {
		message("Codec not initialized found in CReadWriteVideoFFMPEG::closeRead()!\n");
		return false;
	}

	// free buffers
	if (hVideo.pFrame != NULL) {
		av_free(hVideo.pFrame);
		hVideo.pFrame = NULL;
	}

	// Close the codec
	if (hVideo.pCodecCtx != NULL) {
		avcodec_close(hVideo.pCodecCtx);
		av_free(hVideo.pCodecCtx);
		hVideo.pCodecCtx = NULL;
	}

	// Close the video file
	if (hVideo.pFormatCtx != NULL) {
		//av_close_input_file(hVideo.pFormatCtx);
		av_free(hVideo.pFormatCtx);
		hVideo.pFormatCtx = NULL;
	}

	if (hVideo.pOutBuf != NULL) {
		av_free(hVideo.pOutBuf);
		hVideo.pOutBuf = NULL;
	}

	// close file for write
	if (hVideo.fpWrite != NULL) {
		fclose(hVideo.fpWrite);
		hVideo.fpWrite = NULL;
	}

	// make stream ID invalid
	hVideo.videoStream = -1;

	return true;
}

bool CReadWriteVideoFFMPEG::openWrite(char *stFilename, VideoFormat fmt, FFMpegVideoHandle &hVideo)
// Open a video file for write
// NOTE: currently, this function only support MPEG-1 and MPEG-2 video
// can compress WMV and RM format but can not correctly decoded (possibly the wrapper prolbem?)
// can not compress as H.264 because the ffmpeg used does not support H.264 encoding
// Develop based on reference code for ffmpeg, by Luhong Liang, IC-ASD, ASTRI, Nov. 28, 2011
{
	// check status
	if (!m_bInitialized) {
		message("Codec not initialized found in CReadWriteVideoFFMPEG::openWrite()!\n");
		return false;
	}
	if (stFilename == NULL || stFilename[0] == 0) {
		message("Invalide input file name found in CReadWriteVideoFFMPEG::openWrite()!\n");
		return false;
	}
	hVideo.initHandle();

	if (fmt.codec == CODEC_ID_RAWVIDEO) { // && strcmp(strlwr(CImageUtility::getFileExt(stFilename)), "avi") == 0) {
		// for uncompressed AVI
		//AVFormatContext *pFormatCtxEnc;
		//AVCodecContext *pCodecCtxEnc;
		
		// auto detect the output format from the name
		AVOutputFormat *pOutputFormat = guess_format(NULL, stFilename, NULL);
		if (pOutputFormat == NULL) {
			message("Supported file RAWVIDEO format found in CReadWriteVideoFFMPEG::openWrite()!\n");
			return false;
		}

		// allocate the output media context
		hVideo.pFormatCtx = av_alloc_format_context();
		hVideo.pFormatCtx->oformat = pOutputFormat;
		snprintf(hVideo.pFormatCtx->filename, sizeof(hVideo.pFormatCtx->filename), "%s", stFilename);

		AVStream *video_st = av_new_stream(hVideo.pFormatCtx, 0);
		if (video_st == NULL) {
			message("Fail to create video stream in CReadWriteVideoFFMPEG::openWrite()!\n");
			return false;
		}

		hVideo.pCodecCtx = video_st->codec;
		hVideo.videoStream = video_st->index;
		hVideo.pCodecCtx->codec_id = CODEC_ID_RAWVIDEO;
		hVideo.pCodecCtx->codec_type = CODEC_TYPE_VIDEO;
		hVideo.pCodecCtx->codec_tag = MKTAG('I', '4', '2', '0');

		// put sample parameters 
		// resolution must be a multiple of two 
		hVideo.pCodecCtx->width = fmt.width;
		hVideo.pCodecCtx->height = fmt.height;
		// frames per second 
		hVideo.pCodecCtx->time_base.den = (int)fmt.frame_rate;
		hVideo.pCodecCtx->time_base.num = 1;
		hVideo.pCodecCtx->pix_fmt = PIX_FMT_YUV420P;

		// set the output parameters (must be done even if no parameters).
		if (av_set_parameters(hVideo.pFormatCtx, NULL) < 0) {
			message("Fail to set video encoding parameter in CReadWriteVideoFFMPEG::openWrite()!\n");
			return false;
		}

		//dump_format(hVideo.pFormatCtx, 0, stFilename, 1);

        if (url_fopen(&hVideo.pFormatCtx->pb, stFilename, URL_WRONLY) < 0) {
            message("Fail to create video file in CReadWriteVideoFFMPEG::openWrite()!\n");
            return false;
        }

		// write the stream header, if any
		av_write_header(hVideo.pFormatCtx);

		// allocate buffer
		int nLumaW = hVideo.pCodecCtx->width;
		int nLumaH = hVideo.pCodecCtx->height;
		int nChromaW = nLumaW / 2;
		int nChromaH = nLumaH / 2;
		int sizeL = nLumaW * nLumaH;
		int sizeC = nChromaW * nChromaH;
		hVideo.pPacket = new AVPacket;
		if (hVideo.pPacket != NULL) {
			av_init_packet(hVideo.pPacket);
			hVideo.pPacket->flags |= PKT_FLAG_KEY;
			hVideo.pPacket->stream_index= hVideo.videoStream;		//video_st->index;
			hVideo.pPacket->data = (uint8_t*)av_malloc( sizeL + sizeC + sizeC);
		}
		if (hVideo.pPacket == NULL || hVideo.pPacket->data == NULL) {
			message("Fail to allocate buffer in CReadWriteVideoFFMPEG::openWrite()!\n");
			closeWrite(hVideo);
			return false;
		}
		hVideo.pPacket->size= sizeL + sizeC + sizeC;
	} else {
		// for other codecs
		// find the video encoder
		AVCodec *codec = avcodec_find_encoder((CodecID)fmt.codec);
		if (!codec) {
			char stMessage[256];
			sprintf(stMessage, "Codec not found for file %s in CReadWriteVideoFFMPEG::openWrite()!\n", stFilename);
			message(stMessage);
			return false;
		}

		hVideo.pCodecCtx = avcodec_alloc_context();
		hVideo.pFrame = avcodec_alloc_frame();
		if (hVideo.pCodecCtx == NULL || hVideo.pFrame == NULL) {
			closeWrite(hVideo);
			return false;
		}

		// put sample parameters
		hVideo.pCodecCtx->codec_type = CODEC_TYPE_VIDEO;
		if (fmt.pix_fmt == PIX_FMT_YUV420P)
			hVideo.pCodecCtx->codec_tag = MKTAG('I', '4', '2', '0');
		else if (fmt.pix_fmt == PIX_FMT_YUV422P) 
			hVideo.pCodecCtx->codec_tag = MKTAG('I', '4', '2', '2');
		else {
			message("Unsupported pixel format (only support YUV420P and YUV422P!");
			return false;
		}
		hVideo.pCodecCtx->bit_rate = fmt.bit_rate;
		hVideo.pCodecCtx->rc_min_rate = fmt.bit_rate;
		hVideo.pCodecCtx->rc_max_rate = fmt.bit_rate;
		hVideo.pCodecCtx->bit_rate_tolerance = fmt.bit_rate;
		hVideo.pCodecCtx->rc_buffer_size = fmt.bit_rate * 2;
		hVideo.pCodecCtx->rc_initial_buffer_occupancy = hVideo.pCodecCtx->rc_buffer_size;
		hVideo.pCodecCtx->rc_buffer_aggressivity = 1.0f;
		hVideo.pCodecCtx->rc_initial_cplx= 0.5f;
		// resolution must be a multiple of two
		hVideo.pCodecCtx->width = fmt.width;
		hVideo.pCodecCtx->height = fmt.height;
		// frames per second
		hVideo.pCodecCtx->time_base.num = 1; 
		hVideo.pCodecCtx->time_base.den = (int)fmt.frame_rate;		//= (AVRational){1,25};
		hVideo.pCodecCtx->gop_size = 25;
		hVideo.pCodecCtx->max_b_frames=2;		// IBBP
		//hVideo.pCodecCtx->mb_decision = FF_MB_DECISION_RD;
		hVideo.pCodecCtx->pix_fmt = fmt.pix_fmt;

		//////////////////////
		//hVideo.pCodecCtx->bit_rate_tolerance = fmt.bit_rate; 
		//hVideo.pCodecCtx->rc_max_rate = 0; 
		//hVideo.pCodecCtx->rc_buffer_size = 0; 
		/*
		hVideo.pCodecCtx->gop_size = 40; 
		hVideo.pCodecCtx->max_b_frames = 3; 
		hVideo.pCodecCtx->b_frame_strategy = 1; 
		hVideo.pCodecCtx->coder_type = 1; 
		hVideo.pCodecCtx->me_cmp = 1; 
		hVideo.pCodecCtx->me_range = 16; 
		hVideo.pCodecCtx->qmin = 10; 
		hVideo.pCodecCtx->qmax = 51; 
		hVideo.pCodecCtx->scenechange_threshold = 40; 
		hVideo.pCodecCtx->flags |= CODEC_FLAG_LOOP_FILTER; 
		hVideo.pCodecCtx->me_method = ME_HEX; 
		hVideo.pCodecCtx->me_subpel_quality = 5; 
		hVideo.pCodecCtx->i_quant_factor = 0.71; 
		hVideo.pCodecCtx->qcompress = 0.6; 
		hVideo.pCodecCtx->max_qdiff = 4; 
		hVideo.pCodecCtx->directpred = 1; 
		hVideo.pCodecCtx->flags2 |= CODEC_FLAG2_FASTPSKIP;
		*/


		///////////////////////

		// open it 
		if (avcodec_open(hVideo.pCodecCtx, codec) < 0) {
			char stMessage[256];
			sprintf(stMessage, "Could not open codec for %s in CReadWriteVideoFFMPEG::openWrite()!\n", stFilename);
			message(stMessage);
			closeWrite(hVideo);
			return false;
		}

		hVideo.fpWrite = fopen(stFilename, "wb");
		if (!hVideo.fpWrite) {
			char stMessage[256];
			sprintf(stMessage, "Could not create file %s in CReadWriteVideoFFMPEG::openWrite()!\n", stFilename);
			message(stMessage);
			closeWrite(hVideo);
			return false;
		}

		// alloc image and output buffer
		hVideo.pOutBuf = (uint8_t*)av_malloc(_FFMPEG_ENCODER_OUT_BUFFER_SIZE);			//malloc(outbuf_size);
		int nLumaW = hVideo.pCodecCtx->width;
		int nLumaH = hVideo.pCodecCtx->height;
		int nChromaW, nChromaH, nLineSizeLuma;
		switch (getFormat(hVideo)) {
			case PIX_FMT_YUV420P:
				nLineSizeLuma = nLumaW;
				nChromaW = nLumaW / 2;
				nChromaH = nLumaH / 2;
				break;
			case PIX_FMT_YUV422P:	// all YUV files include the one in packed format are stored internally in planar
			case PIX_FMT_YUYV422:
				nLineSizeLuma = nLumaW;
				nChromaW = nLumaW / 2;
				nChromaH = nLumaH;
				break;
			case PIX_FMT_YUV444P:
				nLineSizeLuma = nLumaW;
				nChromaW = nLumaW;
				nChromaH = nLumaH;
				break;
			case PIX_FMT_GRAY8:
				nLineSizeLuma = nLumaW;
				nChromaW = 0;
				nChromaH = 0;
				break;
			// planar formats
			case PIX_FMT_RGB24:
			case PIX_FMT_BGR24:
				nLineSizeLuma = nLumaW * 3;
				nChromaW = 0;
				nChromaH = 0;
				break;
			case PIX_FMT_ARGB:      ///< packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
			case PIX_FMT_RGBA:      ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
			case PIX_FMT_ABGR:      ///< packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
			case PIX_FMT_BGRA:      ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
				nLineSizeLuma = nLumaW * 4;
				nChromaW = 0;
				nChromaH = 0;
				break;
			default:
				message("Unsupported pixel format found in CReadWriteVideoFFMPEG::openWrite()!\n");
				closeWrite(hVideo);
				return false;
		}
		int sizeL = nLineSizeLuma * nLumaH;
		int sizeC = nChromaW * nChromaH;
		hVideo.pFrame->data[0] = (uint8_t*)av_malloc( sizeL + sizeC + sizeC);
		if (hVideo.pOutBuf == NULL || hVideo.pFrame->data[0] == NULL) {
			message("Fail to allocate buffer in CReadWriteVideoFFMPEG::openWrite()!\n");
			closeWrite(hVideo);
			return false;
		}
		hVideo.pFrame->data[1] = hVideo.pFrame->data[0] + sizeL;
		hVideo.pFrame->data[2] = hVideo.pFrame->data[1] + sizeC;
		hVideo.pFrame->linesize[0] = nLineSizeLuma;
		hVideo.pFrame->linesize[1] = nChromaW;
		hVideo.pFrame->linesize[2] = nChromaW;
	}

	return true;
}

bool CReadWriteVideoFFMPEG::writeFrame(FFMpegVideoHandle hVideo, IplImage *iplImageY, IplImage *iplImageU, IplImage *iplImageV)
// Write a video frame
// Develop based on reference code for ffmpeg, by Luhong Liang, IC-ASD, ASTRI, Nov. 28, 2011
{
	// check status
	if (!m_bInitialized) {
		message("Codec not initialized found in CReadWriteVideoFFMPEG::writeFrame()!\n");
		return false;
	}

	// check arguments
	PixelFormat pix_fmt = getFormat(hVideo);
	bool bIsPlanar = isPlanar(pix_fmt);
	if (iplImageY == NULL || bIsPlanar && (iplImageU == NULL || iplImageV == NULL) ||
		hVideo.pCodecCtx == NULL || (hVideo.pFrame == NULL && hVideo.pPacket == NULL)) {
		message("Invalid input video handle or image buffer in CReadWriteVideoFFMPEG::writeFrame()!\n");
		return false;
	}
	int nLumaW = hVideo.pCodecCtx->width;
	int nLumaH = hVideo.pCodecCtx->height;
	int nChromaW, nChromaH, nLineSizeLuma;
	switch (pix_fmt) {
		case PIX_FMT_YUV420P:
			nLineSizeLuma = nLumaW;
			nChromaW = nLumaW / 2;
			nChromaH = nLumaH / 2;
			break;
		case PIX_FMT_YUV422P:
		case PIX_FMT_YUYV422:		// all YUV files include the one in packed format are stored internally in planar
			nLineSizeLuma = nLumaW;
			nChromaW = nLumaW / 2;
			nChromaH = nLumaH;
			break;
		case PIX_FMT_YUV444P:
			nLineSizeLuma = nLumaW;
			nChromaW = nLumaW;
			nChromaH = nLumaH;
			break;
		case PIX_FMT_GRAY8:
			nLineSizeLuma = nLumaW;
			nChromaW = 0;
			nChromaH = 0;
			break;
		// planar formats
		case PIX_FMT_RGB24:
		case PIX_FMT_BGR24:
			nLineSizeLuma = nLumaW * 3;
			nChromaW = 0;
			nChromaH = 0;
			break;
		case PIX_FMT_ARGB:      ///< packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
		case PIX_FMT_RGBA:      ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
		case PIX_FMT_ABGR:      ///< packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
		case PIX_FMT_BGRA:      ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
			nLineSizeLuma = nLumaW * 3;
			nChromaW = 0;
			nChromaH = 0;
			break;
		default:
			message("Unsupported pixel format found in CReadWriteVideoFFMPEG::writeFrame()!\n");
			return false;
	}
	if (iplImageY->width != nLumaW || iplImageY->height != nLumaH || (nChromaW != 0 && (
		iplImageU->width != nChromaW || iplImageU->height != nChromaH ||
		iplImageV->width != nChromaW || iplImageV->height != nChromaH || 
		iplImageY->nChannels != 1 || iplImageU->nChannels != 1 || iplImageV->nChannels != 1)) ) {
		message("Mismatched video format and frame buffer found in CReadWriteVideoFFMPEG::writeFrame()!\n");
		return false;
	}

	// encode the image
	char stMessage[256];
	if (hVideo.pCodecCtx->codec_id == CODEC_ID_RAWVIDEO) {
		// for uncompressed AVI

		// copy video data for Y
		for (int y=0; y<nLumaH; y++) {
			char *pDst = (char *)hVideo.pPacket->data + y * nLumaW;
			char *pSrc = iplImageY->imageData + y * iplImageY->widthStep;
			memcpy(pDst, pSrc, nLineSizeLuma);
		}
		
		// copy video data for U, V
		if (nChromaW != 0) {
			for (int y=0; y<nChromaH; y++) {
				char *pDstU = (char *)hVideo.pPacket->data + nLumaW * nLumaH + y * nChromaW;
				char *pSrcU = iplImageU->imageData + y * iplImageU->widthStep;
				memcpy(pDstU, pSrcU, nChromaW);
			}
			for (int y=0; y<nChromaH; y++) {
				char *pDstV = (char *)hVideo.pPacket->data + nLumaW * nLumaH + nChromaW * nChromaH + y * nChromaW;
				char *pSrcV = iplImageV->imageData + y * iplImageV->widthStep;
				memcpy(pDstV, pSrcV, nChromaW);
			}
		}

        // write the compressed frame in the media file
		av_write_frame(hVideo.pFormatCtx, hVideo.pPacket);
		sprintf(stMessage, "write one frame of %d bytes\n", hVideo.pPacket->size);
		message(stMessage);

	} else {
		// for other codecs

		// copy video data for Y
		for (int y=0; y<nLumaH; y++) {
			char *pDst = (char *)hVideo.pFrame->data[0] + y * hVideo.pFrame->linesize[0];
			char *pSrc = iplImageY->imageData + y * iplImageY->widthStep;
			memcpy(pDst, pSrc, nLineSizeLuma);
		}
		
		// copy video data for U, V
		if (nChromaW != 0) {
			for (int y=0; y<nChromaH; y++) {
				char *pDstU = (char *)hVideo.pFrame->data[1] + y * hVideo.pFrame->linesize[1];
				char *pSrcU = iplImageU->imageData + y * iplImageU->widthStep;
				memcpy(pDstU, pSrcU, nChromaW);
				char *pDstV = (char *)hVideo.pFrame->data[2] + y * hVideo.pFrame->linesize[2];
				char *pSrcV = iplImageV->imageData + y * iplImageV->widthStep;
				memcpy(pDstV, pSrcV, nChromaW);
			}
		}

		fflush(stdout);
		hVideo.out_size = avcodec_encode_video(hVideo.pCodecCtx, hVideo.pOutBuf, _FFMPEG_ENCODER_OUT_BUFFER_SIZE, hVideo.pFrame);

		// write stream
		fwrite(hVideo.pOutBuf, 1, hVideo.out_size, hVideo.fpWrite);
		sprintf(stMessage, "write one frame of %d bytes\n", hVideo.out_size);
		message(stMessage);
	}

	return true;
}

bool CReadWriteVideoFFMPEG::closeWrite(FFMpegVideoHandle hVideo)
// close a video file for write 
// Develop based on reference code for ffmpeg, by Luhong Liang, IC-ASD, ASTRI, Nov. 28, 2011
{
	// check status
	if (!m_bInitialized) {
		message("Codec not initialized found in CReadWriteVideoFFMPEG::closeWrite()!\n");
		return false;
	}

	if (hVideo.pCodecCtx->codec_id == CODEC_ID_RAWVIDEO) {
		// for uncompressed AVI

		// write the trailer, if any
		av_write_trailer(hVideo.pFormatCtx);

		// free the streams
		for(unsigned i = 0; i < hVideo.pFormatCtx->nb_streams; i++) {
			av_freep(&hVideo.pFormatCtx->streams[i]->codec);
			av_freep(&hVideo.pFormatCtx->streams[i]);
		}

		//if (!(pOutputFormat->flags & AVFMT_NOFILE)) 
		//close the output file
		url_fclose(hVideo.pFormatCtx->pb);

		// free the stream
		av_free(hVideo.pFormatCtx);
	} else {
		// for other codecs
		if (hVideo.fpWrite == NULL) {
			message("Invalid file handle found in CReadWriteVideoFFMPEG::closeWrite()!\n");
			return false;
		}

		// get the delayed frames
		while (hVideo.out_size > 0) {
			fflush(stdout);

			hVideo.out_size = avcodec_encode_video(hVideo.pCodecCtx, hVideo.pOutBuf, _FFMPEG_ENCODER_OUT_BUFFER_SIZE, NULL);
			if (hVideo.out_size > 0) {
				char stMessage[256];
				sprintf(stMessage, "write one frame of size=%5d)\n", hVideo.out_size);
				message(stMessage);
				fwrite(hVideo.pOutBuf, 1, hVideo.out_size, hVideo.fpWrite);
			}
		}

		// add sequence end code to
		hVideo.pOutBuf[0] = 0x00;
		hVideo.pOutBuf[1] = 0x00;
		hVideo.pOutBuf[2] = 0x01;
		hVideo.pOutBuf[3] = 0xb7;
		fwrite(hVideo.pOutBuf, 1, 4, hVideo.fpWrite);

		// Close the codec
		if (hVideo.pCodecCtx != NULL) {
			avcodec_close(hVideo.pCodecCtx);
			av_free(hVideo.pCodecCtx);
			hVideo.pCodecCtx = NULL;
		}

		// Close the video file
		if (hVideo.pFormatCtx != NULL) {
			av_close_input_file(hVideo.pFormatCtx);
			av_free(hVideo.pFormatCtx);
			hVideo.pFormatCtx = NULL;
		}
	}

	// free buffers
	if (hVideo.pOutBuf != NULL) {
		av_free(hVideo.pOutBuf);
		hVideo.pOutBuf = NULL;
	}

	// close file for write
	if (hVideo.fpWrite != NULL) {
		fclose(hVideo.fpWrite);
		hVideo.fpWrite = NULL;
	}

	// free buffers
	if (hVideo.pFrame != NULL) {
		av_free(hVideo.pFrame->data[0]);
		av_free(hVideo.pFrame);
		hVideo.pFrame = NULL;
	}

	if (hVideo.pPacket != NULL) {
		//if (hVideo.pPacket->data != NULL) delete hVideo.pPacket->data;	// TODO: still have some problem in packet management
		delete hVideo.pPacket;
	}

	// make stream ID invalid
	hVideo.videoStream = -1;

	return true;
}

bool CReadWriteVideoFFMPEG::scaling(IplImage *iplSrcY, IplImage *iplSrcU, IplImage *iplSrcV, PixelFormat src_pix_fmt,
									IplImage *iplDstY, IplImage *iplDstU, IplImage *iplDstV, PixelFormat dst_pix_fmt, int flags)
// Scale image frame using FFMPEG swscale.lib
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
// Develop based on reference code for ffmpeg, by Luhong Liang, IC-ASD, ASTRI, Nov. 28, 2011
{
	if (iplSrcY == NULL || iplDstY == NULL) {
		message("Invalid input/output image in CReadWriteVideoFFMPEG::scaling()!\n");
		return false;
	}

	// check format
	bool bError = false;
	switch (src_pix_fmt) {
		case PIX_FMT_YUV420P:
        case PIX_FMT_YUVJ420P:      //< planar YUV 4:2:0, 12bpp, full scale (JPEG)
		case PIX_FMT_YUV422P:	    // all YUV files include the one in packed format are stored internally in planar
        case PIX_FMT_YUVJ422P:      // planar YUV 4:2:2, 16bpp, full scale (JPEG)
		case PIX_FMT_YUYV422:
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
		case PIX_FMT_YUV422P:	// all YUV files include the one in packed format are stored internally in planar
		case PIX_FMT_YUYV422:
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
		message("Invalid input/output image in CReadWriteVideoFFMPEG::scaling()!\n");
		return false;
	}

    // Initialize software scaling
	struct SwsContext *img_convert_ctx = sws_getContext(iplSrcY->width, iplSrcY->height, src_pix_fmt, 
														iplDstY->width, iplDstY->height, dst_pix_fmt, flags, NULL, NULL, NULL);
    if(img_convert_ctx == NULL) {
        message("Cannot initialize the conversion context in CReadWriteVideoFFMPEG::scaling()!\n");
        return false;
    }

	// assign picture buffer
	AVPicture in_pic, out_pic;
	char *pSrcBuf=NULL, *pDstBuf=NULL;		// for PIX_FMT_YUYV422

	// input 
	if (src_pix_fmt == PIX_FMT_YUYV422) {
		// needs format conversion for packed YUV 422
		pSrcBuf = new char [iplSrcY->width * iplSrcY->height * 2];
		if (pSrcBuf == NULL) {
			message("Fail to allocate buffer in CReadWriteVideoFFMPEG::scaling()!\n");
			return false;
		}
		// format conversion
		for (int y=0; y<iplSrcY->height; y++) {		// should copy line by line since IplImage is word-aligned!
			char *pSrc = iplSrcY->imageData + y * iplSrcY->widthStep;
			char *pSrcU = iplSrcU->imageData + y * iplSrcU->widthStep;
			char *pSrcV = iplSrcV->imageData + y * iplSrcV->widthStep;
			char *pDst = pSrcBuf + y * iplSrcY->width * 2;
			for (int x=0, xx=0, xxx=0; x<iplSrcY->width; x+=2, xx+=4, xxx++) {
				// copy one YUYV unit
				pDst[xx] = pSrc[x];			// Y
				pDst[xx+1] = pSrcU[xxx];	// U
				pDst[xx+2] = pSrc[x+1];		// Y
				pDst[xx+3] = pSrcV[xxx];	// V
			}
		}
		// Packed
		in_pic.data[0] = (uint8_t *)pSrcBuf;
		in_pic.linesize[0] = iplSrcY->width * 2;
	} else if (src_pix_fmt == PIX_FMT_UYVY422) {
		// needs format conversion for packed YUV 422
		pSrcBuf = new char [iplSrcY->width * iplSrcY->height * 2];
		if (pSrcBuf == NULL) {
			message("Fail to allocate buffer in CReadWriteVideoFFMPEG::scaling()!\n");
			return false;
		}
		// format conversion
		for (int y=0; y<iplSrcY->height; y++) {		// should copy line by line since IplImage is word-aligned!
			char *pSrc = iplSrcY->imageData + y * iplSrcY->widthStep;
			char *pSrcU = iplSrcU->imageData + y * iplSrcU->widthStep;
			char *pSrcV = iplSrcV->imageData + y * iplSrcV->widthStep;
			char *pDst = pSrcBuf + y * iplSrcY->width * 2;
			for (int x=0, xx=0, xxx=0; x<iplSrcY->width; x+=2, xx+=4, xxx++) {
				// copy one YUYV unit
				pDst[xx] = pSrcU[xxx];	// U
				pDst[xx+1] = pSrc[x];			// Y
				pDst[xx+2] = pSrcV[xxx];	// V
				pDst[xx+3] = pSrc[x+1];	// Y
			}
		}
		// Packed
		in_pic.data[0] = (uint8_t *)pSrcBuf;
		in_pic.linesize[0] = iplSrcY->width * 2;
	} else {
		// Y
		in_pic.data[0] = (uint8_t *)iplSrcY->imageData;
		in_pic.linesize[0] = iplSrcY->widthStep;
		// U & V
		if (isPlanar(src_pix_fmt)) {
			in_pic.data[1] = (uint8_t *)iplSrcU->imageData;
			in_pic.linesize[1] = iplSrcU->widthStep;

			in_pic.data[2] = (uint8_t *)iplSrcV->imageData;
			in_pic.linesize[2] = iplSrcV->widthStep;
		}
	}

	// set output buffer
	if (dst_pix_fmt == PIX_FMT_YUYV422 || dst_pix_fmt == PIX_FMT_UYVY422) {
		// needs format conversion for packed YUV 422
		pDstBuf = new char [iplDstY->width * iplDstY->height * 2];
		if (pDstBuf == NULL) {
			message("Fail to allocate buffer in CReadWriteVideoFFMPEG::scaling()!\n");
			return false;
		}
		// Packed
		out_pic.data[0] = (uint8_t *)pDstBuf;
		out_pic.linesize[0] = iplDstY->width * 2;
	} else {
		// Y
		out_pic.data[0] = (uint8_t *)iplDstY->imageData;
		out_pic.linesize[0] = iplDstY->widthStep;
		// U & V
		if (isPlanar(dst_pix_fmt)) {
			out_pic.data[1] = (uint8_t *)iplDstU->imageData;
			out_pic.linesize[1] = iplDstU->widthStep;

			out_pic.data[2] = (uint8_t *)iplDstV->imageData;
			out_pic.linesize[2] = iplDstV->widthStep;
		}
	}

	// scaling
	sws_scale(img_convert_ctx, in_pic.data, in_pic.linesize, 0, iplSrcY->height, out_pic.data, out_pic.linesize);

	// convert data format in need
	if (src_pix_fmt == PIX_FMT_YUYV422 || src_pix_fmt == PIX_FMT_UYVY422) {
		// needs format conversion for packed YUV 422
		delete pSrcBuf;
	}
	// output 
	if (dst_pix_fmt == PIX_FMT_YUYV422) {
		// format conversion
		for (int y=0; y<iplDstY->height; y++) {		// should copy line by line since IplImage is word-aligned!
			char *pDst = iplDstY->imageData + y * iplDstY->widthStep;
			char *pDstU = iplDstU->imageData + y * iplDstU->widthStep;
			char *pDstV = iplDstV->imageData + y * iplDstV->widthStep;
			char *pSrc = pDstBuf + y * iplDstY->width * 2;
			for (int x=0, xx=0, xxx=0; x<iplDstY->width; x+=2, xx+=4, xxx++) {
				// copy one YUYV unit
				pDst[x] = pSrc[xx];		// Y
				pDstU[xxx] = pSrc[xx+1];	// U
				pDst[x+1] = pSrc[xx+2];	// Y
				pDstV[xxx] = pSrc[xx+3];	// V
			}
		} 
		delete pDstBuf;
	} else if (dst_pix_fmt == PIX_FMT_UYVY422) {
		// format conversion
		for (int y=0; y<iplDstY->height; y++) {		// should copy line by line since IplImage is word-aligned!
			char *pDst = iplDstY->imageData + y * iplDstY->widthStep;
			char *pDstU = iplDstU->imageData + y * iplDstU->widthStep;
			char *pDstV = iplDstV->imageData + y * iplDstV->widthStep;
			char *pSrc = pDstBuf + y * iplDstY->width * 2;
			for (int x=0, xx=0, xxx=0; x<iplDstY->width; x+=2, xx+=4, xxx++) {
				// copy one YUYV unit
				pDstU[xxx] = pSrc[xx];	// U
				pDst[x] = pSrc[xx+1];		// Y
				pDstV[xxx] = pSrc[xx+2];	// V
				pDst[x+1] = pSrc[xx+3];	// Y
				
			}
		}
		delete pDstBuf;
	}

	// free context
    sws_freeContext(img_convert_ctx);

	return true;
}

bool CReadWriteVideoFFMPEG::isPlanar(PixelFormat pix_fmt)
{
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
}

bool CReadWriteVideoFFMPEG::isPackedYUV(PixelFormat pix_fmt)
{
	switch (pix_fmt) {
		case PIX_FMT_YUYV422:   ///< packed YUV 4:2:2: 16bpp, Y0 Cb Y1 Cr
		case PIX_FMT_UYVY422:   ///< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
		case PIX_FMT_UYYVYY411: ///< packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
			return true;
		default:
			return false;
	}

	return false;
}

#endif // #ifdef _EXPSR1_USE_FFMPEG
