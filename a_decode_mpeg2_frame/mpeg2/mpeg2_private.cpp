/*!
	@file		mpeg2_private.cpp
	@author		Robert Lluís, december 2014
	@brief		Copied and modified from MPEG
*/
#include "mpeg2_private.h"
#include <stdio.h>

namespace MPEG2 {
struct MPEG2_parsed data;
static unsigned char scan[2][64] =
{
  { /* Zig-Zag scan pattern  */
    0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
    12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
    35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
  },
  { /* Alternate scan pattern */
    0,8,16,24,1,9,2,10,17,25,32,40,48,56,57,49,
    41,33,26,18,3,11,4,12,19,27,34,42,50,58,35,43,
    51,59,20,28,5,13,6,14,21,29,36,44,52,60,37,45,
    53,61,22,30,7,15,23,31,38,46,54,62,39,47,55,63
  }
};
static unsigned char default_intra_quantizer_matrix[64]
=
{
  8, 16, 19, 22, 26, 27, 29, 34,
  16, 16, 22, 24, 27, 29, 34, 37,
  19, 22, 26, 27, 29, 34, 34, 38,
  22, 22, 26, 27, 29, 34, 37, 40,
  22, 26, 27, 29, 32, 35, 40, 48,
  26, 27, 29, 32, 35, 40, 48, 58,
  26, 27, 29, 34, 38, 46, 56, 69,
  27, 29, 35, 38, 46, 56, 69, 83
};

struct MPEG2_parsed * getStruct()
{
	return &data;
}
void next_start_code(CLittle_Bitstream *bits)
{
	bits->get_byte_aligned();
	while (bits->ShowBits(24)!=0x01L){
		bits->GetBits(8);
	}
}
unsigned char (*getScanTable())[64]
{
	return scan;
}
unsigned char *getDefaultIntraQuantizerMatrix()
{
	return default_intra_quantizer_matrix;
}

/* Trace_Flag output */
void Print_Bits(int code, int bits, int len)
{
	int i;
	for (i = 0; i < len; i++){
		printf("%d",(code>>(bits-1-i))&1);
	}
}

}; // namespace MPEG2
