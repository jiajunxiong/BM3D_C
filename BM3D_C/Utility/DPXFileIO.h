// DPX file I/O functions modified by Luhong Liang, ICD-ASD, ASTRI
// Based on the open source file CDPXFileIO.h and CDPXFileIO.cpp

/**
\file CDPXFileIO.h
Created by Javier Hernanz Zajara on 12/01/07.

Class and related resources to load DPX files.
 
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
 
*************
\class CDPXFileIO
This class is intended to manage fully DPX images in order to provide an easy interface with
other classes and functions.

It converts the original DPX file, when loading, to native endianess (big/little endian) and operates in
native mode (setting the variable <i>inverted</i> in order to remember the byte order). 
When writing to file it checks for the <i>inverted</i> property in order to write the new file in the same
order than the original one. This working can be changed using the followinf sentence (once the DPX is
loaded): <b>setInverted(!getInverted);</b>

<b>TO DO:</b> <ul>
		<li> Check consistency for invert* functions to assure good working on all platforms</li>
		<li> Add enhaced control over errors and exception while loading, for example, bad headers</li>
		<li> Copy images in memory</li>
		<li> Generate new images from the scratch</li>
		<li> Enhaced control pixel by pixel (read and write operations)</li>
		<li> Support for 16 bits and unsupported files</li>
		<li> Offer the possibility to read image data using other DPX file as header</li>
		<li> Locate properly (x,y) values depending on orientation</li>
		<li> Correct writeToFile method to don't convert original headers twice if inverted</li>
		<li> Invert endianess (Big->Little or Little->Big) of the DPX</li>
		<li> implement propietary strcpy function to avoid reading/core dumped problems caused by bad/corrupt headers</li>
		<li> Check what happens when loading/writing close to end of image (last pixel with 'pos' positioning)</li>
		<li> If infoFile.fileSize=0 then there will be problems. Solve all of them. loadFromFile() is marked</li>
		<li> Add support for more than one imageElement</li>
		<li> Add support for RGBA under all bitDepths</li>
		<li> Convert between DPX formats</li>
 		</ul>
 
 Supported Read/Write operations depending on format
 <table>
             8 bits  10 bits  16 bits
   Packed      -       R/W       -   
   Unpacked   R/W       -        -   
 </table>
 
 
 <b>CHANGE LOG:</b><ul>
 			<li> 0.1.1<br> First functional version </li>
 			<li> 0.1.2<br> 
 				GNU GPL Licensed<br>
 				solved bug while loading DPX from file<br>
				added function: void setPixel(U32 pos, U16 r, U16 g, U16 b)<br>
				added functions: getRed(U32 pos), getGreen(U32 pos) and getBlue(U32 pos)<br>
				added support for 8bits-unpacked reading and writing<br>
				added fullImageSize<br>
				getPackedPixel() DEPRECATED<br></li>
			<li> 0.1.3<br>
 </li>
 </ul>
 
 
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

#pragma once
#pragma warning(disable : 4996)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// DEFINITIONS 

// This is the maximun length for a file name 
#define MAXFILENAME			512

// ERROR DEFINITIONS 

// Returned when no errors 
#define DPX_OK				0

// Tried to open a file that couldn't be opened 
#define ERR_DPX_CANTOPEN	-1
// Tried to close a file that couldn't be closed 
#define ERR_DPX_CANTCLOSE	-2
// Tried to open a file which is not DPX or has a bad header
#define ERR_DPX_NOTDPX		-3
// Memory allocation for data unsucessfully 
#define ERR_DPX_MEMORY		-4
// In the DPX file there's more information than specified in the header 
#define ERR_DPX_FILE_OVERFLOW -5
// In the DPX header it's supposed to be more information. 
// This could be a DPX corrupt file or an unexpected EOF 
#define ERR_DPX_UNEXPECTED_EOF -6

#define ERR_DPX_UNSUPPORTED_FORMAT -7

// TYPE DEFINITIONS 

// Unsigned 32 bits long type
typedef unsigned int U32;//typedef unsigned long   U32;
// Unsigned 16 bits long type
typedef unsigned short  U16;
// Unsigned 8 bits long type
typedef unsigned char   U8;
// Real/Float 32 bits long type
typedef float           R32;
// Real/Float 64 bits long type
typedef double          R64;
// ASCCI type (8 bits character)
typedef char            ASCII;
// Unsigned 32 bits long type is default to store a pixel
typedef U32				dpxPixel;

#include "ImgProcUtility.h"

// Struct to store DPX's File Information header
typedef struct _dpxFileInformation
{
    U32		magicNumber;        // Magic Number: 0x53445058 (SDPX BIG-ENDIAN) or 0x58504453 (XPDS LITTLE-ENDIAN) 
    U32     offset;             // offset to image data in bytes 
    ASCII	hdrVersion[8];		// which header format version is being used (v1.0)
    U32     fileSize;			// file size in bytes 
    U32		dittoKey;			// read time short cut - 0 = same, 1 = new 
    U32		genericHdrSize;     // generic header length in bytes 
    U32		industryHdrSize;    // industry header length in bytes 
    U32		userDataSize;		// user-defined data length in bytes 
    ASCII	fileName[100];		// image file name 
    ASCII	creationDate[24];   // file creation date "yyyy:mm:dd:hh:mm:ss:LTZ" 
    ASCII   creator[100];       // file creator's name 
    ASCII   project[200];       // project name 
    ASCII   copyright[200];     // right to use or copyright info 
    U32     encryptionKey;      // encryption ( FFFFFFFF = unencrypted ) 
    ASCII   reserved[104];      // reserved field TBD (need to pad) 
} dpxFileInformation;

// Struct to store DPX's Image Information header
typedef struct _dpxImageInformation
{
    U16    imageOrientation;    // image orientation<br><table>
							    // Value  Line Scan Direction  Page Scan Direction
						        //  0     Left to Right        Top to Bottom     
							    //  1     Left to Right        Bottom to top     
							    //  2     Right to Left        Top to Bottom     
							    //  3     Right to Left        Bottom to top     
							    //  4     Top to Bottom        Left to Right     
							    //  5     Top to Bottom        Right to Left     
							    //  6     Bottom to Top        Left to Right     
							    //  7     Bottom to Top        Right to Left     
    U16    imageElements;       // number of image elements 
    U32    x;					// pixels per line per element 
    U32    y;					// lines per image element 
    struct _imageElement
    {
        U32    dataSign;        // data sign (0 = unsigned, 1 = signed ) 
								// "Core set images are unsigned" 
        U32    refLowData;      // reference low data code value 
        R32    refLowQuantity;  // reference low quantity represented 
        U32    refHighData;     // reference high data code value 
        R32    refHighQuantity; // reference high quantity represented 
		U8     descriptor;      // descriptor for image element 
		U8     transfer;        // transfer characteristics for element 
		U8     colorimetric;    // colormetric specification for element 
		U8     bitDepth;        // bit depth for element 
		U16    packing;         // packing for element 
		U16    encoding;        // encoding for element 
		U32    dataOffset;      // offset to data of element 
		U32    eolPadding;      // end of line padding used in element 
		U32    eoiPadding;      // end of image padding used in element 
		ASCII  description[32]; // description of element 
    } imageElement[8];          // NOTE THERE ARE EIGHT OF THESE 

    U8 reserved[52];            // reserved for future use (padding) 
} dpxImageInformation;

// Struct to store DPX's Image Orientation header.

// Cropping, history, borders, etc.
typedef struct _dpxImageOrientation
{
    U32   xOffset;               // X offset 
    U32   yOffset;               // Y offset 
    R32   xCenter;               // X center 
    R32   yCenter;               // Y center 
    U32   xOriginalSize;         // X original size 
    U32   yOriginalSize;         // Y original size 
    ASCII fileName[100];         // source image file name 
    ASCII creationTime[24];      // source image creation date and time 
    ASCII inputDevice[32];       // input device name 
    ASCII inputSerialNum[32];    // input device serial number 
    U16   border[4];             // border validity (XL, XR, YT, YB) 
    U32   pixelAspect[2];        // pixel aspect ratio (H:V) 
    U8    reserved[28];          // reserved for future use (padding) 
} dpxImageOrientation;

// Struct to store DPX's Motion Picture Information header
typedef struct _dpxMotionPicture    // CAUTION! It's a DPX optional header. Check dpxFileInformation.industryHdrSize value 
{
    ASCII filmManufacturer[2];   // film manufacturer ID code (2 digits from film edge code) 
    ASCII filmType[2];           // file type (2 digits from film edge code) 
    ASCII perfOffset[2];         // offset in perfs (2 digits from film edge code)
    ASCII prefix[6];             // prefix (6 digits from film edge code) 
    ASCII count[4];              // count (4 digits from film edge code)
    ASCII format[32];            // format (i.e. academy) 
    U32   framePosition;         // frame position in sequence 
    U32   sequenceLength;        // sequence length in frames 
    U32   heldCount;             // held count (1 = default) 
    R32   frameRate;             // frame rate of original in frames/sec 
    R32   shutterAngle;          // shutter angle of camera in degrees 
    ASCII frameId[32];           // frame identification (i.e. keyframe) 
    ASCII slateInfo[100];        // slate information 
    U8    reserved[56];          // reserved for future use (padding) 
} dpxMotionPicture;

// Struct to store DPX's Television Related Information header
typedef struct _dpxTelevision       // CAUTION! It's a DPX optional header. Check dpxFileInformation.industryHdrSize value 
{
    U32 timeCode;                // SMPTE time code 
    U32 userBits;                // SMPTE user bits 
    U8  interlaced;              // interlace ( 0 = noninterlaced, 1 = 2:1 interlace )
    U8  fieldNumber;             // field number 
    U8  videoStandard;           // video signal standard (table 4)
    U8  unused;                  // used for byte alignment only 
    R32 horizontalSampleRate;    // horizontal sampling rate in Hz 
    R32 verticalSampleRate;      // vertical sampling rate in Hz 
    R32 frameRate;               // temporal sampling rate or frame rate in Hz 
    R32 timeOffset;              // time offset from sync to first pixel 
    R32 gamma;                   // gamma value 
    R32 blackLevel;              // black level code value 
    R32 blackGain;               // black gain 
    R32 breakPoint;              // breakpoint 
    R32 whiteLevel;              // reference white level code value 
    R32 integrationTimes;        // integration time(s) 
    U8  reserved[76];            // reserved for future use (padding) 
} dpxTelevision;

// Struct to store DPX's User Data header
typedef struct _dpxUserDefinedData 
{ 
	ASCII userId[32];		     // User-defined identification string  
	U8 *data;                    // User-defined data. CAUTION! It's variable size  
} dpxUserData; 


class CDPXFileIO
{
private:
	bool				inverted;				// Are we reading the file in a different "endian" machine from it's creation?, should be byte order inverted?
	dpxPixel			*fullImage;				// Memory to store full image in original format
	U32					fullImageSize;			// Size in bytes reserved for fullImage
	dpxFileInformation  infoFile;				// General Header of the DPX
	dpxImageInformation infoImage;				// Image information header of the DPX
	dpxImageOrientation infoOrientation;		// Information relative to geometric specifications of the DPX
	dpxMotionPicture    infoFilm;				// Industry header relative to Motion Picture Film
	dpxTelevision		infoTV;					// Industry header relative to Television
	dpxUserData			userData;				// User data... REMEMBER! It's dynamic!
	char				openFile[MAXFILENAME];	// DPX file name loaded (or tried last time)
	
	U16 invert16 (U16 readValue);				// Inverts big/little endian to current platform (16 bits)
	U32 invert32 (U32 readValue);				// Inverts big/little endian to current platform (32 bits)
	R32 invertR32 (R32 readValue);				// Inverts big/little endian to current platform (Real 32 bits)
 	int convertMainHeaders (void);				// Converts File, Image and Orientation headers if needed
	void convertFilmHeader (void);				// Converts Industry: Motion Picture header if needed
	void convertTVHeader (void);				// Converts Industry: Television header if needed
	void freeUserData (void);					// Frees memory reserved for user data if needed
	int allocUserData (void);					// Allocate memory (frees memory first) for User Data if needed
	void freeImage (void);						// Frees memory reserved for fullImage if needed
	int allocImage (void);						// Allocate memory (frees memory first) for the fullImage

    bool show_message;          // switch on/off internal message about DPX file

public:
	CDPXFileIO (bool show_msg = true);		// This constructor does not reserve memory for variable size data
	~CDPXFileIO ();								// This destructor free all memory

    // OpenCV functions
public:
    IplImage *loadDPXImage(char *fileName);
    bool saveDPXImage(char *fileName, IplImage *iplImage, int bit_depth);

    int loadFromFile (char *fileName);			        // Load the DPX image from a file
	int saveToFile (char *fileName);			        // Save the DPX image to a file 

private:
    int updateImage(void *pImg, int width, int height, int channels, int bit_depth, char szFilename[]);
	void setDftFileInfo(char szFilename[], int width, int height, int bit_depth, char szCreator[], int user_data_size);	
	void setDftImageInfo(int width, int height, int bit_depth, int user_data_size);
	void setDftImageOri(int width, int height);
	void setDftFilmInfo();
	void setDftTVInfo();
    bool setDftUserData(int size);

public:
	unsigned getWidth ();						// Width of the picture
	unsigned getHeight ();						// Height of the picture
	unsigned getBitDepth ();					// Bit depth of the picture
	unsigned getPacking ();						// Packing method of the picture		//Kayton code added
	unsigned getDescriptor ();					// Descriptor of the picture			//Kayton code added
    void *getImageBuf();                        // by Luhong
	U16 getRed(U16 x, U16 y);					// Returns red channel of pixel (x,y) 
	U16 getGreen(U16 x, U16 y);					// Returns green channel of pixel (x,y) 
	U16 getBlue(U16 x, U16 y);					// Returns blue channel of pixel (x,y) 
	U16 getRed(U32 pos);						// Returns red channel of pixel at position 'pos'. Should be interpreted as all the picture in in one line. 
	U16 getGreen(U32 pos);						// Returns green channel of pixel at position 'pos'. Should be interpreted as all the picture in in one line. 
	U16 getBlue(U32 pos);						// Returns blue channel of pixel at position 'pos'. Should be interpreted as all the picture in in one line. 
	bool getInverted (void);					// Returns if loaded DPX was endian inverted or not 
	void setInverted (bool newInverted);		// Sets inverted value for the loaded DPX to later writing to file
	void setPixel(U16 x, U16 y, U16 r, U16 g, U16 b);		// Sets pixel (x,y) with value (r,g,b) 
	void setPixel(U32 pos, U16 r, U16 g, U16 b);			// Sets pixel 'pos' with value (r,g,b). Should be interpreted as all the picture in in one line. 
	void getFileInformation(dpxFileInformation *fileInf);	// Returns on fileInf current infoFile struct 
	void getImageInformation(dpxImageInformation *imgInf);	// Returns on imgInf current infoImage struct 
	void getImageOrientation(dpxImageOrientation *imgOr);	// Returns on imgOr current infoOrientation struct 
	void getFilmInformation(dpxMotionPicture *film);		// Returns on film current infoFilm struct 
	void getTVInformation(dpxTelevision *tv);				// Returns on tv current infoTV 
	
	// Returns on user current userData
	//
	// <b>WARNING!</b> Before using this function correct dpxFileInformation.userDataSize must be set 
	//
	// dpxFileInformation.userDataSize must be 0, 32 or greater depending on data. If this value
	// is set between 1-31 (both included) this function won't work at all. Otherwise this function
	// won't check for correct memory usage, so can cause segmentation faults and similar issues
	// if not used properly.
	//
	// <b>IT IS SUPPOSED THAT USER HAS RESERVED, AT LEAST, ENOUGH MEMORY FOR user.data</b>
	
	void getUserData(dpxUserData *user);
	void setFileInformation(dpxFileInformation fileInf);	// Sets current infoFile from fileInf 
	void setImageInformation(dpxImageInformation imgInf);   // Sets current infoImage from imgInf 
	void setImageOrientation(dpxImageOrientation imgOr);	// Sets current infoOrientation from imgOr 
	void setFilmInformation(dpxMotionPicture film);			// Sets current infoFilm from film 
	void setTVInformation(dpxTelevision tv);				// Sets current infoTV fromo tv 

	//
	// Sets current userData from data
	//
	// <b>WARNING!</b> Before using this function correct dpxFileInformation.userDataSize must be set 
	//
	// dpxFileInformation.userDataSize must be 0, 32 or greater depending on data. If this value
	// is set between 1-31 (both included) this function won't work at all. Otherwise this function
	// won't check for correct memory usage, so can cause segmentation faults and similar issues
	// if not used properly.
	 
	void setUserData(dpxUserData user);
};