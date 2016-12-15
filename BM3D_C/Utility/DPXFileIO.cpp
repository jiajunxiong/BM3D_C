// DPX file I/O functions modified by Luhong Liang, ICD-ASD, ASTRI
// Based on the open source file CDPXFileIO.h and CDPXFileIO.cpp

/** 
\file CDPXFileIO.cpp
Created by Javier Hernanz Zajara on 12/01/07.

Version 0.1.3

Luz3D Tools

Copyright (C) 2007  Javier Hernanz Zajara

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "stdafx.h"

#include "DPXFileIO.h"
#include "ImgProcUtility.h"

CDPXFileIO::CDPXFileIO(bool show_msg)
{
	inverted=false;					// Just an initial suposition
	fullImage=NULL;					// There's no picture still... No space reserved
	fullImageSize=0;				// No reserved space
	userData.data=NULL;				// There's no user data still... No space reserved

    show_message = show_msg;
}

CDPXFileIO::~CDPXFileIO()
{
	freeUserData();
    freeImage();        // by Luhong
}

bool CDPXFileIO::getInverted()
{
	return inverted;
}

void CDPXFileIO::setInverted(bool newInverted)
{
	inverted=newInverted;
}

U16 CDPXFileIO::invert16 (U16 readValue)
{
	U8 *input=(U8 *)&readValue;
	U16 defTemp=input[1] | (input[0]<<8);
	
	return defTemp;
}

U32 CDPXFileIO::invert32 (U32 readValue)
{
	U8 *input=(U8 *)&readValue;
	U32 defTemp=input[3] | (input[2]<<8) | (input[1]<<16) | (input[0]<<24);
	
	return defTemp;
}

/** invertR32 is intended to convert Real/Float 32 bit numbers between Little and Big endian.
*
*<b>TO DO:</b><ul><li>TEST IT in depth!!!!!!!</li></ul>*/
R32 CDPXFileIO::invertR32 (R32 readValue)
{
	U8 *input=(U8 *)&readValue;
	R32 defTemp = (R32)(input[3] | (input[2]<<8) | (input[1]<<16) | (input[0]<<24));
	
	return defTemp;
}

int CDPXFileIO::convertMainHeaders(void)
{
	int counter;
	
	if (!inverted) return DPX_OK;
	
	// Well, if here it's inverted. Let's convert values
	
	infoFile.magicNumber=invert32(infoFile.magicNumber);
	infoFile.offset=invert32(infoFile.offset);
	infoFile.fileSize=invert32(infoFile.fileSize);
	infoFile.dittoKey=invert32(infoFile.dittoKey);
	infoFile.genericHdrSize=invert32(infoFile.genericHdrSize);
	infoFile.industryHdrSize=invert32(infoFile.industryHdrSize);
	infoFile.userDataSize=invert32(infoFile.userDataSize);
	infoFile.encryptionKey=invert32(infoFile.encryptionKey);
	
	infoImage.imageOrientation=invert16(infoImage.imageOrientation);
	infoImage.imageElements=invert16(infoImage.imageElements);
	infoImage.x=invert32(infoImage.x);
	infoImage.y=invert32(infoImage.y);
	
	for (counter=0; counter<8; counter++)
	{
		infoImage.imageElement[counter].dataSign=
			invert32(infoImage.imageElement[counter].dataSign);
		infoImage.imageElement[counter].refLowData=
			invert32(infoImage.imageElement[counter].refLowData);
		infoImage.imageElement[counter].refLowQuantity=
			invertR32(infoImage.imageElement[counter].refLowQuantity);
		infoImage.imageElement[counter].refHighData=
			invert32(infoImage.imageElement[counter].refHighData);
		infoImage.imageElement[counter].refHighQuantity=
			invertR32(infoImage.imageElement[counter].refHighQuantity);
		infoImage.imageElement[counter].packing=
			invert16(infoImage.imageElement[counter].packing);
		infoImage.imageElement[counter].encoding=
			invert16(infoImage.imageElement[counter].encoding);
		infoImage.imageElement[counter].dataOffset=
			invert32(infoImage.imageElement[counter].dataOffset);
		infoImage.imageElement[counter].eolPadding=
			invert32(infoImage.imageElement[counter].eolPadding);
		infoImage.imageElement[counter].eoiPadding=
			invert32(infoImage.imageElement[counter].eoiPadding);
	}
	
	
	infoOrientation.xOffset=invert32(infoOrientation.xOffset);
	infoOrientation.yOffset=invert32(infoOrientation.yOffset);
	infoOrientation.xCenter=invertR32(infoOrientation.xCenter);
	infoOrientation.yCenter=invertR32(infoOrientation.yCenter);
	infoOrientation.xOriginalSize=invert32(infoOrientation.xOriginalSize);
	infoOrientation.yOriginalSize=invert32(infoOrientation.yOriginalSize);
	infoOrientation.border[0]=invert16(infoOrientation.border[0]);
	infoOrientation.border[1]=invert16(infoOrientation.border[1]);
	infoOrientation.border[2]=invert16(infoOrientation.border[2]);
	infoOrientation.border[3]=invert16(infoOrientation.border[3]);
	infoOrientation.pixelAspect[0]=invert32(infoOrientation.pixelAspect[0]);
	infoOrientation.pixelAspect[1]=invert32(infoOrientation.pixelAspect[1]);
	
	return(0);
}

void CDPXFileIO::convertFilmHeader (void)
{
	if (!inverted) return;
	
	infoFilm.framePosition=invert32(infoFilm.framePosition);
	infoFilm.sequenceLength=invert32(infoFilm.sequenceLength);
	infoFilm.heldCount=invert32(infoFilm.heldCount);
	infoFilm.frameRate=invertR32(infoFilm.frameRate);
	infoFilm.shutterAngle=invertR32(infoFilm.shutterAngle);
	return;
}

void CDPXFileIO::convertTVHeader(void)
{
	if (!inverted) return;
	
	infoTV.timeCode=invert32(infoTV.timeCode);
	infoTV.userBits=invert32(infoTV.userBits);
	infoTV.horizontalSampleRate=invertR32(infoTV.horizontalSampleRate);
	infoTV.verticalSampleRate=invertR32(infoTV.verticalSampleRate);
	infoTV.frameRate=invertR32(infoTV.frameRate);
	infoTV.timeOffset=invertR32(infoTV.timeOffset);
	infoTV.gamma=invertR32(infoTV.gamma);
	infoTV.blackLevel=invertR32(infoTV.blackLevel);
	infoTV.blackGain=invertR32(infoTV.blackGain);
	infoTV.breakPoint=invertR32(infoTV.breakPoint);
	infoTV.whiteLevel=invertR32(infoTV.whiteLevel);
	infoTV.integrationTimes=invertR32(infoTV.integrationTimes);
	return;
}

void CDPXFileIO::freeUserData (void)
{
	//if (userData.data) free(userData.data);
    if (userData.data != NULL) 
        delete [] userData.data;
    userData.data = NULL;
}

int CDPXFileIO::allocUserData(void)
{
	freeUserData();
	// There's a reserved field of 32 bytes
	if (infoFile.userDataSize > 32) {
		//userData.data=(U8*) malloc(infoFile.userDataSize-32);
        userData.data = new U8[infoFile.userDataSize-32];
		if (!userData.data) return (ERR_DPX_MEMORY);
		return (DPX_OK);
	}
	return (DPX_OK);
}

void CDPXFileIO::freeImage (void)
{
	if (fullImage != NULL) 
        delete [] fullImage;

    fullImage = NULL;
	fullImageSize = 0;
}

