/*!
	@file		MPEG2_getblk.cpp
	@author		Robert Lluís, december 2014
	@brief		Based on MPEG
	
*/

#include "mpeg2_getblk.h"
#include "mpeg2_getvlc.h"
#include "mpeg2_getvlc_private2.h"
#include "mini_logger.h"
#include <stdio.h>

namespace MPEG2 {

#define Show_Bits(A) bits->ShowBits(A)
#define Flush_Buffer(A) bits->GetBits(A)
#define Get_Bits1() bits->GetBits(1)
#define Get_Bits(A) bits->GetBits(A)

/* decode one intra coded MPEG-2 block */

void Decode_MPEG2_Intra_Block(int comp, int* dc_dct_pred, 
	CLittle_Bitstream *bits, struct MPEG2_parsed *d)
{
	int val, i, sign, cc, run;
	unsigned int code;
	DCTtab *tab;
	short *bp;
	//int nc;
	
	bp = d->block[comp];

	cc = (comp<4) ? 0 : (comp&1)+1;

	/* ISO/IEC 13818-2 section 7.2.1: decode DC coefficients */
	if (cc==0)
		val = (dc_dct_pred[0]+= Get_Luma_DC_dct_diff());
	else if (cc==1)
		val = (dc_dct_pred[1]+= Get_Chroma_DC_dct_diff());
	else
		val = (dc_dct_pred[2]+= Get_Chroma_DC_dct_diff());

	if (d->Fault_Flag) return;

	bp[0] = val << (3-d->intra_dc_precision);

	//nc=0;

	#ifdef TRACE
		printf("DCT(%d): %d",comp, val);
		ML_log("DCT(%d): %d",comp, val);
	#endif /* TRACE */

	/* decode AC coefficients */
	for (i=1; ; i++)
	{
		code = Show_Bits(16);
		if (code>=16384 && !d->intra_vlc_format)
			tab = &DCTtabnext[(code>>12)-4];
		else if (code>=1024)
		{
			if (d->intra_vlc_format)
				tab = &DCTtab0a[(code>>8)-4];
			else
				tab = &DCTtab0[(code>>8)-4];
		}
		else if (code>=512)
		{
			if (d->intra_vlc_format)
				tab = &DCTtab1a[(code>>6)-8];
			else
				tab = &DCTtab1[(code>>6)-8];
		}
		else if (code>=256)
			tab = &DCTtab2[(code>>4)-16];
		else if (code>=128)
			tab = &DCTtab3[(code>>3)-16];
		else if (code>=64)
			tab = &DCTtab4[(code>>2)-16];
		else if (code>=32)
			tab = &DCTtab5[(code>>1)-16];
		else if (code>=16)
			tab = &DCTtab6[code-16];
		else
		{
			//if (!Quiet_Flag)
			#ifdef TRACE
			printf("invalid Huffman code in Decode_MPEG2_Intra_Block()\n");
			ML_log("invalid Huffman code in Decode_MPEG2_Intra_Block()\n");
			#endif
			d->Fault_Flag = 1;
			return;
		}

		Flush_Buffer(tab->len);

		#ifdef TRACE
		//printf(" (");
		//Print_Bits(code,16,tab->len);
		#endif /* TRACE */

		if (tab->run==64) /* end_of_block */
		{
			#ifdef TRACE
			printf("): EOB\n");
			ML_log("): EOB\n");
			#endif /* TRACE */
			return;
		}

		if (tab->run==65) /* escape */
		{
			#ifdef TRACE
			//putchar(' ');
			//Print_Bits(Show_Bits(6),6,6);
			#endif /* TRACE */

			i+= run = Get_Bits(6);

			#ifdef TRACE
			//putchar(' ');
			//Print_Bits(Show_Bits(12),12,12);
			#endif /* TRACE */

			val = Get_Bits(12);
			if ((val&2047)==0)
			{
				//if (!Quiet_Flag)
				#ifndef TRACE
				printf("invalid escape in Decode_MPEG2_Intra_Block()\n");
				ML_log("invalid escape in Decode_MPEG2_Intra_Block()\n");
				#endif
				d->Fault_Flag = 1;
				return;
			}
			if((sign = (val>=2048)))
				val = 4096 - val;
		}
		else
		{
			i+= run = tab->run;
			val = tab->level;
			sign = Get_Bits(1);

			#ifdef TRACE
			//printf("%d",sign);
			#endif /* TRACE */
		}

		if (i>=64)
		{
			#ifdef TRACE
			fprintf(stderr,"DCT coeff index (i) out of bounds (intra2)\n");
			ML_log("DCT coeff index (i) out of bounds (intra2)\n");
			#endif
			d->Fault_Flag = 1;
			return;
		}

		#ifdef TRACE
		//printf("): %d/%d",run,sign ? -val : val);
		#endif /* TRACE */

		//j = scan[ld1->alternate_scan][i];
		//val = (val * ld1->quantizer_scale * qmat[j]) >> 4;
		//bp[j] = sign ? -val : val;
		//nc++;

		//if (base.scalable_mode==SC_DP && nc==base.priority_breakpoint-63)
		//	ld = &enhan;
	}
}

}; // namespace MPEG2
