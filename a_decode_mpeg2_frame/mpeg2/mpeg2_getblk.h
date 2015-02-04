/*!
	@file		MPEG2_getblk.h
	@author		Robert Lluís, december 2014
	@brief		Copied and modified from: 
				getblk.c 
				1996 MPEG Software Simulation Group.
*/
#ifndef __MPEG2_GET_BLK_H__
#define __MPEG2_GET_BLK_H__

#include "LittleBitstream2.h"
#include "mpeg2_private.h"

namespace MPEG2 {

void Decode_MPEG2_Intra_Block(int comp, int* dc_dct_pred, 
	CLittle_Bitstream *bits, struct MPEG2_parsed *d);

}; // namespace MPEG2


#endif