int CDPXFileIO::allocImage (void)
{	
	freeImage();

	fullImageSize = infoFile.fileSize-infoFile.offset;
	fullImage = new U32[fullImageSize];
	if (!fullImage) 
        return (ERR_DPX_MEMORY);

	return (DPX_OK);
}

int CDPXFileIO::loadFromFile (char *fileName)
{
    if (fileName == NULL || fileName[0] == 0) {
        CImageUtility::showErrMsg("Invalid filename in CDPXFileIO::loadFromFile()!\n");
        return (ERR_DPX_CANTOPEN);
    }
    
    if (show_message) {     // switch by Luhong, June 9, 2014
	    CImageUtility::showMessage1("Load DPX file...\n");
    }

	FILE * dpxFile = NULL;      // by Luhong
	int temp;
	U32 tempCounter;
	
	strcpy(openFile,fileName);
	
	dpxFile = fopen(fileName,"rb");
	if (!dpxFile) {
        return ERR_DPX_CANTOPEN;
    }
    //fseek(dpxFile, 0, SEEK_END);
    //long file_size = ftell(dpxFile);
    //printf("File size: %d\n", file_size);
    //fseek(dpxFile, 0, SEEK_SET);
	
	fread(&infoFile,sizeof(infoFile),1,dpxFile);
	fread(&infoImage,sizeof(dpxImageInformation),1,dpxFile);
	fread(&infoOrientation,sizeof(dpxImageOrientation),1,dpxFile);

    //infoImage.x = invert(infoImage.x);

	// Next headers are optional so let's convert all values to be sure what we need
	if (infoFile.magicNumber==0x58504453)  {
        inverted=true; 
    } else if (infoFile.magicNumber==0x53445058) {
        inverted=false;
    } else {
        fclose(dpxFile);        // fixed by Luhong, June 8, 2015
        return (ERR_DPX_NOTDPX);
    }
	
	temp=convertMainHeaders();
	if (temp<0) {
        fclose(dpxFile);        // fixed by Luhong, June 8, 2015
        return (temp); // There was any error
    }
	
	// Is there any other header to load?
	
	if (infoFile.industryHdrSize>0) {  // There's at least 1 industry header
		switch (infoFile.industryHdrSize) {
		case sizeof(dpxMotionPicture):
			fread(&infoFilm,sizeof(dpxMotionPicture),1,dpxFile);
			convertFilmHeader();
			break;
			
		case sizeof(dpxTelevision):
			fread(&infoTV,sizeof(dpxTelevision),1,dpxFile);
			convertTVHeader();
			break;
			
		default:  // case (sizeof(dpxMotionPicture)+sizeof(dpxTelevision)) && UNSPECIFIED IN HEADER
			fread(&infoFilm,sizeof(dpxMotionPicture),1,dpxFile);
			fread(&infoTV,sizeof(dpxTelevision),1,dpxFile);
			convertFilmHeader();
			convertTVHeader();
			break;
		}
	}

	// Is there User Data to load?
	if (infoFile.userDataSize>0) {
		if (allocUserData()<0) {
			fclose(dpxFile);
			return ERR_DPX_MEMORY;
		}
		//int temp1 = fread(userData.userId, (size_t)1, (size_t)32, dpxFile); // If file header is corrupt may cause loading here image parts
        //cur_pos = ftell(dpxFile);
        //printf("%d bytes read, pos=%d\n", temp1, cur_pos);
		//int temp2 = fread(userData.data,(size_t)1,(size_t)(infoFile.userDataSize-32),dpxFile);
        //cur_pos = ftell(dpxFile);
        //printf("%d bytes read, pos=%d\n", temp2, cur_pos);
	}
	
	// If infoFile.fileSize = 0 then solve problems before they appear
	
	if(infoFile.fileSize < infoFile.offset) {
		infoFile.fileSize=infoFile.offset;
		// ******* CHECK HERE WHEN PACKING=0 *****
		if (infoImage.imageElement[0].packing==1 && infoImage.imageElement[0].bitDepth==10)
			infoFile.fileSize+=(infoImage.imageElements*((infoImage.x*infoImage.y)*sizeof(U32)));
		else if (infoImage.imageElement[0].packing==0 && infoImage.imageElement[0].bitDepth==8){
			// Must be 32 bit divisible
			U32 tempSize;
			bool padded;
			
			padded=false;
			tempSize=((infoImage.x*infoImage.y)*3)*infoImage.imageElements;
			
			while (!padded)
				if (tempSize%4==0) padded=true;
				else tempSize++;
			
			infoFile.fileSize+=tempSize;
		}
	}
		
	// Well, we are ready to load the full picture
	if (allocImage()<0) {
		fclose(dpxFile);
		return ERR_DPX_MEMORY;
	}

	tempCounter=0;
			
    //while (!feof(dpxFile)){
	//	dpxPixel tempPixel;
	
    //    cur_pos = ftell(dpxFile);
    //    int read_bytes = fread(&tempPixel, 1, sizeof(dpxPixel), dpxFile);
	//	if(read_bytes!=0) // Check for eventual EOF
	//	{
			/////////////// THIS IS BAD FOR READING MULTIPLE IMAGE ELEMENTS ///////////////////////
	//		if (fullImageSize<(tempCounter*4)) { // We are reading 32 bits each time
	//			fclose(dpxFile);
	//			return (ERR_DPX_FILE_OVERFLOW);
	//		}
			
	//		if (inverted && (infoImage.imageElement[0].packing==1)) 
    //            fullImage[tempCounter]=invert32(tempPixel);
	//		else 
    //            fullImage[tempCounter]=tempPixel;
	//		tempCounter++;
	//	}
	//}

    U32 read_bytes = fread(fullImage, (size_t)1, (size_t)fullImageSize, dpxFile);
	//printf("Kayton: read_bytes: %d\n", read_bytes);
	//if (read_bytes < fullImageSize) {
	//	fclose(dpxFile);
	//	return (ERR_DPX_UNEXPECTED_EOF);
	//}
	
	//if (fclose(dpxFile)) 
    //    return (ERR_DPX_CANTCLOSE);

    // check loaded image
    int color_num = 3;
    int bit_depth = infoImage.imageElement[0].bitDepth;
	//printf("Kayton: bit_depth: %d\n", bit_depth);
    int pix_bytes;
    if (bit_depth <= 8) {
        pix_bytes = 1;					// NOTE: UNCLEAR foramt for 8-bit data!!!
    } else if (bit_depth == 10) {		//Kayton code added
        pix_bytes = 4;
    }else if (bit_depth <= 16) {
        pix_bytes = 2;
    } else {
        fclose(dpxFile);        // fixed by Luhong, June 8, 2015
        return ERR_DPX_UNSUPPORTED_FORMAT;
    }

	//Kayton code added for debug
    if (show_message) {     // switch by Luhong, June 9, 2014
	    CImageUtility::showMessage1("magicNumber: %d\n", infoFile.magicNumber);
	    CImageUtility::showMessage1("infoImage.x: %d\n", infoImage.x);
	    CImageUtility::showMessage1("infoImage.x: %d\n", infoImage.x);
	    CImageUtility::showMessage1("infoImage.y: %d\n", infoImage.y);
	    CImageUtility::showMessage1("color_num: %d\n", color_num);
	    CImageUtility::showMessage1("pix_bytes: %d\n", pix_bytes);
	    CImageUtility::showMessage1("infoImage.x * infoImage.y * color_num * pix_bytes: %d\n", infoImage.x * infoImage.y * color_num * pix_bytes);
    }
	
	//Original code
	/*if (read_bytes < infoImage.x * infoImage.y * color_num * pix_bytes) {
		return ERR_DPX_UNSUPPORTED_FORMAT;
	}*/
	
	//Kayton code added
	if (bit_depth == 8 || bit_depth == 16){
		if (read_bytes < infoImage.x * infoImage.y * color_num * pix_bytes) {
            fclose(dpxFile);        // fixed by Luhong, June 8, 2015
			return ERR_DPX_UNSUPPORTED_FORMAT;
		}
	}else if (bit_depth == 10){
		if (read_bytes < infoImage.x * infoImage.y * color_num * pix_bytes /3) {
            fclose(dpxFile);        // fixed by Luhong, June 8, 2015
			return ERR_DPX_UNSUPPORTED_FORMAT;
		}
	}

	//Kayton code added
	// invert image in need for 10-bit
    if (inverted && pix_bytes == 4) {
        for (U32 y=0; y<infoImage.y; y++) {
            U32 *pLine = (U32 *)fullImage + y * infoImage.x * color_num;
            for (U32 x=0; x<infoImage.x*color_num; x++) {
                pLine[x] = invert32(pLine[x]);
            }
        }
    }

    // invert image in need for 16-bit
    if (inverted && pix_bytes == 2) {
        for (U32 y=0; y<infoImage.y; y++) {
            U16 *pLine = (U16 *)fullImage + y * infoImage.x * color_num;
            for (U32 x=0; x<infoImage.x*color_num; x++) {
                pLine[x] = invert16(pLine[x]);
            }
        }
    }

	if (fclose(dpxFile)) {      // fixed by Luhong, June 8, 2015
        return (ERR_DPX_CANTCLOSE);
    }

	return (DPX_OK);
}

