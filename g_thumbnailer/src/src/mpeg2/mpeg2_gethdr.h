/*!
	@file		mpeg2_gethdr.h
	@author		Robert Lluís, december 2014
	@brief		Based on MPEG
*/
#ifndef __MPEG2_GET_HDR_H__
#define __MPEG2_GET_HDR_H__

#include "LittleBitstream2.h"

namespace MPEG2 {

int Get_Hdr(CLittle_Bitstream *bitsParam);

};

#endif
