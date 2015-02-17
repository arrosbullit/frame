/*!
	@file		MPEG2_getpic.h
	@author		Robert Lluís, december 2014
	@brief		Based on MPEG
*/
#ifndef __MPEG2_GET_PIC_H__
#define __MPEG2_GET_PIC_H__

#include "LittleBitstream2.h"

namespace MPEG2 {

void Decode_Picture(CLittle_Bitstream *bitsParam);

}; // namespace MPEG2


#endif