int CDPXFileIO::updateImage(void *pImg, int width, int height, int channels, int bit_depth, char szFilename[])
// update the image in the internal buffers
{
    if (pImg == NULL || width < 4 || height < 4 || channels != 3) {
        CImageUtility::showErrMsg("Invalid input argument in updateImage()!\n");
        return ERR_DPX_UNSUPPORTED_FORMAT;
    }
    if (bit_depth < 1 || bit_depth > 16) {
        CImageUtility::showErrMsg("Only support image with bit depth 1~16 in updateImage()!\n");
        return ERR_DPX_UNSUPPORTED_FORMAT;
    }

    // clean existing image
    freeImage();
    freeUserData();

    // set header
    inverted = true;
    const int user_data_size = 6112;

    setDftFileInfo(szFilename, width, height, bit_depth, "LLH, ICD-ASD, ASTRI", user_data_size);
    setDftImageInfo(width, height, bit_depth, user_data_size);
    if (!setDftUserData(user_data_size)) {     // user data allocated here!
        CImageUtility::showErrMsg("Fail to allocate user data in saveDPXImage()!\n");
        return ERR_DPX_MEMORY;
    }
  
    setDftImageOri(width, height);
    setDftFilmInfo();
    setDftTVInfo();

    // allocate image
    if (allocImage() != DPX_OK) {
        freeUserData();
        CImageUtility::showErrMsg("Fail to allocate image buffer in saveDPXImage()!\n");
        return ERR_DPX_MEMORY;
    }

    // copy image data (BGR-->RGB)
    if (bit_depth > 8) {
        // 16U
        for (int y=0; y<height; y++) {
            unsigned short *pSrc = (unsigned short *)pImg + y * 3 * width;
            unsigned short *pDst = (unsigned short *)fullImage + y * 3 * infoImage.x;
            for (int x=0; x<width*3; x+=3) {
                pDst[x] = pSrc[x+2];
                pDst[x+1] = pSrc[x+1];
                pDst[x+2] = pSrc[x];
            }
        }
    } else {
        // 8U
        for (int y=0; y<height; y++) {
            unsigned char *pSrc = (unsigned char *)((char *)pImg + y * 3 * width);
            unsigned char *pDst = (unsigned char *)fullImage + y * 3 * infoImage.x;
            for (int x=0; x<width*3; x+=3) {
                pDst[x] = pSrc[x+2];
                pDst[x+1] = pSrc[x+1];
                pDst[x+2] = pSrc[x];
            }
        }
    }

    return DPX_OK;
}

int CDPXFileIO::saveToFile (char *fileName)
{
    if (fileName == NULL || fileName[0] == 0) {
        CImageUtility::showErrMsg("Invalid filename in CDPXFileIO::loadFromFile()!\n");
        return (ERR_DPX_CANTOPEN);
    }
    
    if (show_message) {     // switch by Luhong, June 9, 2014
	    CImageUtility::showMessage1("Save DPX file...\n");
    }

	// Let's write as is in our structures...
	// TODO: Improve options and performance
	FILE *dpxFile;
	U32 imageSize;
	//unsigned int counter;
	
	dpxFile = fopen(fileName,"wb+");        // by Luhong
	if (!dpxFile) {
        return ERR_DPX_CANTOPEN;
    }
	
	imageSize=infoFile.fileSize-infoFile.genericHdrSize-infoFile.industryHdrSize-infoFile.userDataSize;
	
	convertMainHeaders();
	fwrite(&infoFile,sizeof(dpxFileInformation),1,dpxFile);
	fwrite(&infoImage,sizeof(dpxImageInformation),1,dpxFile);
	fwrite(&infoOrientation,sizeof(dpxImageOrientation),1,dpxFile);
	convertMainHeaders();
	
	if (infoFile.industryHdrSize>0) {
		switch (infoFile.industryHdrSize) {
			
			case sizeof(dpxMotionPicture):
				convertFilmHeader();
				fwrite(&infoFilm,sizeof(dpxMotionPicture),1,dpxFile);
				convertFilmHeader();
				break;
				
			case sizeof(dpxTelevision):
				convertTVHeader();
				fwrite(&infoTV,sizeof(dpxTelevision),1,dpxFile);
				convertTVHeader();
				break;
				
			default:  // case (sizeof(dpxMotionPicture)+sizeof(dpxTelevision)) && UNSPECIFIED IN HEADER
				convertFilmHeader();
				convertTVHeader();
				fwrite(&infoFilm,sizeof(dpxMotionPicture),1,dpxFile);
				fwrite(&infoTV,sizeof(dpxTelevision),1,dpxFile);
				convertFilmHeader();
				convertTVHeader();
				break;
		}
	}
	
	if (infoFile.userDataSize>0) {
		fwrite(userData.userId, 32, 1, dpxFile);
		fwrite(userData.data,infoFile.userDataSize-32,1,dpxFile);
	}

    // invert image in need and save
    const int color_num = 3;
    int pix_bytes;
    if (infoImage.imageElement[0].bitDepth <= 8) {
        pix_bytes = 1;          // NOTE: UNCLEAR foramt for 8-bit data!!!
    } else if (infoImage.imageElement[0].bitDepth == 10) {		//Kayton Code added for 10-bit DPX encoding
        pix_bytes = 4;
	} else if (infoImage.imageElement[0].bitDepth <= 16) {
		pix_bytes = 2;
    } else {
        fclose(dpxFile);        // fixed by Luhong, June 8, 2015
        return ERR_DPX_UNSUPPORTED_FORMAT;
    }

	if (infoImage.imageElement[0].bitDepth == 8 || infoImage.imageElement[0].bitDepth == 16){		//Kayton added
		U16 *pDstBuf = new U16[infoImage.x*color_num*pix_bytes];
		if (pDstBuf == NULL) {
			fclose(dpxFile);
			return (ERR_DPX_MEMORY);
		}
		if (inverted && pix_bytes == 2) {
			for (U32 y=0; y<infoImage.y; y++) {
				U16 *pLine = (U16 *)fullImage + y * infoImage.x * color_num;
				for (U32 x=0; x<infoImage.x*color_num; x++) {
					if (pLine[x] != 0)
					pDstBuf[x] = invert16(pLine[x]);
				}
				fwrite((void*)pDstBuf, 1, infoImage.x*color_num*pix_bytes, dpxFile);
			}
		} else {
			fwrite((void*)fullImage, 1, infoImage.x*infoImage.y*color_num*pix_bytes, dpxFile);
		}
	}

	//Kayton Code added for 10-bit DPX encoding
	else if (infoImage.imageElement[0].bitDepth == 10){
		U32 *pDstBuf_32U = new U32[infoImage.x];
		if (pDstBuf_32U == NULL) {
			fclose(dpxFile);
			return (ERR_DPX_MEMORY);
		}

		if (inverted && pix_bytes == 4) {
			for (U32 y=0; y<infoImage.y; y++) {
				U16 *pLine_16U = (U16 *)fullImage + y * infoImage.x * color_num;
				for (U32 x=0; x<infoImage.x; x++) {

					unsigned int R_32U = pLine_16U[x*3]   << 22;
					unsigned int G_32U = pLine_16U[x*3+1] << 12;
					unsigned int B_32U = pLine_16U[x*3+2] << 2;
					pDstBuf_32U[x] = R_32U + G_32U + B_32U;

					pDstBuf_32U[x] = invert32(pDstBuf_32U[x]);
				}
				fwrite((void*)pDstBuf_32U, 1, infoImage.x*pix_bytes, dpxFile);
			}
		}
	}

	//for (counter=0; (4*counter)<fullImageSize; counter++){
	//	dpxPixel temp;
	//	temp=fullImage[counter];
	//	if (inverted  && (infoImage.imageElement[0].packing==1)) 
    //        temp=invert32(temp);
	//	fwrite(&temp,sizeof(dpxPixel),1,dpxFile);
	//}
	
	if (fclose(dpxFile)) {
        return (ERR_DPX_CANTCLOSE);
    }
	
	return (DPX_OK);
}

