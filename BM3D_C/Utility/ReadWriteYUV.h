// YUV IO functions
// Luhong Liang, IC-ASD, ASTRI
// Nov. 2, 2011

#pragma once
#pragma warning(disable : 4996)

#include "HastTypeDef.h"

#define MAX_YUV_FILE_NUM	9999

typedef enum {
	KEYWORD_YUV_Unknown,
	KEYWORD_YUV_Width,
	KEYWORD_YUV_Height, 
    KEYWORD_YUV_Bitdepth,
	KEYWORD_YUV_FrameNum,
	KEYWORD_YUV_StartFrame,
	KEYWORD_YUV_EndFrame,
    KEYWORD_YUV_StartIDShown,
	KEYWORD_YUV_Format,
	KEYWORD_YUV_FormatRecom,		// recommended format for result YUV file
	KEYWORD_YUV_FrameRate,
	KEYWORD_YUV_RawDataFile
} YUVIndexItemID;

typedef struct {
	char keyword[256];
	YUVIndexItemID id;
} YUVIndexItemList;

typedef enum {
	IDX_YUV_Unknown = -1,
	IDX_YUV420P,		// 4:2:0 format in planes
	IDX_YUV422P,
	IDX_YUYV422,
	IDX_UYVY422,
	IDX_YUV444P,
	IDX_UYVY422C,
    IDX_YUVMS444,       // converted from RGB 4:4:4 using Microsoft YUV color space (CHROMA_YUVMS)
	IDX_YUVGrey
} YUVFormat;

typedef struct {
	char stFile[256];		// full path of an YUV file
	int nWidth;				// frame width
	int nHeight;			// frame height
	int nWidthChroma;
	int nHeightChroma;
	int nFrameNum;			// frame number in this YUV file
	YUVFormat nFormat;	// a string of YUV format, could be "420", TODO: more format support
	int nGlobalindex;		// global index of the first frame in this file (frame 0 in this file)
} YUVFileFmt;

class CReadWriteYUV //: public CImageUtility
{
private:
	// attributes
	bool m_bOpen;
	char m_cStatus;				// file status, could be 'r', 'w' and 'a'
	int m_nWidth;				// frame width
	int m_nHeight;				// frame height
	int m_nWidthChroma;
	int m_nHeightChroma;
    int m_nBitDepth;
	YUVFormat m_nFormat;		// raw data format, could be '420', 'YUYV422', 'UYVY422'
	YUVFormat m_nFormatResult;	// raw data format of result video (if in YUV format), could be '420', 'YUYV422', 'UYVY422'
	float m_fFrameRate;			// frame rate, should large or equal to 0.1
	int m_nFrameSize;			// size of one frame in BYTE
	int m_nFrameNum;			// frame number in all opened YUV files

	int m_nStartFrame;			// first frame to process
	int m_nEndFrame;			// last frame to process
    int m_nStartIDShown;        // the frame number shown in the file name of the first\n");
                                // frame in need. For example, if the output is image\n");
                                // files Img_#####.bmp, the file name of the first frame\n");
                                // processed (i.e. frame 200 numbered in the 'FILE' items\n");
                                // below) is Img_10255.bmp.\n");
	
	int m_nYUVFileNum;			// number of YUV raw data files in this index file
	int m_nYUVFileCount;		// number of YUV file written 
	YUVFileFmt *m_pFileList;		// each item record one YUV raw data file

	bool m_bFirstWrite;		// if write the fist frame

	char m_stIndexFile[256];		// index file name

	// functions
	void message(char stMsg[]);

	bool isKeyChar(char ch);
	bool findKeyword(char stLine[], YUVIndexItemID &id, char stValueString[], int line);

	bool writeIndexFile(char *stExtraMsg = NULL);

public:
	CReadWriteYUV(void);
	~CReadWriteYUV(void);

    bool init(void);

    // for YUV sequence/single file IO

	bool openIDX(char stFilename[], char stMode[]);
	bool openIDX(char stFilename[], int nWidth, int nHeight, YUVFormat nFormat, char stMode[], int start_frame_id = 0, int frame_num = 0);

	int getWidth(void){ return m_bOpen? m_nWidth : 0; }
	int getHeight(void){ return m_bOpen? m_nHeight : 0; }
	int getWidthChroma(void){ return m_bOpen? m_nWidthChroma : 0; }
	int getHeightChroma(void){ return m_bOpen? m_nHeightChroma : 0; }
    int getBitDepth(void){ return m_bOpen? m_nBitDepth : -1; }
	int getFrameNum(void) { return m_bOpen?  m_nFrameNum : 0; }
	int getStartFrame(void) { return m_bOpen?  m_nStartFrame : 0; }
    int getStartIDShown(void) { return m_bOpen?  m_nStartIDShown : 0; }
	int getEndFrame(void) { return m_bOpen?  m_nEndFrame : 0; }
	float getFrameRate(void) { return m_bOpen?  m_fFrameRate : 0; }
	YUVFormat getFormat(void) { return m_bOpen? m_nFormat : IDX_YUV_Unknown; }
	YUVFormat getResultFormat(void) { return m_bOpen? m_nFormatResult : IDX_YUV_Unknown; }
	int getFrameSize(void) { return m_bOpen? m_nFrameSize : 0; }

	IplImage *readFrame(int nFrame);
	bool readFrame(int nFrame, IplImage *iplImageY, IplImage *iplImageU, IplImage *iplImageV);

	bool writeFrame(IplImage *iplImage);
	bool writeFrame(IplImage *iplImageY, IplImage *iplImageU, IplImage *iplImageV);

	void close(char *stExtraMsg = NULL);

    // for YUV single frame file write
    bool writeImage(char stFilePre[], IplImage *iplImageY, IplImage *iplImageU, IplImage *iplImageV);
};

