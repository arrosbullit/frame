/*!
	@file		mpeg2.h
	@author		Robert Lluís, december 2014
	@brief		Copied and modified from MPEG
*/
#ifndef __MPEG_2_H__
#define __MPEG_2_H__

#include "LittleBitstream2.h"

namespace MPEG2 {

void Initialize_Decoder();
int Decode_Bitstream(CLittle_Bitstream *bitsParam);

};


#endif