IplImage *CDPXFileIO::loadDPXImage(char *fileName)
// loade DPX image to BGR OpenCV image
// by Luhong
{
    // load image from file 
    switch(loadFromFile(fileName)) {
        case DPX_OK:
            break;
        case ERR_DPX_CANTOPEN:
            CImageUtility::showErrMsg("Fail to open image file %s in loadDPXImage()!\n", fileName);
            return NULL;
        case ERR_DPX_CANTCLOSE:
            CImageUtility::showErrMsg("Fail to close image file %s in loadDPXImage()!\n", fileName);
            return NULL;
        case ERR_DPX_NOTDPX:
            CImageUtility::showErrMsg("Invalid DPX header in %s in loadDPXImage()!\n", fileName);
            return NULL;
        case ERR_DPX_MEMORY:
            CImageUtility::showErrMsg("Fail to allocate buffer in file %s in loadDPXImage()!\n", fileName);
            return NULL;
        case ERR_DPX_FILE_OVERFLOW:
            CImageUtility::showErrMsg("Data overflow in file %s in loadDPXImage()!\n", fileName);
            return NULL;
        case ERR_DPX_UNEXPECTED_EOF:
            CImageUtility::showErrMsg("Unexpected EOF in file %s in loadDPXImage()!\n", fileName);
            return NULL;
        case ERR_DPX_UNSUPPORTED_FORMAT:
            CImageUtility::showErrMsg("Unsupported format in file %s in loadDPXImage()!\n", fileName);
            return NULL;
        default:
            CImageUtility::showErrMsg("Unknow error in file %s in loadDPXImage()!\n", fileName);
            return false;
    }

    // check image loaded
    int width = getWidth();
    int height = getHeight();
    int bit_depth = getBitDepth();
    void *pImage = getImageBuf();
    if (pImage == NULL) {
        CImageUtility::showErrMsg("Fail to load DPX file %s\n", fileName);
        return NULL;
    }

    // allocate image buffer
    IplImage *iplImage = NULL;
    if (bit_depth <= 8) {       // NOTE: UNCLEAR foramt for 8-bit DPX file!!!
        iplImage = CImageUtility::createImage(width, height, SR_DEPTH_8U, 3);
        if (iplImage == NULL) {
            CImageUtility::showErrMsg("Fail to allocate buffer in loadDPXImage()!\n");
            return false;
        }
        for (int y=0; y<iplImage->height; y++) {
            unsigned char *pSrcBuf = (unsigned char*)pImage + y * width * 3;        // alignment problem?
            unsigned char *pDstBuf = (unsigned char*)(iplImage->imageData + y * iplImage->widthStep);
            for (int x=0; x<iplImage->width*3; x+=3) {
                pDstBuf[x] = pSrcBuf[x+2];
                pDstBuf[x+1] = pSrcBuf[x+1];
                pDstBuf[x+2] = pSrcBuf[x];
            }
        }

	//Kayton coded for 10-bit data unpacking
    } else if (bit_depth == 10) {
		iplImage = CImageUtility::createImage(width, height, SR_DEPTH_16U, 3);
        if (iplImage == NULL) {
            CImageUtility::showErrMsg("Fail to allocate buffer in loadDPXImage()!\n");
            return false;
        }
		for (int y=0; y<iplImage->height; y++) {
            U32 *pSrcBuf = (U32 *)pImage + y * width;
            U16 *pDstBuf = (U16 *)(iplImage->imageData + y * iplImage->widthStep);
            for (int x=0; x<iplImage->width*3; x+=3) {
				pDstBuf[x] = (U16)((pSrcBuf[x/3] & 0xffc) >> 2);			//Blue
				pDstBuf[x+1] = (U16)((pSrcBuf[x/3] & 0x3ff000) >> 12);		//Green
				pDstBuf[x+2]   = (U16)((pSrcBuf[x/3] & 0xffc00000) >> 22);	//Red
            }
        }
	}
	else {
        iplImage = CImageUtility::createImage(width, height, SR_DEPTH_16U, 3);
        if (iplImage == NULL) {
            CImageUtility::showErrMsg("Fail to allocate buffer in loadDPXImage()!\n");
            return false;
        }
        for (int y=0; y<iplImage->height; y++) {
            U16 *pSrcBuf = (U16 *)pImage + y * width * 3;        // alignment problem?
            U16 *pDstBuf = (U16 *)(iplImage->imageData + y * iplImage->widthStep);
            for (int x=0; x<iplImage->width*3; x+=3) {
                pDstBuf[x] = pSrcBuf[x+2];
                pDstBuf[x+1] = pSrcBuf[x+1];
                pDstBuf[x+2] = pSrcBuf[x];
            }
        }
    }

    return iplImage;
}

