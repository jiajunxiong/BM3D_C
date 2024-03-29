// YUV IO functions
// Luhong Liang, IC-ASD, ASTRI
// Nov. 2, 2011

#include "stdafx.h"
#include "ImgProcUtility.h"
#include "ReadWriteYUV.h"

void CReadWriteYUV::message(char stMsg[])
{
	printf("%s", stMsg);
}

CReadWriteYUV::CReadWriteYUV(void)
{
    init();

    return;
}

CReadWriteYUV::~CReadWriteYUV(void)
{
	if (m_pFileList != NULL) 
		delete m_pFileList;

	return;
}

bool CReadWriteYUV::init()
{
	m_bOpen = false;
	m_cStatus = 'u';			// unknown
	m_nWidth = -1;				// frame width, -1 as "unknown"
	m_nHeight = -1;				// frame height, -1 as "unknown"
	m_nWidthChroma = 0;
	m_nHeightChroma = 0;
    m_nBitDepth = -1;           // bit depth, -1 as "unknown"
	m_nFrameNum = 0;			// frame number in this YUV file
	m_nStartFrame = 0;
	m_nEndFrame = -1;           // -1 means end of the sequence
    m_nStartIDShown = -1;       // use -1 to denote that this field does not appear
	m_nFormat = IDX_YUV_Unknown;	// a string of YUV format, could be "420", TODO: more format support
	m_fFrameRate = 25.0f;		// frame rate
	m_nFrameSize = 0;			// frame size in BYTE

	m_nYUVFileCount = 0;

	m_nYUVFileNum = 0;			// number of YUV raw data files in this index file
	m_pFileList = NULL;			// each item record one YUV raw data file

	m_bFirstWrite = true;

    return true;
}

bool CReadWriteYUV::isKeyChar(char ch)
// if it is a valid keywork char
{
	if (ch >= '0' && ch <= '9') return true;
	if (ch >= 'a' && ch <= 'z') return true;
	if (ch >= 'A' && ch <='Z') return true;
	if (ch == '_') return true;
	return false;
}

bool CReadWriteYUV::findKeyword(char stLine[], YUVIndexItemID &id, char stValueString[], int line)
// Find keyword from a line, jump over the comments
// by Luhong Liang, ICD-ASD, ASTRI
// Nov. 4, 2011
{
	char stMsg[256];
	YUVIndexItemList pCommandList[] = {	{"Width", KEYWORD_YUV_Width}, {"Height", KEYWORD_YUV_Height}, 
                        {"Bitdepth", KEYWORD_YUV_Bitdepth},
                        {"TotalFrame", KEYWORD_YUV_FrameNum}, {"Format", KEYWORD_YUV_Format}, {"ResultFormat", KEYWORD_YUV_FormatRecom},
                        {"StartFrame", KEYWORD_YUV_StartFrame }, {"EndFrame", KEYWORD_YUV_EndFrame },
                        {"StartIDShown", KEYWORD_YUV_StartIDShown }, {"FrameRate", KEYWORD_YUV_FrameRate},
                        {"FILE", KEYWORD_YUV_RawDataFile} };
	static int nCommandNum = 11;

	// check argument
	if (stLine == NULL || stValueString == NULL) {
		sprintf(stMsg, "Invalid input argument in CReadWriteYUV::findKeyword()!\n");
		message(stMsg);
		return false;
	}

	// remove comments
	char *stComments = strstr( stLine, "//" );
	int nCommandLen;
	if (stComments == NULL) {
		nCommandLen = (int)strlen(stLine);
	} else {
		nCommandLen = (int)(stComments - stLine);
	}
	//assert(nCommandLen < 256);
	if (nCommandLen <= 0 || nCommandLen >= 256) {
		id = KEYWORD_YUV_Unknown;
		stValueString[0] = 0;
		return false;
	}
	char stCommand[256];
	strncpy(stCommand, stLine, nCommandLen);
	stCommand[nCommandLen] = 0;

	// extract keyword
	// remove tab and space before the keyword
	char *pCommand = stCommand;
	while (pCommand[0] != 0 && !isKeyChar(pCommand[0]))
		pCommand ++;
	// extract the keywords until space, tab, 0 or '='
	char stKeyword[256];
	int n;
	for (n=0; n<255; n++) {
		if (pCommand[0] == 0 || !isKeyChar(pCommand[0]))
			break;
		stKeyword[n] = pCommand[0];
		pCommand ++;
	}
	stKeyword[n] = 0;
	if (strlen(stKeyword) < 1)
		return false;

	// search keyword
	bool bFound = false;
	for (int i=0; i<nCommandNum; i++) {
		if (strcmp(stKeyword, pCommandList[i].keyword) == 0) {
			bFound = true;
			// copy command
			id = pCommandList[i].id;
			// copy value
			char *pValue = strstr(pCommand, "=");
			if (pValue != NULL) {
				// jump over '='
				pValue ++;
				// jump over space and tab
				while ( pValue[0] != 0 && (pValue[0] == ' ' || pValue[0] == '\t') )
					pValue ++;
				// copy value string
				int n;
				for (n=0; n<255; n++) {
					if (pValue[n] == 0 || pValue[n] == ' ' || pValue[n] == '\t' || pValue[n] <= 10)
						break;
					stValueString[n] = pValue[n];
				}
				stValueString[n] = 0;
			} else {
				stValueString[0] = 0;
			}
			// quit search
			break;
		}
	}

	if (!bFound) {
		id = KEYWORD_YUV_Unknown;
		sprintf(stMsg, "Unknown command \"%s\" in configure file (line %d)!\n", stKeyword, line);
		message(stMsg);
	}

	return bFound;
}

bool CReadWriteYUV::writeIndexFile(char stExtraMsg[])
// The index file format is:
//			'//' indicates comments
//Width = 640        // frame width, must consistant in all YUV raw data files
//Height = 36        // frame width, must consistant in all YUV raw data files
//Bitdepth = 8       // bit depth of the input frame (support 8-bit only)
//TotalFrame = 500   // frame number. NOTE: in loading operation, this field
//                   // will be ignored; the frame number is calculated using
//                   // the size(s) of raw data file(s).
//StartFrame = 200   // first frame (numbered in the frames listed in the 'FILE'
//                   // items below) to process, starting from 0
//EndFrame = 504     // last frame to process (included)
//                   // will process to the end when EndFrame = -1
//Format = 420       // raw data format,could be '420', '420', '444',
//                   // 'YUYV422', 'UYVY422', 'YUVMS444
//StartIDShown = 10255 // the frame number shown in the file name of the first
//                   // frame in need. For example, if the output is image
//                   // files Img_#####.bmp, the file name of the first frame
//                   // processed (i.e. frame 200 numbered in the 'FILE' items
//                   // below) is Img_10255.bmp.
//ResultFormat = 420 // raw data format of result YUV (*.idx) file
//                   // could be '420', '420', '444', 'YUYV422', 'UYVY422'
//
//FILE = video01.yuv // YUV or image file(s). The final frame order is identical to the
//FILE = video02.yuv // file order here. The YUV files(s) should be placed in
//                      the same folder as the *.idx file!
//			...   ...
// by Luhong Liang, ICD-ASD, ASTRI
// Nov. 4, 2011
{
	// check internal parameters
	//if (m_nYUVFileNum != 1) {
	//		message("Invalid internal YUV number found in CReadWriteYUV::writeIndexFile()!\n");
	//		return false;
	//}

	// open file
	char stMsg[256];
	FILE *fp = fopen(m_stIndexFile, "wt+");
	if (fp == NULL) {
		sprintf(stMsg, "Fail to create index file%s in CReadWriteYUV::writeIndexFile()!\n", m_stIndexFile);
		message(stMsg);
		return false;
	};

	// write contents
	fprintf(fp, "// YUV index file. You can add your own comments begaining with '//'.\n");
	fprintf(fp, "// Developed by Luhong Liang, IC-ASD, ASTRI'.\n");

	// write additional message
	if (stExtraMsg != NULL) 
		fprintf(fp, "%s\n", stExtraMsg);

	// write sequence information
	fprintf(fp, "Width = %d			// frame width, must consistant in all YUV raw data files\n", m_nWidth);
	fprintf(fp, "Height = %d\n", m_nHeight);
    fprintf(fp, "Bitdepth = %d\n", m_nBitDepth);
	fprintf(fp, "TotalFrame = %d	    // frame number. NOTE: in loading operation, this field will be ignored; the actual\n", m_nFrameNum);
	fprintf(fp, "                               		// frame number is calculated using the size(s) of raw data file(s).\n");
	switch (m_nFormat) {
		case 	IDX_YUV420P:		// 4:2:0 format in planes
			fprintf(fp, "Format = 420		// raw data format\n");
			break;
		case IDX_YUV422P:
			fprintf(fp, "Format = 422			// raw data format\n");
			break;
		case IDX_YUYV422:
			fprintf(fp, "Format = YUYV422		// raw data format\n");
			break;
		case IDX_UYVY422:
		case IDX_UYVY422C:
			fprintf(fp, "Format = UYVY422		// raw data format\n");
			break;
		case IDX_YUV444P:
			fprintf(fp, "Format = 444			// raw data format\n");
			break;
		//case YUV_444p:
		//	fprintf(fp, "Format = 444p		// raw data format\n");
		//	break;
		case IDX_YUVMS444:  // converted from RGB 4:4:4 using Microsoft YUV color space (CHROMA_YUVMS)
			fprintf(fp, "Format = YUVMS444			// raw data format\n");
			break;
		case IDX_YUVGrey:
			fprintf(fp, "Format = Grey		// raw data format\n");
			break;
		default:
			message("Invalid YUV data format found in CReadWriteYUV::writeIndexFile()!\n");
			fclose(fp);
			return false;
	}
	switch (m_nFormatResult) {
		case 	IDX_YUV420P:		// 4:2:0 format in planes
			fprintf(fp, "ResultFormat = 420		// raw data format\n");
			break;
		case IDX_YUV422P:
			fprintf(fp, "ResultFormat = 422			// raw data format\n");
			break;
		case IDX_YUYV422:
			fprintf(fp, "ResultFormat = YUYV422		// raw data format\n");
			break;
		case IDX_UYVY422:
		case IDX_UYVY422C:
			fprintf(fp, "ResultFormat = UYVY422		// raw data format\n");
			break;
		case IDX_YUV444P:
			fprintf(fp, "ResultFormat = 444			// raw data format\n");
			break;
		//case YUV_444p:
		//	fprintf(fp, "ResultFormat = 444p		// raw data format\n");
		//	break;
		case IDX_YUVMS444:  // converted from RGB 4:4:4 using Microsoft YUV color space (CHROMA_YUVMS)
			fprintf(fp, "Format = YUVMS444			// raw data format\n");
			break;
		case IDX_YUVGrey:
			fprintf(fp, "ResultFormat = Grey		// raw data format\n");
			break;
		default:
			message("Invalid YUV data format found in CReadWriteYUV::writeIndexFile()!\n");
			fclose(fp);
			return false;
	}
	fprintf(fp, "FrameRate = %f		// FrameRate\n", m_fFrameRate);

	char stFilename[256];
	for (int i=0; i<m_nYUVFileNum; i++) {
		CImageUtility::getFilename(m_pFileList[i].stFile, stFilename);
		if (i == 0) {
			fprintf(fp, "FILE = %s		// Raw data file(s). The final frame order is identical to the file order here. \n", stFilename);
		} else {
			fprintf(fp, "FILE = %s\n", stFilename);
		}
	}

	fclose(fp);

	return true;
}

