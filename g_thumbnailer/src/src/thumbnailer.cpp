/*!
	@file		thumbnailer.cpp
	@brief		Create a thumbnail.
	@author		Robert Lluís, 2015
	@note
*/

#include <stdio.h>
#include <vector>
#include "thumbnailer.h"
#include "extract_coded_img_from_ts.h"
#include "mpeg2_dec.h"

using namespace std;

#ifdef TRACE
	#define PRINT(A) printf(A);
#else
	#define PRINT(A)
#endif

// Returns 0 if success.
int doThumbnail(const char *inFileName, unsigned pid, const char* outFileName)
{
	vector<unsigned char> buf;
	int error = 1;
	int success = 0;
	int i;
	const int MAX = 10;
	if(openInputFile(inFileName)){
		printf("Error by openInputFile\n");
		return 1;
	}
	for(i = 0; i < MAX; i++){
		CLittle_Bitstream bits;
		error = getCodedImage(pid, buf);
		if(error){ continue; }
		bits.init(&buf[0], buf.size());
		MPEG2::Initialize_Decoder();
		try{
			success = MPEG2::Decode_Bitstream(&bits);
			if(!success) {
				PRINT("INFO:Error by Decode_Bitstream\n");
			}
		} catch (...){
			success = 0;
			PRINT("Exc. by Decode_Bitstream.\n");
		}
		if(success == 1){
			break;
		}
	}
	if(success == 1){
		success = MPEG2::write_ppm_file(outFileName);
		if(!success){
			success = 0;
			printf("Thumbnailer: Error by write_ppm_file.\n");
		}
	}
	MPEG2::Uninitialize_Decoder();
	closeInputFile();
	if(!success){
		printf("Thumbnailer: failed.\n");
	}
	// !success because MPEG2 returns a success code
	// while I usually return error.
	return !success;
}