bool CDPXFileIO::saveDPXImage(char *fileName, IplImage *iplImage, int bit_depth)
// iplImage -- must be a 8U or 16U data
{
    if (fileName == NULL || fileName[0] == 0 || iplImage == NULL || iplImage->nChannels != 3) {
        CImageUtility::showErrMsg("Invalid input argument in saveDPXImage()!\n");
        return false;
    }
    if (iplImage->depth == SR_DEPTH_16U) {
        if (bit_depth < 1 || bit_depth > 16) {
            CImageUtility::showErrMsg("Only support 16U image with bit depth 1~16 in saveDPXImage()!\n");
            return false;
        }
    } else if (iplImage->depth == SR_DEPTH_8U) {
        if (bit_depth < 1 || bit_depth > 8) {
            CImageUtility::showErrMsg("Only support 8U image with bit depth 1~8 in saveDPXImage()!\n");
            return false;
        }
    } else {
        CImageUtility::showErrMsg("Only support 8U or 16U image in saveDPXImage()!\n");
        return false;
    }
    
    int rlt = updateImage((void *)(iplImage->imageData), iplImage->width, iplImage->height, iplImage->nChannels, bit_depth, fileName);
    if (rlt != DPX_OK) return false;

    // write image
    switch(saveToFile(fileName)) {
        case DPX_OK:
            break;
        case ERR_DPX_CANTOPEN:
            CImageUtility::showErrMsg("Fail to create image file %s in saveDPXImage()!\n", fileName);
            return false;
        case ERR_DPX_CANTCLOSE:
            CImageUtility::showErrMsg("Fail to close image file %s in saveDPXImage()!\n", fileName);
            return false;
        case ERR_DPX_NOTDPX:
            CImageUtility::showErrMsg("Invalid DPX header in %s in saveDPXImage()!\n", fileName);
            return false;
        case ERR_DPX_MEMORY:
            CImageUtility::showErrMsg("Fail to allocate buffer in file %s in saveDPXImage()!\n", fileName);
            return false;
        case ERR_DPX_FILE_OVERFLOW:
            CImageUtility::showErrMsg("Data overflow in file %s in saveDPXImage()!\n", fileName);
            return false;
        case ERR_DPX_UNEXPECTED_EOF:
            CImageUtility::showErrMsg("Unexpected EOF in file %s in saveDPXImage()!\n", fileName);
            return false;
        case ERR_DPX_UNSUPPORTED_FORMAT:
            CImageUtility::showErrMsg("Unsupported format in file %s in saveDPXImage()!\n", fileName);
            return false;
        default:
            CImageUtility::showErrMsg("Unknow error in file %s in saveDPXImage()!\n", fileName);
            return false;
    }

    return true;
}

void CDPXFileIO::setDftFileInfo(char szFilename[], int width, int height, int bit_depth, char szCreator[], int user_data_size)
{
    int pix_byte = 0;
    if (bit_depth <= 8) {
        pix_byte = 1;       // NOTE: UNCLEAR data type of 8U!!!
    } else {
        pix_byte = 2;
    }
    //infoFile
    if (inverted) {
        infoFile.magicNumber = 0x53445058;  // Magic Number: 0x53445058 (SDPX BIG-ENDIAN) or 0x58504453 (XPDS LITTLE-ENDIAN)
    } else {
        infoFile.magicNumber = 0x58504453;
    }
    infoFile.dittoKey = 1;			                // read time short cut - 0 = same, 1 = new
    infoFile.genericHdrSize = sizeof(infoFile) +  sizeof(dpxImageInformation) + sizeof(dpxImageOrientation); // generic header length in bytes (1664)
    infoFile.industryHdrSize = sizeof(dpxMotionPicture) + sizeof(dpxTelevision);   // industry header length in bytes (
    infoFile.userDataSize = sizeof(userData) + user_data_size - 4; // user-defined data length in bytes
    infoFile.offset = infoFile.genericHdrSize +     // offset to image data in bytes (8192) (6144)
                      infoFile.industryHdrSize + infoFile.userDataSize; 
    strcpy(infoFile.hdrVersion, "V1.0");            // which header format version is being used (v1.0)
    infoFile.fileSize = infoFile.offset +           // file size in bytes (28001792)
                        width * height * 3 * pix_byte;
    strncpy(infoFile.fileName, szFilename, 100);	// image file name
    time_t cur_time;
    _tzset();
    time( &cur_time );
    struct tm *today = localtime( &cur_time );      // cannot delete today!
    sprintf(infoFile.creationDate, "%04d:%02d:%02d:%02d:%02d:%02d:LTZ",  // file creation date "yyyy:mm:dd:hh:mm:ss:LTZ"
            today->tm_year+1900, today->tm_mon+1, today->tm_mday, today->tm_hour, today->tm_min, today->tm_sec);  
    strncpy(infoFile.creator, szCreator, 100);      // file creator's name
    strncpy(infoFile.project, "HAST and VEUHD at ASTRI", 200);  // project name
    strncpy(infoFile.copyright, "", 200);           // right to use or copyright info
    infoFile.encryptionKey = 0xFFFFFFFF;            // encryption ( FFFFFFFF = unencrypted )
    infoFile.reserved[0] = 0;                       // reserved field TBD (need to pad)
}

void CDPXFileIO::setDftImageInfo(int width, int height, int bit_depth, int user_data_size)
{
    //infoImage
    infoImage.imageOrientation = 0;                         // image orientation: left to right and top to bottom
    infoImage.imageElements = 1;                            // number of image elements 
    infoImage.x = width;									// pixels per line per element
    infoImage.y = height;									// lines per image element

    infoImage.imageElement[0].dataSign = 0;                 // data sign (0 = unsigned, 1 = signed )
    infoImage.imageElement[0].refLowData = 0;               // reference low data code value
    infoImage.imageElement[0].refLowQuantity = 0.0f;        // reference low quantity represented
    int ref_high_data;
    if (bit_depth >= 32) {
        ref_high_data = 0xFFFFFFFF;
    } else {
        ref_high_data = (1<<bit_depth) - 1;
    }
    infoImage.imageElement[0].refHighData = ref_high_data;  // reference high data code value
    infoImage.imageElement[0].refHighQuantity = 0.0f;       // reference high quantity represented
	//infoImage.imageElement[0].descriptor = '2';             // descriptor for image element
	infoImage.imageElement[0].descriptor = (U8)getDescriptor();  // Kayton code added: Write descriptor according to the input DPX's descriptor
    if (show_message) {
        CImageUtility::showMessage1("infoImage.imageElement[0].descriptor: %d...\n",getDescriptor());	// Kayton code for debug
    }
	infoImage.imageElement[0].transfer = ' ';               // transfer characteristics for element
	infoImage.imageElement[0].colorimetric = 0;             // colormetric specification for element
	infoImage.imageElement[0].bitDepth = (U8)bit_depth;         // bit depth for element
	//infoImage.imageElement[0].packing = 0;				// packing for element
	infoImage.imageElement[0].packing = (U16)getPacking();       // Kayton code added: Write packing number according to the input DPX's packing number
    if (show_message) {
        CImageUtility::showMessage1("infoImage.imageElement[0].packing: %d...\n",getPacking());			// Kayton code for debug
    }
	infoImage.imageElement[0].encoding = 0;                 // encoding for element
    int dataoff = sizeof(infoFile) +  sizeof(dpxImageInformation) + sizeof(dpxImageOrientation) +   // generic header length in bytes (1664)
                  sizeof(dpxMotionPicture) + sizeof(dpxTelevision) +                                // industry header length in bytes (
                  sizeof(userData) + user_data_size - 4;                                            // user-defined data length in bytes
	infoImage.imageElement[0].dataOffset = dataoff;         // offset to data of element
	//printf("infoImage.imageElement[0].dataOffset: %d...\n",dataoff);			// Kayton code for debug
	infoImage.imageElement[0].eolPadding = 0;               // end of line padding used in element
	infoImage.imageElement[0].eoiPadding = 0;               // end of image padding used in element
	infoImage.imageElement[0].description[0] = 0;           // description of element

    infoImage.reserved[0] = 0;                              // reserved for future use (padding)
}