bool CReadWriteYUV::openIDX(char stFilename[], char stMode[])
// Open the YUV file for READ. Actually, it opens an index file where informations like frame size and raw data files
// are recorded. 
// The index file format is:
//			'//' indicates comments
//Width = 640        // frame width, must consistant in all YUV raw data files
//Height = 36        // frame width, must consistant in all YUV raw data files
//Bitdepth = 8       // bit depth of the input frame (support 8-bit only)
//TotalFrame = 500   // frame number. NOTE: in loading operation, this field
//                   // will be ignored; the frame number is calculated using
//                   // the size(s) of raw data file(s).
//StartFrame = 200   // first frame (numbered in the frames listed in the 'FILE'
//                   // items below) to process, starting from 0
//EndFrame = 504     // last frame to process (included)
//                   // will process to the end when EndFrame = -1
//Format = 420       // raw data format,could be '420', '420', '444',
//                   // 'YUYV422', 'UYVY422', 'YUVMS444
//StartIDShown = 10255 // the frame number shown in the file name of the first
//                   // frame in need. For example, if the output is image
//                   // files Img_#####.bmp, the file name of the first frame
//                   // processed (i.e. frame 200 numbered in the 'FILE' items
//                   // below) is Img_10255.bmp.
//ResultFormat = 420 // raw data format of result YUV (*.idx) file
//                   // could be '420', '420', '444', 'YUYV422', 'UYVY422'
//
//FILE = video01.yuv // YUV or image file(s). The final frame order is identical to the
//FILE = video02.yuv // file order here. The YUV files(s) should be placed in
//                      the same folder as the *.idx file!
//			...   ...
// Arguments:
//		stFilename -- [I] file path of the YUV index file. NOTE: The index file and the raw data file(s) must be in the same folder!
//      stMode -- [I] mode of the open operation; could be "r" (read), "w" (create and write), "a" (append to existing file, not supported yet)
// Return:
//		Return true for sucess.
// by Luhong Liang, ICD-ASD, ASTRI
// Nov. 4, 2011
{
	char stMsg[256];

    // close the previous YUV file
    close();

    // clear the sequence information
	init();

	// check input
	if (stFilename == NULL || stFilename[0] == 0 || stMode == NULL) {
		message("Invalid input argument in CReadWriteYUV::open()!\n");
		return false;
	}

	strncpy(m_stIndexFile, stFilename, 256);

	// -------------------------------------
	// read
	// -------------------------------------
	FILE *fp;
	if (strcmp(stMode, "r") == 0) {
		// open file
		fp = fopen(stFilename, "rt");
		if (fp == NULL) {
			sprintf(stMsg, "Fail to open file %s in CReadWriteYUV::open()!\n", stFilename);
			message(stMsg);
			return false;
		}

		m_nFormatResult = IDX_YUV420P;		// default result YUV file format, to be compatible with previous *.idx files

		// load and parse index file
		bool bError = false;
		char stLine[256];
		int nLineNum = 0;
		YUVIndexItemID id;
		char stValue[256];
		int nTemp;
		float fTemp;
		int nFileNum = 0;
		while (!feof(fp) && fgets(stLine, 256, fp) != NULL) {
			nLineNum ++;
			if (!findKeyword(stLine, id, stValue, nLineNum)) continue;
			switch (id) {
				case KEYWORD_YUV_Width:
					if (sscanf(stValue, "%d", &nTemp) != 1) {
						sprintf(stMsg, "Invalid input item 'Width' in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
						break;
					}
					m_nWidth = nTemp;
                    if (m_nWidth < 1 && m_nWidth != -1) {  // -1 for "unknown"
						sprintf(stMsg, "Invalid/unsupported 'Width' in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
					}
					break;
				case KEYWORD_YUV_Height:
					if (sscanf(stValue, "%d", &nTemp) != 1) {
						sprintf(stMsg, "Invalid input item 'Height' in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
						break;
					}
					m_nHeight = nTemp;
                    if (m_nHeight < 1 && m_nHeight != -1) { // -1 for "unknown"
						sprintf(stMsg, "Invalid/unsupported 'Height' in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
					}
					break;
				case KEYWORD_YUV_Bitdepth:
					if (sscanf(stValue, "%d", &nTemp) != 1) {
						sprintf(stMsg, "Invalid input item 'Bitdepth' in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
						break;
					}
					m_nBitDepth = nTemp;
                    if (m_nBitDepth != 8 && m_nBitDepth != -1) {     // -1 for "unknown"
						sprintf(stMsg, "Only support 8-bit YUV file (BitDepth=8) or image file (BitDepth=-1) in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
					}
					break;
				case KEYWORD_YUV_FrameNum:
					if (sscanf(stValue, "%d", &nTemp) != 1) {
						sprintf(stMsg, "Invalid input item 'TotalFrame' in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
						break;
					}
					m_nFrameNum = nTemp;
					break;
				case KEYWORD_YUV_StartFrame:
					if (sscanf(stValue, "%d", &nTemp) != 1) {
						sprintf(stMsg, "Invalid input item 'StartFrame' in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
						break;
					}
					m_nStartFrame = nTemp;
                    if (m_nStartFrame < 0) {
						sprintf(stMsg, "Invalid/unsupported 'StartFrame' in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
					}
					break;
				case KEYWORD_YUV_EndFrame:
					if (sscanf(stValue, "%d", &nTemp) != 1) {
						sprintf(stMsg, "Invalid input item 'EndFrame' in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
						break;
					}
					m_nEndFrame = nTemp;
                    if (m_nEndFrame < -1) {
						sprintf(stMsg, "Invalid/unsupported 'EndFrame' in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
					}
					break;
                case KEYWORD_YUV_StartIDShown:
					if (sscanf(stValue, "%d", &nTemp) != 1) {
						sprintf(stMsg, "Invalid input item 'StartIDShown' in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
						break;
					}
					m_nStartIDShown = nTemp;
                    if (m_nStartIDShown < -1) {
						sprintf(stMsg, "Invalid/unsupported 'StartIDShown' in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
					}
					break;
				case KEYWORD_YUV_Format:
					if (strcmp(stValue, "420") == 0) {		
						m_nFormat = IDX_YUV420P;
					} else if (strcmp(stValue, "422") == 0) {		
						m_nFormat = IDX_YUV422P;
					} else if (strcmp(stValue, "444") == 0) {		
						m_nFormat = IDX_YUV444P;
					} else if (strcmp(stValue, "YUVMS444") == 0) {			
						m_nFormat = IDX_YUVMS444; // converted from RGB 4:4:4 using Microsoft YUV color space (CHROMA_YUVMS)
					} else if (strcmp(stValue, "YUYV422") == 0) {			
						m_nFormat = IDX_YUYV422;
					} else if (strcmp(stValue, "UYVY422") == 0) {			
						m_nFormat = IDX_UYVY422;                  
                    } else {
						sprintf(stMsg, "Unsupported raw data format in line %d in *.idx file!\nOnly support 420P ('420'), YUYV422 ('YUYV422') and UYVY422('UYVY42')!\n", nLineNum);
						message(stMsg);
						bError = true;
					}
					break;
				case KEYWORD_YUV_FormatRecom:
					if (strcmp(stValue, "420") == 0) {		
						m_nFormatResult = IDX_YUV420P;
                    } else if (strcmp(stValue, "422") == 0) {		
						m_nFormatResult = IDX_YUV422P;
					} else if (strcmp(stValue, "444") == 0) {		
						m_nFormatResult = IDX_YUV444P;
					} else if (strcmp(stValue, "YUVMS444") == 0) {			
						m_nFormatResult = IDX_YUVMS444; // converted from RGB 4:4:4 using Microsoft YUV color space (CHROMA_YUVMS)
					} else if (strcmp(stValue, "YUYV422") == 0) {			
						m_nFormatResult = IDX_YUYV422;
					} else if (strcmp(stValue, "UYVY422") == 0) {			
						m_nFormatResult = IDX_UYVY422;
					} else {
						sprintf(stMsg, "Unsupported raw data format in line %d in *.idx file!\nOnly support 420P ('420'), YUYV422 ('YUYV422') and UYVY422('UYVY42')!\n", nLineNum);
						message(stMsg);
						bError = true;
					}
					break;
				case KEYWORD_YUV_FrameRate:
					if (sscanf(stValue, "%f", &fTemp) != 1 || fTemp < 0.1f) {
						sprintf(stMsg, "Invalid input item 'FrameRate' in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
						break;
					}
					m_fFrameRate = fTemp;
                    if (m_fFrameRate < 0.0f) {
						sprintf(stMsg, "Invalid/unsupported 'FrameRate' in line %d in *.idx file!\n", nLineNum);
						message(stMsg);
						bError = true;
					}
				case KEYWORD_YUV_RawDataFile:
					nFileNum ++;
					break;
				default:
					break;
			}
		}
		if (bError) {
			fclose(fp);
			return false;
		}

		// calculate and check frame size
        // TODO: support bit depth other than 8
		bError = false;
		switch (m_nFormat) {
			case IDX_YUVGrey:
				m_nWidthChroma =0;
				m_nHeightChroma = 0;
				m_nFrameSize = m_nWidth * m_nHeight;
				if (m_nFrameSize < 1) bError = true;
				break;
			case IDX_YUV420P:		// 4:2:0 format in planes
				m_nWidthChroma = m_nWidth / 2;
				m_nHeightChroma = m_nHeight / 2;
				m_nFrameSize = m_nWidth * m_nHeight + 2 * m_nWidthChroma * m_nHeightChroma;
				if (m_nFrameSize < 1) bError = true;
				break;
			case IDX_YUYV422:
			case IDX_UYVY422:
				m_nWidthChroma = m_nWidth / 2;
				m_nHeightChroma = m_nHeight;
				m_nFrameSize = m_nWidth * m_nHeight + 2 * m_nWidthChroma * m_nHeightChroma;
				if (m_nFrameSize < 1) bError = true;
				break;
			case IDX_YUV444P:
            case IDX_YUVMS444: // converted from RGB 4:4:4 using Microsoft YUV color space (CHROMA_YUVMS)
				m_nWidthChroma = m_nWidth;
				m_nHeightChroma = m_nHeight;
				m_nFrameSize = m_nWidth * m_nHeight + 2 * m_nWidthChroma * m_nHeightChroma;
				if (m_nFrameSize < 1) bError = true;
				break;
			case IDX_YUV422P:
				m_nWidthChroma = m_nWidth / 2;
				m_nHeightChroma = m_nHeight;
				m_nFrameSize = m_nWidth * m_nHeight + 2 * m_nWidthChroma * m_nHeightChroma;
				if (m_nFrameSize < 1) bError = true;
				break;
			case IDX_YUV_Unknown:
			default:
				bError = true;
				sprintf(stMsg, "Unsupported YUV data format in *.idx file!\n");
				message(stMsg);
				break;
		}
		if (bError) {
			fclose(fp);
			return false;
		}

		// allocate file list
		m_pFileList = new YUVFileFmt[nFileNum];
		if (m_pFileList == NULL) {
			sprintf(stMsg, "Fail to allocate file list in *.idx file!\n");
			message(stMsg);
			fclose(fp);
			return false;
		}

		// set file list
		fseek(fp, 0, SEEK_SET);
		int n = 0;
		while (!feof(fp) && fgets(stLine, 256, fp) != NULL) {
			if (n < nFileNum) {
				if (findKeyword(stLine, id, stValue, n)) {
					if (id == KEYWORD_YUV_RawDataFile) {
						strncpy(m_pFileList[n].stFile, stValue, 256);
						m_pFileList[n].nWidth = m_nWidth;
						m_pFileList[n].nHeight = m_nHeight;
						m_pFileList[n].nWidthChroma = m_nWidthChroma;
						m_pFileList[n].nHeightChroma = m_nHeightChroma;
						m_pFileList[n].nFrameNum = m_nFrameNum;
						m_pFileList[n].nFormat = m_nFormat;
						n++;
					}
				}
			}
		}
		m_nYUVFileNum = n;
		fclose(fp);
		
		// check raw data files
		bError = false;
		int nGlobalindex = 0;
		FILE *fp;
		char stFilePath[256];
		char stRawDataFile[256];
        char stFileExt[32];
		CImageUtility::getFilePath(stFilename, stFilePath);
        int yuv_num = 0;
        int img_num = 0;
		for (int i=0; i<m_nYUVFileNum; i++) {
			// update raw data file name
			if (stFilePath[0] == 0) {
				strcpy(stRawDataFile,  m_pFileList[i].stFile);
			} else {
				sprintf(stRawDataFile, "%s\\%s", stFilePath, m_pFileList[i].stFile);
			}
            strncpy(m_pFileList[i].stFile, stRawDataFile, 256);
            // check file extension (modified to support image list, May 20, 2015)
            int nFrameNum = 0;
            CImageUtility::getFileExt(m_pFileList[i].stFile, stFileExt);
            if (strcmp(strlwr(stFileExt), "yuv") == 0) {
                // open YUV raw data file
			    fp = fopen(stRawDataFile, "rb");
			    if (fp == NULL) {
				    sprintf(stMsg, "Fail to open YUV raw data file %s!\n", stRawDataFile);
				    message(stMsg);
				    bError = true;
				    continue;
			    }
			    // check raw data size
			    _fseeki64(fp, 0, SEEK_END);	//fseek(fp, 0, SEEK_END);
			    _int64 nFileSize = _ftelli64(fp);	//ftell(fp);
			    // close file
			    fclose(fp);
                // count frame number
			    nFrameNum = (int)(nFileSize / m_nFrameSize);
			    if (nFrameNum == 0 || m_nFrameSize * nFrameNum != nFileSize) {
				    sprintf(stMsg, "Warning: file %s is empty or has frame fragement!\n", stRawDataFile);
				    message(stMsg);
			    }
                yuv_num ++;
            } else {
                // frame number increase only one for image file 
                nFrameNum = 1;
                img_num ++;
            }
            // update file list and total frame number
            m_pFileList[i].nFrameNum = nFrameNum;
            m_pFileList[i].nGlobalindex = nGlobalindex;
            nGlobalindex += nFrameNum;
		}
		m_nFrameNum = nGlobalindex;

        if (m_nYUVFileNum > 0 && yuv_num > 0 && img_num > 0) {
            sprintf(stMsg, "Do not support hybrid list of YUV and image files in *.idx!\n", stRawDataFile);
            message(stMsg);
            bError = true;
        }

        // calculate the frame size by loading one frame (must do this since there may be "get width/height" operations
        // just after the openIDX()!)
        if (img_num > 0) {
            // clear the width/height/depth in the IDX file
            m_nWidth = -1;
            m_nHeight = -1;
            m_nBitDepth = -1;
            m_nFrameSize = 0;
            // read image to get the size
            IplImage *iplTmp = readFrame(m_nStartFrame); // width, height and bit depth are set here
            if (iplTmp == NULL) {
                sprintf(stMsg, "Fail to read the image size by loading the start image in the file list in *.idx!\n", stRawDataFile);
                message(stMsg);
                bError = true;
            }
            CImageUtility::safeReleaseImage(&iplTmp);
        }
		// set status
		m_cStatus = 'r';
		if (!bError) 
			m_bOpen = true;

		return !bError;
	}

	message("Invalid file open mode in CReadWriteYUV::open()!\n");
	return false;
}

bool CReadWriteYUV::openIDX(char stFilename[], int nWidth, int nHeight, YUVFormat nFormat, char stMode[], int start_frame_id, int frame_num)
// Open the YUV file for WRITE. Actually, it opens an index file where informations like frame size and raw data files
// are recorded.
// The index file format is:
//			'//' indicates comments
//Width = 640        // frame width, must consistant in all YUV raw data files
//Height = 36        // frame width, must consistant in all YUV raw data files
//Bitdepth = 8       // bit depth of the input frame (support 8-bit only)
//TotalFrame = 500   // frame number. NOTE: in loading operation, this field
//                   // will be ignored; the frame number is calculated using
//                   // the size(s) of raw data file(s).
//StartFrame = 200   // first frame (numbered in the frames listed in the 'FILE'
//                   // items below) to process, starting from 0
//EndFrame = 504     // last frame to process (included)
//                   // will process to the end when EndFrame = -1
//Format = 420       // raw data format,could be '420', '420', '444',
//                   // 'YUYV422', 'UYVY422', 'YUVMS444
//StartIDShown = 10255 // the frame number shown in the file name of the first
//                   // frame in need. For example, if the output is image
//                   // files Img_#####.bmp, the file name of the first frame
//                   // processed (i.e. frame 200 numbered in the 'FILE' items
//                   // below) is Img_10255.bmp.
//ResultFormat = 420 // raw data format of result YUV (*.idx) file
//                   // could be '420', '420', '444', 'YUYV422', 'UYVY422'
//
//FILE = video01.yuv // YUV or image file(s). The final frame order is identical to the
//FILE = video02.yuv // file order here. The YUV files(s) should be placed in
//                      the same folder as the *.idx file!
//			...   ...
// Arguments:
//		stFilename -- [I] file path of the YUV index file. NOTE: The index file and the raw data file(s) must be in the same folder!
//		nWidth -- [I] width of frame, i.e. width of Y plane
//		nHeight -- [I] height of frame, i.e. height of Y plane
//		nFormat -- [I] YUV format ,could be IDX_YUVGrey, IDX_YUV420P, IDX_YUV422P, IDX_YUV444P, IDX_YUYV422, IDX_UYVY422, IDX_YUVMS444  TODO: more supports
//      stMode -- [I] mode of the open operation; could be "r" (read), "w" (create and write), "a" (append to existing file, not supported yet)
//		frame_num -- [I] frame number of the video; 0 for unknown
//		start_frame_id -- [I] start frame number, UYVU files only!
// Return:
//		Return true for sucess.
// by Luhong Liang, ICD-ASD, ASTRI
// Nov. 4, 2011
{
	char stMsg[256];

    // close the previous YUV file
    close();

    // clear the sequence information
	init();

	// check input
	if (stFilename == NULL || stFilename[0] == 0 || stMode == NULL) {
		message("Invalid input argument in CReadWriteYUV::open()!\n");
		return false;
	}

	strncpy(m_stIndexFile, stFilename, 256);

	if (!(strcmp(stMode, "w") == 0 || strcmp(stMode, "a") == 0)) {
		message("Invalid file open mode in CReadWriteYUV::open()!\n");
		return false;
	}

	m_nWidth = nWidth;
	m_nHeight = nHeight;

	// calculate and check frame size
	switch (nFormat) {
		case IDX_YUVGrey:
			m_nFormat = nFormat;
			m_nWidthChroma = 0;
			m_nHeightChroma = 0;
			m_nFrameSize = m_nWidth * m_nHeight;
			if (m_nFrameSize < 1) {
				sprintf(stMsg, "Invalid frame size!\n");
				message(stMsg);
				return false;
			}
			break;
		case IDX_YUV420P:		// 4:2:0 format in planes
			m_nFormat = nFormat;
			m_nWidthChroma = m_nWidth / 2;
			m_nHeightChroma = m_nHeight / 2;
			m_nFrameSize = m_nWidth * m_nHeight + 2 * m_nWidthChroma * m_nHeightChroma;
			if (m_nFrameSize < 1) {
				sprintf(stMsg, "Invalid frame size!\n");
				message(stMsg);
				return false;
			}
			break;
		case IDX_YUYV422:
		case IDX_UYVY422:
		case IDX_UYVY422C:
			m_nFormat = nFormat;
			m_nWidthChroma = m_nWidth / 2;
			m_nHeightChroma = m_nHeight;
			m_nFrameSize = m_nWidth * m_nHeight + 2 * m_nWidthChroma * m_nHeightChroma;
			if (m_nFrameSize < 1) {
				sprintf(stMsg, "Invalid frame size!\n");
				message(stMsg);
				return false;
			}
			break;
		case IDX_YUV422P:		// 4:2:2 format in planes
			m_nFormat = nFormat;
			m_nWidthChroma = m_nWidth;
			m_nHeightChroma = m_nHeight / 2;
			m_nFrameSize = m_nWidth * m_nHeight + 2 * m_nWidthChroma * m_nHeightChroma;
			if (m_nFrameSize < 1) {
				sprintf(stMsg, "Invalid frame size!\n");
				message(stMsg);
				return false;
			}
			break;
		case IDX_YUV444P:		// 4:4:4 format in planes
        case IDX_YUVMS444:		// converted from RGB 4:4:4 using Microsoft YUV color space (CHROMA_YUVMS)
			m_nFormat = nFormat;
			m_nWidthChroma = m_nWidth;
			m_nHeightChroma = m_nHeight;
			m_nFrameSize = m_nWidth * m_nHeight + 2 * m_nWidthChroma * m_nHeightChroma;
			if (m_nFrameSize < 1) {
				sprintf(stMsg, "Invalid frame size!\n");
				message(stMsg);
				return false;
			}
			break;
		case IDX_YUV_Unknown:
		default:
			sprintf(stMsg, "Unsupported YUV data format!\n");
			message(stMsg);
			return false;
	}

	m_nFormatResult = m_nFormat;

	// allocate file list
	if (nFormat == IDX_UYVY422C) {
		m_nYUVFileNum = frame_num <= 0 ? MAX_YUV_FILE_NUM : frame_num;
	} else {
		m_nYUVFileNum = 1;
	}
	m_pFileList = new YUVFileFmt[m_nYUVFileNum];
	if (m_pFileList == NULL) {
		sprintf(stMsg, "Fail to allocate file list in CReadWriteYUV::open()!\n");
		message(stMsg);
		return false;
	}

	// set file list
	char stFilePre[256];
	CImageUtility::getFilePre(stFilename, stFilePre);
	for (int i=0; i<m_nYUVFileNum; i++) {
		if (m_nYUVFileNum == 1) {
			sprintf(m_pFileList[i].stFile, "%s_%04d.yuv", stFilePre, start_frame_id);
		} else {
			sprintf(m_pFileList[i].stFile, "%s_%04d.yuv", stFilePre, i+start_frame_id);
		}
		m_pFileList[i].nWidth = m_nWidth;
		m_pFileList[i].nHeight = m_nHeight;
		m_pFileList[i].nWidthChroma = m_nWidthChroma;
		m_pFileList[i].nHeightChroma = m_nHeightChroma;
		m_pFileList[i].nFrameNum = m_nFrameNum;
		m_pFileList[i].nFormat = m_nFormat;
	}

	// set file count
	m_nYUVFileCount = 0;

	// -------------------------------------
	// write
	// -------------------------------------
	FILE *fp;
	if (strcmp(stMode, "w") == 0) {
		// create an empty index file
		fp = fopen(stFilename, "wt+");
		if (fp == NULL) {
			sprintf(stMsg, "Fail to create file %s!\n", stFilename);
			message(stMsg);
			return false;
		}
		fclose(fp);
		// create an empty YUV raw data file
		fp = fopen(m_pFileList[0].stFile, "wt+");
		if (fp == NULL) {
			sprintf(stMsg, "Fail to create file %s!\n", m_pFileList[0].stFile);
			message(stMsg);
			return false;
		}
		fclose(fp);

		// set status
		m_cStatus = 'w';
		m_bOpen = true;
		m_bFirstWrite = true;

		return true;
	}

	// -------------------------------------
	// write & append
	// -------------------------------------
	if (strcmp(stMode, "a") == 0) {
		//m_cStatus = 'a';
		message("Does not support append operan in current version in CReadWriteYUV::open()!\n");
		return false;
	}

	message("Invalid file open mode in CReadWriteYUV::open()!\n");
	return false;
}

IplImage *CReadWriteYUV::readFrame(int nFrame)
// read one frame and converted to RGB format
// May 20, 2015: support image file list in IDX file
{
	// locate YUV sequence
	int nFileNum = -1;
	for (int i=0; i<m_nYUVFileNum; i++) {
		if (nFrame >= m_pFileList[i].nGlobalindex && nFrame<m_pFileList[i].nGlobalindex+m_pFileList[i].nFrameNum) {
			nFileNum = i;
			break;
		}
	}
	if (nFileNum < 0) {
        CImageUtility::showErrMsg("Frame number exceed YUV file length or invalid frame number in CReadWriteYUV::readFrame()!\n");
        return NULL;
	}
	int nFrameNum = nFrame - m_pFileList[nFileNum].nGlobalindex;

    // open image file (support other image formats, May 20, 2015)
    IplImage *iplImage = NULL;
    char stFilename[256];
    char stFileExt[64];
    CImageUtility::getFilename(m_pFileList[nFileNum].stFile, stFilename); 
    CImageUtility::getFileExt(m_pFileList[nFileNum].stFile, stFileExt);
    if (strcmp(strlwr(stFileExt), "yuv") == 0) {
	    // open YUV file
	    FILE *fp = fopen(m_pFileList[nFileNum].stFile, "rb");
	    if (fp == NULL) {
            CImageUtility::showErrMsg("Fail to open YUV file %s in CReadWriteYUV::readFrame()!\n", m_pFileList[nFileNum].stFile);
            return NULL;
	    }

	    // allocate buffer
	    char *pBuf = new char[m_nFrameSize];
	    if (pBuf == NULL) {
            CImageUtility::showErrMsg("Fail to allocate buffer in CReadWriteYUV::readFrame()!\n");
            fclose(fp);
            return NULL;
	    }

	    // read raw data
	    _int64 nOffset = (_int64)nFrameNum * (_int64)m_nFrameSize;
	    _fseeki64(fp, nOffset, SEEK_SET);
	    int nCount = (int)fread(pBuf, sizeof(char), m_nFrameSize, fp);
	    fclose(fp);
	    if (nCount != m_nFrameSize) {
            CImageUtility::showErrMsg("Fail to read data in YUV file %s in CReadWriteYUV::readFrame()!\n", m_pFileList[nFileNum].stFile);
            delete pBuf;
            return NULL;
	    } 

	    // fill IPL image
	    int y;
	    char *pDst, *pSrc;
	    switch (m_nFormat) {
		    case IDX_YUVGrey:		
			    // create grey image
			    iplImage = CImageUtility::createImage(m_nWidth, m_nHeight, SR_DEPTH_8U, 1);
			    if (iplImage == NULL) {
				    CImageUtility::showErrMsg("Fail to allocate IPL image in CReadWriteYUV::readFrame()!\n");
				    delete pBuf;
				    return NULL;
			    }
			    // copy Y plane
			    pSrc = pBuf;
			    for (y=0; y<iplImage->height; y++) {		// should copy line by line since IplImage is word-aligned!
				    pDst = iplImage->imageData + y * iplImage->widthStep;
				    memcpy(pDst, pSrc, iplImage->width);
				    pSrc += m_nWidth;
			    }
			    break;
		    case IDX_YUV420P:		// 4:2:0 format in planes
		    case IDX_YUV422P:
		    case IDX_YUYV422:
		    case IDX_UYVY422:
		    case IDX_YUV444P:
            case IDX_YUVMS444:// converted from RGB 4:4:4 using Microsoft YUV color space (CHROMA_YUVMS)
		    default:
			    message("Invalid YUV or unsupported data format in CReadWriteYUV::readFrame()!\n");
			    delete pBuf;
			    return NULL;
	    } 
	
	    // relase
	    delete pBuf;
    } else {
        // load image (support other image formats, May 20, 2015)
        int bit_depth;
        iplImage = CImageUtility::loadImage(m_pFileList[nFileNum].stFile, bit_depth);
        if (iplImage == NULL) {
            CImageUtility::showErrMsg("Fail to load image file %s in CReadWriteYUV::readFrame()!\n", stFilename);
            return false;
        }
        if (m_nBitDepth == -1) {
            m_nBitDepth = bit_depth;    // initial setting
        } else {
            if (bit_depth != m_nBitDepth) {
                CImageUtility::showErrMsg("Inconsistent bitdepth in frame(s) or IDX file in CReadWriteYUV::readFrame()!\n", stFilename);
                CImageUtility::safeReleaseImage(&iplImage);
                return NULL;
            }
        }
        if (iplImage->width < 1 || iplImage->height < 1) {
            CImageUtility::showErrMsg("Invalid image size parsed in CReadWriteYUV::readFrame()!\n", stFilename);
            CImageUtility::safeReleaseImage(&iplImage);
            return false;
        }
        if (m_nWidth == -1 && m_nHeight == -1) {
            // the first image loaded
            m_nWidth = iplImage->width;
            m_nHeight = iplImage->height;
            m_nWidthChroma = m_nWidth;
            m_nHeightChroma = m_nHeight;
            m_nFormat = IDX_YUVMS444;   // CHROMA_YUVMS
            m_nFrameSize = m_nWidth * m_nHeight * iplImage->nChannels; // suppose 4:4:4
            
        } else {
            // check the frame size for successive frames
            if (iplImage->width != m_nWidth  || iplImage->height != m_nHeight) {
                CImageUtility::showErrMsg("Size of image file %s does not match the frame size in CReadWriteYUV::readFrame()!\n", stFilename);
                CImageUtility::safeReleaseImage(&iplImage);
                return false;
            }
        }
    }

	return iplImage;
}

bool CReadWriteYUV::readFrame(int nFrame, IplImage *iplImageY, IplImage *iplImageU, IplImage *iplImageV)
// read one frame
// May 20, 2015: support image file list in IDX file
{
	// check input
	if (iplImageY == NULL || iplImageU == NULL || iplImageV == NULL || 
		iplImageY->width != m_nWidth || iplImageY->height != m_nHeight || 
		iplImageU->width != m_nWidthChroma || iplImageU->height != m_nHeightChroma || 
		iplImageV->width != m_nWidthChroma || iplImageV->height != m_nHeightChroma) {
			CImageUtility::showErrMsg("Invalid input IPL images in CReadWriteYUV::readFrame()!\n");
			return false;
	}

	// locate YUV sequence
	int nFileNum = -1;
	for (int i=0; i<m_nYUVFileNum; i++) {
		if (nFrame >= m_pFileList[i].nGlobalindex && nFrame<m_pFileList[i].nGlobalindex+m_pFileList[i].nFrameNum) {
			nFileNum = i;
			break;
		}
	}
	if (nFileNum < 0) {
        CImageUtility::showErrMsg("Frame number exceed YUV file length or invalid frame number in CReadWriteYUV::readFrame()!\n");
        return false;
	}
	int nFrameNum = nFrame - m_pFileList[nFileNum].nGlobalindex;

    // open image file (support other image formats, May 20, 2015)
    char stFilename[256];
    char stFileExt[64];
    CImageUtility::getFilename(m_pFileList[nFileNum].stFile, stFilename); 
    CImageUtility::getFileExt(m_pFileList[nFileNum].stFile, stFileExt);
    if (strcmp(strlwr(stFileExt), "yuv") == 0) {
	    // open YUV file
	    FILE *fp = fopen(m_pFileList[nFileNum].stFile, "rb");
	    if (fp == NULL) {
            CImageUtility::showErrMsg("Fail to open YUV file %s in CReadWriteYUV::readFrame()!\n", stFilename);
            return false;
	    }

	    // allocate buffer
	    char *pBuf = new char[m_nFrameSize];
	    if (pBuf == NULL) {
            CImageUtility::showErrMsg("Fail to allocate buffer in CReadWriteYUV::readFrame()!\n");
            fclose(fp);
            return false;
	    }

	    // read raw data
	    _int64 nOffset = (_int64)nFrameNum * (_int64)m_nFrameSize;
	    _fseeki64(fp, nOffset, SEEK_SET);
	    int nCount = (int)fread(pBuf, sizeof(char), m_nFrameSize, fp);
	    fclose(fp);
	    if (nCount != m_nFrameSize) {
            CImageUtility::showErrMsg("Fail to read data in YUV file %s in CReadWriteYUV::readFrame()!\n", stFilename);
            delete pBuf;
            return false;
	    } 

	    // fill IPL image
	    int xxx, xx, x, y;
	    char *pDst, *pSrc, *pDstU, *pDstV;
	    switch (m_nFormat) {
		    case IDX_YUV420P:		// 4:2:0 format in planes
            case IDX_YUV422P:		// 4:2:2 format in planes
            case IDX_YUV444P:		// 4:4:4 format in planes
            case IDX_YUVMS444:      // converted from RGB 4:4:4 using Microsoft YUV color space (CHROMA_YUVMS)
			    // copy Y plane
			    pSrc = pBuf;
			    for (y=0; y<iplImageY->height; y++) {		// should copy line by line since IplImage is word-aligned!
				    pDst = iplImageY->imageData + y * iplImageY->widthStep;
				    memcpy(pDst, pSrc, iplImageY->width);
				    pSrc += m_nWidth;
			    }
			    // copy U plane
			    pSrc = pBuf + m_nWidth * m_nHeight;
			    for (y=0; y<iplImageU->height; y++) {		// should copy line by line since IplImage is word-aligned!
				    pDst = iplImageU->imageData + y * iplImageU->widthStep;
				    memcpy(pDst, pSrc, iplImageU->width);
				    pSrc += m_nWidthChroma;
			    }
			    // copy V plane
			    pSrc = pBuf + m_nWidth * m_nHeight + m_nWidthChroma * m_nHeightChroma;
			    for (y=0; y<iplImageV->height; y++) {		// should copy line by line since IplImage is word-aligned!
				    pDst = iplImageV->imageData + y * iplImageV->widthStep;
				    memcpy(pDst, pSrc, iplImageV->width);
				    pSrc += m_nWidthChroma;
			    }
			    break;
		    case IDX_YUYV422:
			    // copy Y U V plane
			    for (y=0; y<iplImageY->height; y++) {		// should copy line by line since IplImage is word-aligned!
				    pDst = iplImageY->imageData + y * iplImageY->widthStep;
				    pDstU = iplImageU->imageData + y * iplImageU->widthStep;
				    pDstV = iplImageV->imageData + y * iplImageV->widthStep;
				    pSrc = pBuf + y * (m_nWidth + m_nWidthChroma + m_nWidthChroma);
				    for (x=0, xx=0, xxx=0; x<iplImageY->width; x+=2, xx+=4, xxx++) {
					    // copy one YUYV unit
					    pDst[x] = pSrc[xx];		// Y
					    pDstU[xxx] = pSrc[xx+1];	// U
					    pDst[x+1] = pSrc[xx+2];	// Y
					    pDstV[xxx] = pSrc[xx+3];	// V
				    }
			    }
			    break;
		    case IDX_UYVY422:
			    // copy Y U V plane
			    for (y=0; y<iplImageY->height; y++) {		// should copy line by line since IplImage is word-aligned!
				    pDst = iplImageY->imageData + y * iplImageY->widthStep;
				    pDstU = iplImageU->imageData + y * iplImageU->widthStep;
				    pDstV = iplImageV->imageData + y * iplImageV->widthStep;
				    pSrc = pBuf + y * (m_nWidth + m_nWidthChroma + m_nWidthChroma);
				    for (x=0; x<iplImageU->width; x++) {
					    // copy one YUYV unit
					    int xx_y = x * 2;
					    int xx_s = x * 4;
					    pDstU[x] = pSrc[xx_s];	// U
					    pDst[xx_y] = pSrc[xx_s+1];		// Y
					    pDstV[x] = pSrc[xx_s+2];	// V
					    pDst[xx_y+1] = pSrc[xx_s+3];	// Y
					
				    }
			    }
			    break;
		    case IDX_YUVGrey:
			    // copy Y plane
			    pSrc = pBuf;
			    for (y=0; y<iplImageY->height; y++) {		// should copy line by line since IplImage is 4/64-byte-aligned!
				    pDst = iplImageY->imageData + y * iplImageY->widthStep;
				    memcpy(pDst, pSrc, iplImageY->width);
				    pSrc += m_nWidth;
			    }
                break;
		    default:
			    CImageUtility::showErrMsg("Invalid YUV or unsupported data format in CReadWriteYUV::readFrame()!\n");
			    delete pBuf;
			    return false;
	    } 
	
	    // relase
	    delete pBuf;
    } else {    // Support other image formats, May 20, 2015
        // load image
        int bit_depth;
        IplImage *iplImg = CImageUtility::loadImage(m_pFileList[nFileNum].stFile, bit_depth);
        if (iplImg == NULL) {
            CImageUtility::showErrMsg("Fail to load image file %s in CReadWriteYUV::readFrame()!\n", stFilename);
            return false;
        }
        if (m_nBitDepth == -1) {
            m_nBitDepth = bit_depth;    // initial setting
        } else {
            if (bit_depth != m_nBitDepth) {
                CImageUtility::showErrMsg("Inconsistent bitdepth in frame(s) or IDX file in CReadWriteYUV::readFrame()!\n", stFilename);
                CImageUtility::safeReleaseImage(&iplImg);
                return NULL;
            }
        }
        if (iplImg->width < 1 || iplImg->height < 1) {
            CImageUtility::showErrMsg("Invalid image size parsed in CReadWriteYUV::readFrame()!\n", stFilename);
            CImageUtility::safeReleaseImage(&iplImg);
            return false;
        }
        if (m_nWidth == -1 && m_nHeight == -1) {
            // the first image loaded
            m_nWidth = iplImg->width;
            m_nHeight = iplImg->height;
            m_nWidthChroma = m_nWidth;
            m_nHeightChroma = m_nHeight;
            m_nFormat = IDX_YUVMS444;// CHROMA_YUVMS
            m_nFrameSize = m_nWidth * m_nHeight * iplImg->nChannels; // support 4:4:4 image only
        } else {
            // check the frame size for successive frames
            if (iplImg->width != m_nWidth  || iplImg->height != m_nHeight) {
                CImageUtility::showErrMsg("Size of image file %s does not match the frame size in CReadWriteYUV::readFrame()!\n", stFilename);
                CImageUtility::safeReleaseImage(&iplImg);
                return false;
            }
        }
        // RGB to YUV
        if (iplImg->nChannels == 1) {
            if (m_nFormat != IDX_YUVGrey) {
                CImageUtility::showErrMsg("Warning: Grey image file %s loaded in CReadWriteYUV::readFrame()!\n", stFilename);
            }
            if (iplImageY->depth == SR_DEPTH_8U) {
                if (iplImg->depth == SR_DEPTH_8U) {
                    CImageUtility::copy(iplImg, iplImageY);
                } else if (iplImg->depth == SR_DEPTH_16U) {
                    CImageUtility::cvtImage16Uto8U(iplImg, bit_depth, iplImageY, SR_DEPTHCVT_ROUND);
                } else {
                    CImageUtility::showErrMsg("Unsupported image data type in CReadWriteYUV::readFrame()!\n");
                    CImageUtility::safeReleaseImage(&iplImg);
                    return false;
                }
            } else {
                CImageUtility::showErrMsg("Only support 8-bit output in CReadWriteYUV::readFrame()!\n");
                CImageUtility::safeReleaseImage(&iplImg);
                return false;
            }
            CImageUtility::setValue(iplImageU, 128);
            CImageUtility::setValue(iplImageV, 128);
            CImageUtility::safeReleaseImage(&iplImg);
            //return true;
        } else {
            //CImageUtility::saveImage("_BGR.bmp", iplImg, 0, 1, bit_depth);
            bool rlt = CImageUtility::cvtBGRtoYUV(iplImg, iplImageY, iplImageU, iplImageV, CHROMA_YUVMS, bit_depth);
            //CImageUtility::saveImage("_Y.bmp", iplImageY);
            //CImageUtility::saveImage("_U.bmp", iplImageY);
            //CImageUtility::saveImage("_V.bmp", iplImageY);
            CImageUtility::safeReleaseImage(&iplImg);
            m_nFormat = IDX_YUVMS444;
            return rlt;
        }
    }

	return true;
}

bool CReadWriteYUV::writeFrame(IplImage *iplImage)
// write one frame and convert color space and format before writting
{
	// check input
	if (iplImage == NULL || iplImage->width != m_nWidth || iplImage->height != m_nHeight) {
			message("Invalid input IPL images in CReadWriteYUV::writeFrame()!\n");
			return false;
	}
	if (m_nYUVFileNum != 1) {
			message("Invalid internal YUV number found in CReadWriteYUV::writeFrame()!\n");
			return false;
	}
	IplImage *iplWrite;
	switch (iplImage->depth) {
		case SR_DEPTH_32F:
			iplWrite = CImageUtility::cvtImage32Fto8U(iplImage, 0);
			break;
		case SR_DEPTH_8U:
			iplWrite = CImageUtility::clone(iplImage);
			break;
		default:
			message("Unsupported image data type found in CReadWriteYUV::writeFrame()!\n");
			return false;
	}
	if (iplWrite == NULL)
		return false;

	// open YUV file
	char stMsg[256];
	FILE *fp;
	int nFileNum = 0;			// only ONE yuv file when writing
	if (m_bFirstWrite) {
		fp = fopen(m_pFileList[nFileNum].stFile, "wb+");
		m_bFirstWrite = false;
	} else {
		fp = fopen(m_pFileList[nFileNum].stFile, "ab");
	}
	if (fp == NULL) {
			sprintf(stMsg, "Fail to open YUV file %s in CReadWriteYUV::writeFrame()!\n", m_pFileList[nFileNum].stFile);
			message(stMsg);
			return false;
	}

	// write raw data
	bool bError = false;
	char *pSrc;
	int nCount, y;
	switch (m_nFormat) {
		case IDX_YUVGrey:
			// write plane
			for (y=0; y<iplWrite->height; y++) {		// should write line by line since IplImage is word-aligned!
				pSrc = iplWrite->imageData + y * iplWrite->widthStep;
				nCount = (int)fwrite(pSrc, sizeof(char), m_nWidth, fp);
				if (nCount != m_nWidth)
					bError = true;
			}
			break;
		case IDX_YUV420P:		// 4:2:0 format in planes
		case IDX_YUV422P:
		case IDX_YUV444P:
        case IDX_YUVMS444:      // converted from RGB 4:4:4 using Microsoft YUV color space (CHROMA_YUVMS)
		default:
			message("Invalid YUV data format in CReadWriteYUV::writeFrame()!\n");
			fclose(fp);
			return false;
	} 
	fclose(fp);

	if (bError) {
		message("Error in writing YUV file in CReadWriteYUV::writeFrame()!\n");
		return false;
	}

	m_nFrameNum ++;

	if (iplWrite != NULL) CImageUtility::safeReleaseImage(&iplWrite);

	return true;
}

bool CReadWriteYUV::writeFrame(IplImage *iplImageY, IplImage *iplImageU, IplImage *iplImageV)
// write one frame in the sequence
// add support on YUV400 (grey scale), by Luhong, 26/01/2016
{
    // check input
    int plane;
    if (m_nFormat == IDX_YUVGrey) {
        if (iplImageY == NULL) {
            message("Invalid input IPL images in CReadWriteYUV::writeFrame()!\n");
            return false;
        }
        plane = 1;
    } else {
	    if (iplImageY == NULL || iplImageU == NULL || iplImageV == NULL || 
		    iplImageY->width != m_nWidth || iplImageY->height != m_nHeight || 
		    iplImageU->width != m_nWidthChroma || iplImageU->height != m_nHeightChroma || 
		    iplImageV->width != m_nWidthChroma || iplImageV->height != m_nHeightChroma) {		
			return false;
	    }
        plane = 3;
    }

	if (m_nYUVFileNum != 1 && m_nFormat != IDX_UYVY422C) {
		message("Invalid internal YUV number found in CReadWriteYUV::writeFrame()!\n");
		return false;
	}

	IplImage *iplWriteY = NULL, *iplWriteU = NULL, *iplWriteV = NULL;
    if (plane == 1) {
	    switch (iplImageY->depth) {
		    case SR_DEPTH_32F:
			    iplWriteY = CImageUtility::cvtImage32Fto8U(iplImageY, 0);
			    break;
		    case SR_DEPTH_8U:
			    iplWriteY = CImageUtility::clone(iplImageY);
			    break;
		    default:
			    message("Unsupported image data type found in CReadWriteYUV::writeFrame()!\n");
			    return false;
	    }
	    if (iplWriteY == NULL) {
            CImageUtility::safeReleaseImage(&iplWriteY);
		    return false;
	    }
    } else {
	    switch (iplImageY->depth) {
		    case SR_DEPTH_32F:
			    iplWriteY = CImageUtility::cvtImage32Fto8U(iplImageY, 0);
                iplWriteU = CImageUtility::cvtImage32Fto8U(iplImageU, 0);
                iplWriteV = CImageUtility::cvtImage32Fto8U(iplImageV, 0);
			    break;
		    case SR_DEPTH_8U:
			    iplWriteY = CImageUtility::clone(iplImageY);
                iplWriteU = CImageUtility::clone(iplImageU);
                iplWriteV = CImageUtility::clone(iplImageV);
			    break;
		    default:
			    message("Unsupported image data type found in CReadWriteYUV::writeFrame()!\n");
			    return false;
	    }
	    if (iplWriteY == NULL || iplWriteU == NULL || iplWriteV == NULL) {
            CImageUtility::safeReleaseImage(&iplWriteY, &iplWriteU, &iplWriteV);
		    return false;
	    }
    }

	// open YUV file
	char stMsg[256];
	FILE *fp = NULL;
	if (m_nFormat == IDX_UYVY422C) {
		if (m_nYUVFileCount >= m_nYUVFileNum) {
			message("File list buffer overflow (file number > 9999) in CReadWriteYUV::writeFrame()!\n");
			return false;
		}
		fp = fopen(m_pFileList[m_nYUVFileCount].stFile, "wb+");
		if (fp == NULL) {
			sprintf(stMsg, "Fail to open YUV file %s in CReadWriteYUV::writeFrame()!\n", m_pFileList[m_nYUVFileCount].stFile);
			message(stMsg);
			return false;
		}
		m_nYUVFileCount ++;
	} else {
		if (m_bFirstWrite) {
			fp = fopen(m_pFileList[0].stFile, "wb+");
			m_bFirstWrite = false;
		} else {
			fp = fopen(m_pFileList[0].stFile, "ab");
		}
		if (fp == NULL) {
			sprintf(stMsg, "Fail to open YUV file %s in CReadWriteYUV::writeFrame()!\n", m_pFileList[0].stFile);
			message(stMsg);
			return false;
		}
		m_nYUVFileCount = 1;
	}

	// write raw data
	bool bError = false;
	char *pSrc, *pSrcU, *pSrcV;
	int nCount, x, y;
	switch (m_nFormat) {
		case IDX_YUV420P:		// 4:2:0 format in planes
        case IDX_YUV422P:		// 4:2:2 format in planes
        case IDX_YUV444P:		// 4:4:4 format in planes
        case IDX_YUVMS444:      // converted from RGB 4:4:4 using Microsoft YUV color space (CHROMA_YUVMS)
			// write Y plane
			for (y=0; y<iplWriteY->height; y++) {		// should write line by line since IplImage is word-aligned!
				pSrc = iplWriteY->imageData + y * iplWriteY->widthStep;
				nCount = (int)fwrite(pSrc, sizeof(char), m_nWidth, fp);
				if (nCount != m_nWidth)
					bError = true;
			}
			// write U plane
			for (y=0; y<iplWriteU->height; y++) {		// should write line by line since IplImage is word-aligned!
				pSrc = iplWriteU->imageData + y * iplWriteU->widthStep;
				nCount = (int)fwrite(pSrc, sizeof(char), m_nWidthChroma, fp);
				if (nCount != m_nWidthChroma)
					bError = true;
			}
			// write V plane
			for (y=0; y<iplWriteV->height; y++) {		// should write line by line since IplImage is word-aligned!
				pSrc = iplWriteV->imageData + y * iplWriteV->widthStep;
				nCount = (int)fwrite(pSrc, sizeof(char), m_nWidthChroma, fp);
				if (nCount != m_nWidthChroma)
					bError = true;
			}
			break;
		case IDX_YUYV422:
			// copy Y plane
			for (y=0; y<iplImageY->height; y++) {		// should copy line by line since IplImage is word-aligned!
				pSrc = iplImageY->imageData + y * iplImageY->widthStep;
				pSrcU = iplImageU->imageData + y * iplImageU->widthStep;
				pSrcV = iplImageV->imageData + y * iplImageV->widthStep;
				for (x=0; x<iplImageY->width; x+=2) {
					// copy one YUYV unit
					fputc(pSrc[x], fp);		// Y
					fputc(pSrcU[x], fp);		// U
					fputc(pSrc[x+1], fp);		// Y
					fputc(pSrcV[x], fp);		// V
				}
			}
			break;
		case IDX_UYVY422:
		case IDX_UYVY422C:
			// copy Y plane
			for (y=0; y<iplImageY->height; y++) {		// should copy line by line since IplImage is word-aligned!
				pSrc = iplImageY->imageData + y * iplImageY->widthStep;
				pSrcU = iplImageU->imageData + y * iplImageU->widthStep;
				pSrcV = iplImageV->imageData + y * iplImageV->widthStep;
				for (x=0; x<iplImageU->width; x++) {
					// copy one YUYV unit
					int xx = x * 2;
					fputc(pSrcU[x], fp);		// U
					fputc(pSrc[xx], fp);		// Y
					fputc(pSrcV[x], fp);		// V
					fputc(pSrc[xx+1], fp);		// Y				
				}
			}
			break;
		case IDX_YUVGrey:
			// write Y plane
			for (y=0; y<iplWriteY->height; y++) {		// should write line by line since IplImage is word-aligned!
				pSrc = iplWriteY->imageData + y * iplWriteY->widthStep;
				nCount = (int)fwrite(pSrc, sizeof(char), m_nWidth, fp);
				if (nCount != m_nWidth)
					bError = true;
			}
            break;
		default:
			message("Invalid YUV data format in CReadWriteYUV::writeFrame()!\n");
			bError = true;
	} 

	fclose(fp);
    CImageUtility::safeReleaseImage(&iplWriteY, &iplWriteU, &iplWriteV);

	if (bError) {
		message("Error in writing YUV file in CReadWriteYUV::writeFrame()!\n");
		return false;
	}

	m_nFrameNum ++;

	return true;
}

void CReadWriteYUV::close(char *stExtraMsg)
// close the YUV data file
// by Luhong Liang, ICD-ASD, ASTRI
// Nov. 6, 2011
{
	if (!m_bOpen) return;

	m_nYUVFileNum = m_nYUVFileCount;

	if (m_cStatus == 'w' || m_cStatus == 'a') {
		writeIndexFile(stExtraMsg);
	}

	if (m_pFileList != NULL) 
		delete m_pFileList;

	m_bOpen = false;
	m_nWidth = 0;				// frame width
	m_nHeight = 0;				// frame height
	m_nFrameNum = 0;			// frame number in this YUV file
	m_nFormat = IDX_YUV_Unknown;	// a string of YUV format, could be "420", TODO: more format support
	m_nFrameSize = 0;			// frame size in BYTE

	m_nYUVFileNum = 0;			// number of YUV raw data files in this index file
	m_nYUVFileCount = 0;
	m_pFileList = NULL;			// each item record one YUV raw data file

	return;
}

bool CReadWriteYUV::writeImage(char stFilePre[], IplImage *iplImageY, IplImage *iplImageU, IplImage *iplImageV)
// Write image to a single YUV file
// This function only supports IDX_YUVGrey, IDX_YUV420P, IDX_YUV422P and IDX_YUV444P format.
// NOTE: This function is independent to other part of this class!
{
    // check input
    if (stFilePre == NULL || iplImageY == NULL) {
        message("Invalid input file name or image in CReadWriteYUV::writeImage()!\n");
        return false;
    }

    // check format, set filename, and convert data format
    char stFilename[256];
    char stMsg[256];
    YUVFormat format = IDX_YUV_Unknown;
    IplImage *iplWriteY = NULL, *iplWriteU = NULL, *iplWriteV = NULL;
    if (iplImageU != NULL && iplImageV != NULL) {
        // check format & set filename
        if (iplImageU->width == iplImageY->width && iplImageV->width == iplImageY->width &&
            iplImageU->height == iplImageY->height && iplImageV->height == iplImageY->height) {
            sprintf(stFilename, "%s_%dx%d(444P).yuv", stFilePre, iplImageY->width, iplImageY->height);
            format = IDX_YUV444P;
        } else if (iplImageU->width*2 == iplImageY->width && iplImageV->width*2 == iplImageY->width &&
                   iplImageU->height == iplImageY->height && iplImageV->height == iplImageY->height) {
            sprintf(stFilename, "%s_%dx%d(422P).yuv", stFilePre, iplImageY->width, iplImageY->height);
            format = IDX_YUV422P;
        } else if (iplImageU->width*2 == iplImageY->width && iplImageV->width*2 == iplImageY->width &&
                   iplImageU->height*2 == iplImageY->height && iplImageV->height*2 == iplImageY->height) {
            sprintf(stFilename, "%s_%dx%d(420P).yuv", stFilePre, iplImageY->width, iplImageY->height);
            format = IDX_YUV420P;
        } else {
            message("Warning: unsupported YUV format found in CReadWriteYUV::writeImage()!\n");
            return false;
        }
        // convert data format
	    switch (iplImageY->depth) {
		    case SR_DEPTH_32F:
			    iplWriteY = CImageUtility::cvtImage32Fto8U(iplImageY, 0);
                iplWriteU = CImageUtility::cvtImage32Fto8U(iplImageU, 0);
                iplWriteV = CImageUtility::cvtImage32Fto8U(iplImageV, 0);
			    break;
		    case SR_DEPTH_8U:
			    iplWriteY = CImageUtility::clone(iplImageY);
                iplWriteU = CImageUtility::clone(iplImageU);
                iplWriteV = CImageUtility::clone(iplImageV);
			    break;
		    default:
			    message("Unsupported image data type found in CReadWriteYUV::writeImage()!\n");
			    return false;
	    }
	    if (iplWriteY == NULL || iplWriteU == NULL || iplWriteV == NULL) {
            CImageUtility::safeReleaseImage(&iplWriteY, &iplWriteU, &iplWriteV);
		    return false;
	    }
    } else {
        // check format & set filename
        sprintf(stFilename, "%s_%dx%d(400).yuv", stFilePre, iplImageY->width, iplImageY->height);
        format = IDX_YUVGrey;
        // convert data format
	    switch (iplImageY->depth) {
		    case SR_DEPTH_32F:
			    iplWriteY = CImageUtility::cvtImage32Fto8U(iplImageY, 0);
			    break;
		    case SR_DEPTH_8U:
			    iplWriteY = CImageUtility::clone(iplImageY);
			    break;
		    default:
			    message("Unsupported image data type found in CReadWriteYUV::writeImage()!\n");
			    return false;
	    }
	    if (iplWriteY == NULL) {
            CImageUtility::safeReleaseImage(&iplWriteY);
		    return false;
	    }
    }

    // open YUV file
    FILE *fp = fopen(stFilename, "wb+");
    if (fp == NULL) {
        sprintf(stMsg, "Fail to open YUV file %s in CReadWriteYUV::writeImage()!\n", stFilename);
        message(stMsg);
        return false;
    }

    // write image data
	bool bError = false;
	char *pSrc, *pSrcU, *pSrcV;
	int nCount, x, y;
	switch (format) {
		case IDX_YUV420P:		// 4:2:0 format in planes
        case IDX_YUV422P:		// 4:2:2 format in planes
        case IDX_YUV444P:		// 4:4:4 format in planes
        case IDX_YUVMS444:      // converted from RGB 4:4:4 using Microsoft YUV color space (CHROMA_YUVMS)
			// write Y plane
			for (y=0; y<iplWriteY->height; y++) {		// should write line by line since IplImage is word-aligned!
				pSrc = iplWriteY->imageData + y * iplWriteY->widthStep;
                nCount = (int)fwrite(pSrc, sizeof(char), iplWriteY->width, fp);
				if (nCount != iplWriteY->width)
					bError = true;
			}
			// write U plane
			for (y=0; y<iplWriteU->height; y++) {		// should write line by line since IplImage is word-aligned!
				pSrc = iplWriteU->imageData + y * iplWriteU->widthStep;
				nCount = (int)fwrite(pSrc, sizeof(char), iplWriteU->width, fp);
				if (nCount != iplWriteU->width)
					bError = true;
			}
			// write V plane
			for (y=0; y<iplWriteV->height; y++) {		// should write line by line since IplImage is word-aligned!
				pSrc = iplWriteV->imageData + y * iplWriteV->widthStep;
				nCount = (int)fwrite(pSrc, sizeof(char), iplWriteV->width, fp);
				if (nCount != iplWriteV->width)
					bError = true;
			}
			break;
		case IDX_YUYV422:
			// copy Y plane
			for (y=0; y<iplImageY->height; y++) {		// should copy line by line since IplImage is word-aligned!
				pSrc = iplImageY->imageData + y * iplImageY->widthStep;
				pSrcU = iplImageU->imageData + y * iplImageU->widthStep;
				pSrcV = iplImageV->imageData + y * iplImageV->widthStep;
				for (x=0; x<iplImageY->width; x+=2) {
					// copy one YUYV unit
					fputc(pSrc[x], fp);		    // Y
					fputc(pSrcU[x], fp);		// U
					fputc(pSrc[x+1], fp);		// Y
					fputc(pSrcV[x], fp);		// V
				}
			}
			break;
		case IDX_UYVY422:
		case IDX_UYVY422C:
			// copy Y plane
			for (y=0; y<iplImageY->height; y++) {		// should copy line by line since IplImage is word-aligned!
				pSrc = iplImageY->imageData + y * iplImageY->widthStep;
				pSrcU = iplImageU->imageData + y * iplImageU->widthStep;
				pSrcV = iplImageV->imageData + y * iplImageV->widthStep;
				for (x=0; x<iplImageU->width; x++) {
					// copy one YUYV unit
					int xx = x * 2;
					fputc(pSrcU[x], fp);		// U
					fputc(pSrc[xx], fp);		// Y
					fputc(pSrcV[x], fp);		// V
					fputc(pSrc[xx+1], fp);		// Y				
				}
			}
			break;
		case IDX_YUVGrey:
			// write Y plane
			for (y=0; y<iplWriteY->height; y++) {		// should write line by line since IplImage is word-aligned!
				pSrc = iplWriteY->imageData + y * iplWriteY->widthStep;
				nCount = (int)fwrite(pSrc, sizeof(char), iplWriteY->width, fp);
				if (nCount != iplWriteY->width)
					bError = true;
			}
            break;
		default:
			message("Invalid YUV data format in CReadWriteYUV::writeImage()!\n");
			bError = true;
	} 

    // close file
	fclose(fp);
    CImageUtility::safeReleaseImage(&iplWriteY, &iplWriteU, &iplWriteV);

	if (bError) {
		message("Error in writing YUV file in CReadWriteYUV::writeImage()!\n");
		return false;
	}

	return true;;
}