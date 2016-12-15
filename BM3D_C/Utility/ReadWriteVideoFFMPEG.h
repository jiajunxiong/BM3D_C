// YUV IO functions
// Luhong Liang, IC-ASD, ASTRI
// Nov. 2, 2011

#pragma once
#pragma warning(disable : 4996)

#ifdef _EXPSR1_USE_FFMPEG

#include "ImgProcUtility.h"

extern "C"		// important to compatible with C code!
{
#include "avformat.h"
#include "avcodec.h"
#include "swscale.h"
}

typedef struct {	// wrapper for a handle of video stream
	AVFormatContext *pFormatCtx;		// context for the video stream
	AVCodecContext *pCodecCtx;			// context for video codec, width, height, bit-rate, format information here
	int videoStream;					// index of video stream
	AVFrame *pFrame;					// buffer for frame loading
	AVPacket *pPacket;					// buffer for uncompressed AVI
	uint8_t *pOutBuf;					// output buffer for writing
	int out_size;						// data counter for write 
	FILE *fpWrite;						// file handle for writing video
	void initHandle() 
	{
		pFormatCtx = NULL;
		pCodecCtx = NULL;
		videoStream = -1;
		pFrame = NULL;
		pPacket = NULL;
		pOutBuf = NULL;
		out_size = 0;
		fpWrite = NULL;
	}
} FFMpegVideoHandle;

typedef struct {	// wrapper for video format
	int codec;			// enum CodecID, see ffmpeg data type, currently support CODEC_ID_MPEG1VIDEO, CODEC_ID_MPEG2VIDEO; use int type for expension
	int width;
	int height;
	enum PixelFormat pix_fmt;	// see ffmpeg data type
	enum PixelFormat pix_fmt_rlt;	// see ffmpeg data type
	float frame_rate;
	int bit_rate;
	int start_frame_num;		// start frame number
    int jump_frame_num;         // number of frames that jumps over
} VideoFormat;

class CReadWriteVideoFFMPEG //: public CImageUtility
{
private:
	// attributes
	bool m_bInitialized;

	// functions
	void message(char stMsg[]);

public:
	CReadWriteVideoFFMPEG(void);
	~CReadWriteVideoFFMPEG(void);


	bool init(void);

	// load video
	bool openRead(char *stFilename, FFMpegVideoHandle &hVideo);
	
	PixelFormat getFormat(FFMpegVideoHandle hVideo);
	int getWidth(FFMpegVideoHandle hVideo);
	int getHeight(FFMpegVideoHandle hVideo);
	int getBitRate(FFMpegVideoHandle hVideo);
	float getFrameRate(FFMpegVideoHandle hVideo);

	bool createImages(FFMpegVideoHandle hVideo, IplImage **ppImageY, IplImage **ppImageU, IplImage **ppImageV);
	bool createImages(PixelFormat fmt, int width, int height, IplImage **ppImageY, IplImage **ppImageU, IplImage **ppImageV);

	bool getFrame(FFMpegVideoHandle hVideo, IplImage *iplImageY, IplImage *iplImageU, IplImage *iplImageV);
	bool closeRead(FFMpegVideoHandle hVideo);

	// write video
	bool openWrite(char *stFilename, VideoFormat fmt, FFMpegVideoHandle &hVideo);
	bool writeFrame(FFMpegVideoHandle hVideo, IplImage *iplImageY, IplImage *iplImageU, IplImage *iplImageV);
	bool closeWrite(FFMpegVideoHandle hVideo);

	// scaling
	bool scaling(IplImage *iplSrcY, IplImage *iplSrcU, IplImage *iplSrcV, PixelFormat src_pix_fmt,
				 IplImage *iplDstY, IplImage *iplDstU, IplImage *iplDstV, PixelFormat dst_pix_fmt, int flags);

	// others
	bool isPlanar(PixelFormat pix_fmt);
	bool isPackedYUV(PixelFormat pix_fmt);

	//void initHandle(FFMpegVideoHandle &hVideo);

};

#endif // #ifdef _EXPSR1_USE_FFMPEG