void CDPXFileIO::setDftImageOri(int width, int height)
{
    //infoOrientation
    infoOrientation.xOffset = 0;                                //X offset
    infoOrientation.yOffset = 0;                                //Y offset
    infoOrientation.xCenter = 0.0f;                             //X center
    infoOrientation.yCenter = 0.0f;                             //Y center
    infoOrientation.xOriginalSize = width;                      //X original size
    infoOrientation.yOriginalSize = height;                     //Y original size
    infoOrientation.fileName[0] = 0;                            //source image file name
    infoOrientation.creationTime[0] = 0;                        //source image creation date and time
    strncpy(infoOrientation.inputDevice, "A001R10B", 31);       //input device name
    strncpy(infoOrientation.inputSerialNum, "LLHICASDASTRI", 31);//input device serial number
    infoOrientation.border[0] = 0;                              //border validity (XL, XR, YT, YB)
    infoOrientation.pixelAspect[0] = 1;                         //pixel aspect ratio (H:V)
    infoOrientation.pixelAspect[1] = 1;
    infoOrientation.reserved[0] = 0;                            //reserved for future use (padding)
}

void CDPXFileIO::setDftFilmInfo()
{
    //infoFilm
    infoFilm.filmManufacturer[0] = 0;   // film manufacturer ID code (2 digits from film edge code)
    infoFilm.filmManufacturer[1] = 0;
    infoFilm.filmType[0] = 0;           // file type (2 digits from film edge code)
    infoFilm.filmType[1] = 0;
    infoFilm.perfOffset[0] = 0;         // offset in perfs (2 digits from film edge code)
    infoFilm.perfOffset[1] = 0;
    infoFilm.prefix[0] = 0;             // prefix (6 digits from film edge code)
    infoFilm.prefix[1] = 0;
    infoFilm.prefix[2] = 0;
    infoFilm.prefix[3] = 0;
    infoFilm.prefix[4] = 0;
    infoFilm.prefix[5] = 0;
    infoFilm.count[0] = 0;              // count (4 digits from film edge code)
    infoFilm.count[1] = 0;
    infoFilm.count[2] = 0;
    infoFilm.count[3] = 0;
    strncpy(infoFilm.format, "academy", 31); // format (i.e. academy)
    infoFilm.framePosition = 0;         // frame position in sequence
    infoFilm.sequenceLength = 1;        // sequence length in frames
    infoFilm.heldCount = 1;             // held count (1 = default)
    infoFilm.frameRate = 24.0f;         // frame rate of original in frames/sec
    infoFilm.shutterAngle = 0.0f;       // shutter angle of camera in degrees
    infoFilm.frameId[0] = 0;            // frame identification (i.e. keyframe)
    infoFilm.slateInfo[0] = 0;          // slate information
    strncpy((char*)infoFilm.reserved, "ARRI", 55); // reserved for future use (padding)
}

void CDPXFileIO::setDftTVInfo()
{
    //infoTV
    infoTV.timeCode = 0x00133613;       // SMPTE time code
    infoTV.userBits = 0xFFFFFFFF;       // SMPTE user bits
    infoTV.interlaced = 0;              // interlace ( 0 = noninterlaced, 1 = 2:1 interlace )
    infoTV.fieldNumber = 0;             // field number
    infoTV.videoStandard = 0;           // video signal standard (table 4)
    infoTV.unused = 0;                  // used for byte alignment only
    infoTV.horizontalSampleRate = 0.0f; // horizontal sampling rate in Hz
    infoTV.verticalSampleRate = 0.0f;   // vertical sampling rate in Hz
    infoTV.frameRate = 24.0f;           // temporal sampling rate or frame rate in Hz
    infoTV.timeOffset = 0.0f;           // time offset from sync to first pixel
    infoTV.gamma = 0.0f;                // gamma value
    infoTV.blackLevel = 0.0f;           // black level code value
    infoTV.blackGain = 0.0f;            // black gain
    infoTV.breakPoint = 0.0f;           // breakpoint
    infoTV.whiteLevel = 0.0f;           // reference white level code value
    infoTV.integrationTimes = 0.0f;     // integration time(s)
    infoTV.reserved[0] = 0;             // reserved for future use (padding)
}

bool CDPXFileIO::setDftUserData(int size)
{
    freeUserData();

    //userData
	strncpy(userData.userId, "LLHICASDASTRI", 31);   // User-defined identification string 
    
    // allocate user data buffer
    userData.data = new unsigned char[size];
    if (userData.data == NULL) {
        return false;
    }

    memset(userData.data, 0, size);

    return true;
}

unsigned CDPXFileIO::getWidth (void)
{
	return infoImage.x;
}

unsigned CDPXFileIO::getHeight (void)
{
	return infoImage.y;
}

unsigned CDPXFileIO::getBitDepth (void)
{
	return infoImage.imageElement[0].bitDepth;
}

unsigned CDPXFileIO::getPacking (void)		//Kayton code added
{
	return infoImage.imageElement[0].packing;
}

unsigned CDPXFileIO::getDescriptor (void)	//Kayton code added
{
	return infoImage.imageElement[0].descriptor;
}

void * CDPXFileIO::getImageBuf()
// get the pointer to image buffer
// the pixel is in RGB order in the buffer
{
    return (void *)fullImage;
}

U16 CDPXFileIO::getRed(U16 x, U16 y)
{
	U16 r;
	dpxPixel pixelread;
				
	if (infoImage.imageElement[0].packing==1) {
		r=0;
	
		if (((U32)(x+1)>infoImage.x) || ((U32)(y+1)>infoImage.y)) return 0; // Out of image is seen black
				
		pixelread=fullImage[y*infoImage.x+x];
				
		pixelread=pixelread>>22;
		r=r|(pixelread&1023);
	} else {
		//U8 *channelPointer;
        U16 *channelPointer = (U16 *)fullImage;
		
		//channelPointer=(U8 *)fullImage;
		r=(U16) channelPointer[y*infoImage.x*3+(x*3)];
	}
	return r;
}

U16 CDPXFileIO::getGreen(U16 x, U16 y)
{
	U16 g;
	dpxPixel pixelread;
	
	if (infoImage.imageElement[0].packing==1) {			
		g=0;
	
		if (((U32)(x+1)>infoImage.x) || ((U32)(y+1)>infoImage.y)) return 0; // Out of image is seen black
				
		pixelread=fullImage[y*infoImage.x+x];
				
		pixelread=pixelread>>12;
		g=g|(pixelread&1023);
	} else {
		U8 *channelPointer;
		channelPointer=(U8 *)fullImage;
		g=(U16) channelPointer[y*infoImage.x+(x*3)+1];
	}

	return g;
}

U16 CDPXFileIO::getBlue(U16 x, U16 y)
{
	U16 b;
	dpxPixel pixelread;
				
	if (infoImage.imageElement[0].packing==1) {
		b=0;
	
		if (((U32)(x+1)>infoImage.x) || ((U32)(y+1)>infoImage.y)) return 0; // Out of image is seen black
				
		pixelread=fullImage[y*infoImage.x+x];
				
		pixelread=pixelread>>2;
		b=b|(pixelread&1023);
	} else {
		U8 *channelPointer;
		channelPointer=(U8 *)fullImage;
		b=(U16) channelPointer[y*infoImage.x+(x*3)+2];
	}

	return b;
}

U16 CDPXFileIO::getRed(U32 pos)
{
	U16 x, y;
	
	if (pos>=(infoImage.x*infoImage.y)) return 0; // Out of image is seen black
	
	y = (U16)(pos/infoImage.y);
	x = (U16)(pos%infoImage.y);
	
	return getRed(x,y);
}

