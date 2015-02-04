/*!
	@file		MPEG2_getpic.h
	@author		Robert Lluís, december 2014
	@brief		Copied and modified from: 
				getpic.c 
				1996 MPEG Software Simulation Group.
*/
#ifndef __MPEG2_GET_PIC_H__
#define __MPEG2_GET_PIC_H__

#include "LittleBitstream2.h"

namespace MPEG2 {

void Decode_Picture(CLittle_Bitstream *bitsParam);

}; // namespace MPEG2


#endif






