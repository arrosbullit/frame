/*!
	@file		MPEG2_getvlc.h
	@author		Robert Lluís, december 2014
	@brief		Copied and modified from MPEG
*/
#ifndef __MPEG2_GET_VLC_H__
#define __MPEG2_GET_VLC_H__

#include "LittleBitstream2.h"

namespace MPEG2 {

int Get_macroblock_address_increment(CLittle_Bitstream *bits);
int Get_I_macroblock_type();
int Get_macroblock_type();
int Get_Chroma_DC_dct_diff();
int Get_Luma_DC_dct_diff();

}; // namespace MPEG2


#endif