U16 CDPXFileIO::getGreen(U32 pos)
{
	U16 x, y;
	
	if (pos>=(infoImage.x*infoImage.y)) return 0; // Out of image is seen black
	
	y = (U16)(pos/infoImage.y);
	x = (U16)(pos%infoImage.y);
	
	return getGreen(x,y);
}

U16 CDPXFileIO::getBlue(U32 pos)
{
	U16 x, y;
	
	if (pos>=(infoImage.x*infoImage.y)) return 0; // Out of image is seen black
	
	y = (U16)(pos/infoImage.y);
	x = (U16)(pos%infoImage.y);
		
	return getBlue(x,y);
}

void CDPXFileIO::setPixel(U16 x, U16 y, U16 r, U16 g, U16 b)
{
	U32 newPixel;
	
	if (((U32)(x+1)>infoImage.x) || ((U32)(y+1)>infoImage.y)) return; // Can't write off-limits
	
	// TODO: Support 8 bits packed and other unpacked formats
	
	if (infoImage.imageElement[0].packing==1) { // 10 bits-packed
		newPixel=0;
		newPixel=r;
		newPixel=newPixel<<10;
		newPixel=newPixel|g;
		newPixel=newPixel<<10;
		newPixel=newPixel|b;
		newPixel=newPixel<<2;
	
		fullImage[y*infoImage.x+x]=newPixel;
	} else { // 8 bits-unpacked
		U8 *channelPointer;
		channelPointer=(U8 *)fullImage;
		channelPointer[y*infoImage.x+(x*3)]=(U8) r;
		channelPointer[y*infoImage.x+(x*3)+1]=(U8) g;
		channelPointer[y*infoImage.x+(x*3)+2]=(U8) b;
	}
}

void CDPXFileIO::setPixel(U32 pos, U16 r, U16 g, U16 b)
{
	U16 x, y;
	
	if (pos>=(infoImage.x*infoImage.y)) return; // Can't write off-limits. U32 is unsigned so there's no values under 0

	y = (U16)(pos/infoImage.y);
	x = (U16)(pos%infoImage.y);
	
	setPixel(x,y,r,g,b);
	
	return;
}

void CDPXFileIO::getFileInformation(dpxFileInformation *fileInf)
{
	int counter;
	
	fileInf->magicNumber=infoFile.magicNumber;
	fileInf->offset=infoFile.offset;
	strcpy(fileInf->hdrVersion,infoFile.hdrVersion);
	fileInf->fileSize=infoFile.fileSize;
	fileInf->dittoKey=infoFile.dittoKey;
	fileInf->genericHdrSize=infoFile.genericHdrSize;
	fileInf->industryHdrSize=infoFile.industryHdrSize;
	fileInf->userDataSize=infoFile.userDataSize;
	strcpy(fileInf->fileName, infoFile.fileName);
	strcpy(fileInf->creationDate, infoFile.creationDate);
	strcpy(fileInf->creator, infoFile.creator);
	strcpy(fileInf->project, infoFile.project);
	strcpy(fileInf->copyright, infoFile.copyright);
	fileInf->encryptionKey=infoFile.encryptionKey;
	
	for (counter=0; counter<104; counter++)
		fileInf->reserved[counter]=infoFile.reserved[counter];
	
	return;
}

void CDPXFileIO::getImageInformation(dpxImageInformation *imgInf)
{
	int counter;
	
	imgInf->imageOrientation=infoImage.imageOrientation;
	imgInf->imageElements=infoImage.imageElements;
	imgInf->x=infoImage.x;
	imgInf->y=infoImage.y;
	
	for ( counter=0; counter<8; counter++) {
		imgInf->imageElement[counter].dataSign = infoImage.imageElement[counter].dataSign;
		imgInf->imageElement[counter].refLowData=infoImage.imageElement[counter].refLowData;
		imgInf->imageElement[counter].refLowQuantity=infoImage.imageElement[counter].refLowQuantity;
		imgInf->imageElement[counter].refHighData=infoImage.imageElement[counter].refHighData;
		imgInf->imageElement[counter].refHighQuantity=infoImage.imageElement[counter].refHighQuantity;
		imgInf->imageElement[counter].descriptor=infoImage.imageElement[counter].descriptor;
		imgInf->imageElement[counter].transfer=infoImage.imageElement[counter].transfer;
		imgInf->imageElement[counter].colorimetric=infoImage.imageElement[counter].colorimetric;
		imgInf->imageElement[counter].bitDepth=infoImage.imageElement[counter].bitDepth;
		imgInf->imageElement[counter].packing=infoImage.imageElement[counter].packing;
		imgInf->imageElement[counter].encoding=infoImage.imageElement[counter].encoding;
		imgInf->imageElement[counter].dataOffset=infoImage.imageElement[counter].dataOffset;
		imgInf->imageElement[counter].eolPadding=infoImage.imageElement[counter].eolPadding;
		imgInf->imageElement[counter].eoiPadding=infoImage.imageElement[counter].eoiPadding;
		strcpy(imgInf->imageElement[counter].description,infoImage.imageElement[counter].description);
	}
	
	for (counter=0; counter<52; counter++)
		imgInf->reserved[counter]=infoImage.reserved[counter];
	
	return;
}


void CDPXFileIO::getImageOrientation(dpxImageOrientation *imgOr)
{
	int counter;
	
	imgOr->xOffset=infoOrientation.xOffset;
	imgOr->yOffset=infoOrientation.yOffset;
	imgOr->xCenter=infoOrientation.xCenter;
	imgOr->yCenter=infoOrientation.yCenter;
	imgOr->xOriginalSize=infoOrientation.xOriginalSize;
	imgOr->yOriginalSize=infoOrientation.yOriginalSize;
	strcpy(imgOr->fileName,infoOrientation.fileName);
	strcpy(imgOr->creationTime,infoOrientation.creationTime);
	strcpy(imgOr->inputDevice,infoOrientation.inputDevice);
	strcpy(imgOr->inputSerialNum,infoOrientation.inputSerialNum);
	
	for (counter=0; counter<4; counter++)
		imgOr->border[counter]=infoOrientation.border[counter];
	
	for (counter=0; counter<2; counter++)
		imgOr->pixelAspect[counter]=infoOrientation.pixelAspect[counter];
	
	for (counter=0; counter<28; counter ++)
		imgOr->reserved[counter]=infoOrientation.reserved[counter];
	
	return;
}

void CDPXFileIO::getFilmInformation(dpxMotionPicture *film)
{
	int counter;
	
	strcpy(film->filmManufacturer,infoFilm.filmManufacturer);
	strcpy(film->filmType,infoFilm.filmType);
	strcpy(film->perfOffset,infoFilm.perfOffset);
	strcpy(film->prefix,infoFilm.prefix);
	strcpy(film->count,infoFilm.count);
	strcpy(film->format,infoFilm.format);
	film->framePosition=infoFilm.framePosition;
	film->sequenceLength=infoFilm.sequenceLength;
	film->heldCount=infoFilm.heldCount;
	film->frameRate=infoFilm.frameRate;
	film->shutterAngle=infoFilm.shutterAngle;
	strcpy(film->frameId, infoFilm.frameId);
	strcpy(film->slateInfo, infoFilm.slateInfo);
	
	for (counter=0; counter<8; counter++)
		film->reserved[counter]=infoFilm.reserved[counter];
	
	return;
}

