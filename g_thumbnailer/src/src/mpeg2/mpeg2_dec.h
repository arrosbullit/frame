/*!
	@file		mpeg2.h
	@author		Robert Lluís, december 2014
	@brief		Based on MPEG
*/
#ifndef __MPEG_2_H__
#define __MPEG_2_H__

#include "LittleBitstream2.h"

namespace MPEG2 {

void Initialize_Decoder();
void Uninitialize_Decoder();
int Decode_Bitstream(CLittle_Bitstream *bitsParam);
int write_ppm_file(const char *fileName);

};


#endif