void CDPXFileIO::getTVInformation(dpxTelevision *tv)
{
	int counter;
	
	tv->timeCode=infoTV.timeCode;
	tv->userBits=infoTV.userBits;
	tv->interlaced=infoTV.interlaced;
	tv->fieldNumber=infoTV.fieldNumber;
	tv->videoStandard=infoTV.videoStandard;
	tv->unused=infoTV.unused;
	tv->horizontalSampleRate=infoTV.horizontalSampleRate;
	tv->verticalSampleRate=infoTV.verticalSampleRate;
	tv->frameRate=infoTV.frameRate;
	tv->timeOffset=infoTV.timeOffset;
	tv->gamma=infoTV.gamma;
	tv->blackLevel=infoTV.blackLevel;
	tv->blackGain=infoTV.blackGain;
	tv->breakPoint=infoTV.breakPoint;
	tv->whiteLevel=infoTV.whiteLevel;
	tv->integrationTimes=infoTV.integrationTimes;
	
	for (counter=0; counter<76; counter++)
		tv->reserved[counter]=infoTV.reserved[counter];
	
	return;
}

void CDPXFileIO::getUserData(dpxUserData *user)
{
	unsigned int counter;
	
	if (infoFile.userDataSize>0 && infoFile.userDataSize<32) return;
		
	strcpy(user->userId,userData.userId);
	
	for ( counter=0; counter<infoFile.userDataSize; counter++)
		user->data[counter]=userData.data[counter];
	
	return;
	
}

void CDPXFileIO::setFileInformation(dpxFileInformation fileInf)
{
	infoFile.magicNumber=fileInf.magicNumber;
	infoFile.offset=fileInf.offset;
	strcpy(infoFile.hdrVersion,fileInf.hdrVersion);
	infoFile.fileSize=fileInf.fileSize;
	infoFile.dittoKey=fileInf.dittoKey;
	infoFile.genericHdrSize=fileInf.genericHdrSize;
	infoFile.industryHdrSize=fileInf.industryHdrSize;
	infoFile.userDataSize=fileInf.userDataSize;
	strcpy(infoFile.fileName, fileInf.fileName);
	strcpy(infoFile.creationDate, fileInf.creationDate);
	strcpy(infoFile.creator, fileInf.creator);
	strcpy(infoFile.project, fileInf.project);
	strcpy(infoFile.copyright, fileInf.copyright);
	infoFile.encryptionKey=fileInf.encryptionKey;
	strcpy(infoFile.reserved, fileInf.reserved);
	
	return;
}

void CDPXFileIO::setImageInformation(dpxImageInformation imgInf)
{
	int counter;
	
	infoImage.imageOrientation=imgInf.imageOrientation;
	infoImage.imageElements=imgInf.imageElements;
	infoImage.x=imgInf.x;
	infoImage.y=imgInf.y;
	
	for ( counter=0; counter<8; counter++) {
		infoImage.imageElement[counter].dataSign=imgInf.imageElement[counter].dataSign;
		infoImage.imageElement[counter].refLowData=imgInf.imageElement[counter].refLowData;
		infoImage.imageElement[counter].refLowQuantity=imgInf.imageElement[counter].refLowQuantity;
		infoImage.imageElement[counter].refHighData=imgInf.imageElement[counter].refHighData;
		infoImage.imageElement[counter].refHighQuantity=imgInf.imageElement[counter].refHighQuantity;
		infoImage.imageElement[counter].descriptor=imgInf.imageElement[counter].descriptor;
		infoImage.imageElement[counter].transfer=imgInf.imageElement[counter].transfer;
		infoImage.imageElement[counter].colorimetric=imgInf.imageElement[counter].colorimetric;
		infoImage.imageElement[counter].bitDepth=imgInf.imageElement[counter].bitDepth;
		infoImage.imageElement[counter].packing=imgInf.imageElement[counter].packing;
		infoImage.imageElement[counter].encoding=imgInf.imageElement[counter].encoding;
		infoImage.imageElement[counter].dataOffset=imgInf.imageElement[counter].dataOffset;
		infoImage.imageElement[counter].eolPadding=imgInf.imageElement[counter].eolPadding;
		infoImage.imageElement[counter].eoiPadding=imgInf.imageElement[counter].eoiPadding;
		strcpy(infoImage.imageElement[counter].description,imgInf.imageElement[counter].description);
	}
	
	return;
}

void CDPXFileIO::setImageOrientation(dpxImageOrientation imgOr)
{
	int counter;
	
	infoOrientation.xOffset=imgOr.xOffset;
	infoOrientation.yOffset=imgOr.yOffset;
	infoOrientation.xCenter=imgOr.xCenter;
	infoOrientation.yCenter=imgOr.yCenter;
	infoOrientation.xOriginalSize=imgOr.xOriginalSize;
	infoOrientation.yOriginalSize=imgOr.yOriginalSize;
	strcpy(infoOrientation.fileName, imgOr.fileName);
	strcpy(infoOrientation.creationTime, imgOr.creationTime);
	strcpy(infoOrientation.inputDevice, imgOr.inputDevice);
	strcpy(infoOrientation.inputSerialNum, imgOr.inputSerialNum);
	for ( counter=0; counter<4; counter++) 
		infoOrientation.border[counter]=imgOr.border[counter];
	for (counter=0; counter<2; counter++)
		infoOrientation.pixelAspect[counter]=imgOr.pixelAspect[counter];
	for (counter=0; counter<28; counter++)
		infoOrientation.reserved[counter]=imgOr.reserved[counter];
	
	return;
}

void CDPXFileIO::setFilmInformation(dpxMotionPicture film)
{
	int counter;
	
	strcpy(infoFilm.filmManufacturer, film.filmManufacturer);
	strcpy(infoFilm.filmType, film.filmType);
	strcpy(infoFilm.perfOffset, film.perfOffset);
	strcpy(infoFilm.prefix, film.prefix);
	strcpy(infoFilm.count, film.count);
	strcpy(infoFilm.format, film.format);
	infoFilm.framePosition=film.framePosition;
	infoFilm.sequenceLength=film.sequenceLength;
	infoFilm.heldCount=film.heldCount;
	infoFilm.frameRate=film.frameRate;
	infoFilm.shutterAngle=film.shutterAngle;
	strcpy(infoFilm.frameId, film.frameId);
	strcpy(infoFilm.slateInfo, film.slateInfo);
	
	for (counter=0; counter<56; counter++)
		infoFilm.reserved[counter]=film.reserved[counter];
	
	return;
}

void CDPXFileIO::setTVInformation(dpxTelevision tv)
{
	int counter;
	
	infoTV.timeCode=tv.timeCode;
	infoTV.userBits=tv.userBits;
	infoTV.interlaced=tv.interlaced;
	infoTV.fieldNumber=tv.fieldNumber;
	infoTV.videoStandard=tv.videoStandard;
	infoTV.unused=tv.unused;
	infoTV.horizontalSampleRate=tv.horizontalSampleRate;
	infoTV.verticalSampleRate=tv.verticalSampleRate;
	infoTV.frameRate=tv.frameRate;
	infoTV.timeOffset=tv.timeOffset;
	infoTV.gamma=tv.gamma;
	infoTV.blackLevel=tv.blackLevel;
	infoTV.breakPoint=tv.breakPoint;
	infoTV.whiteLevel=tv.whiteLevel;
	infoTV.integrationTimes=tv.integrationTimes;
	for (counter=0; counter<76; counter++)
		infoTV.reserved[counter]=tv.reserved[counter];
}


void CDPXFileIO::setUserData(dpxUserData user)
{
	unsigned int counter;
	
	if (infoFile.userDataSize>0 && infoFile.userDataSize<32) return;
	
	strcpy(userData.userId,user.userId);
	if (allocUserData()>=0)
	for ( counter=0; counter<infoFile.userDataSize; counter++)
		userData.data[counter]=user.data[counter];
	
	return;
}

