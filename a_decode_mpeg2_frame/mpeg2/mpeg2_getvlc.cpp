/*!
	@file		MPEG2_getvlc.cpp
	@author		Robert Lluís, december 2014
	@brief		Copied and modified from MPEG
*/

#include "mpeg2_getvlc.h"
#include "mpeg2_private.h"
#include "mpeg2_getvlc_private.h"
#include "mini_logger.h"
#include <stdio.h>

namespace MPEG2 {

static struct MPEG2_parsed *d;
static CLittle_Bitstream *bits;

#define Show_Bits(A) bits->ShowBits(A)
#define Flush_Buffer(A) bits->GetBits(A)
#define Get_Bits1() bits->GetBits(1)
#define Get_Bits(A) bits->GetBits(A)

// Aquesta �s cridada en primer lloc o sigui que aprofito
// per guardar-me el bits i el d
int Get_macroblock_address_increment(CLittle_Bitstream *bitsParam)
{
	int code, val;
	d = getStruct();
	bits = bitsParam;

#ifdef TRACE
	//if (d->Trace_Flag)
		printf("macroblock_address_increment ");
		ML_log("macroblock_address_increment ");
#endif /* TRACE */

	val = 0;

	while ((code = Show_Bits(11))<24)
	{
		if (code!=15) /* if not macroblock_stuffing */
		{
			if (code==8) /* if macroblock_escape */
			{
#ifdef TRACE
				//if (d->Trace_Flag)
					printf("00000001000 ");
					ML_log("00000001000 ");
#endif /* TRACE */

				val+= 33;
			}
			else
			{
				//if (!d->Quiet_Flag)
					printf("Invalid macroblock_address_increment code\n");
					ML_log("Invalid macroblock_address_increment code\n");

				d->Fault_Flag = 1;
				return 1;
			}
		}
		else /* macroblock suffing */
		{
#ifdef TRACE
			//if (d->Trace_Flag)
				printf("00000001111 ");
				ML_log("00000001111 ");
#endif /* TRACE */
		}

		Flush_Buffer(11);
	}

	/* macroblock_address_increment == 1 */
	/* ('1' is in the MSB position of the lookahead) */
	if (code>=1024)
	{
		Flush_Buffer(1);
#ifdef TRACE
		//if (d->Trace_Flag)
			printf("1): %d\n",val+1);
			ML_log("1): %d\n",val+1);
#endif /* TRACE */
		return val + 1;
	}

	/* codes 00010 ... 011xx */
	if (code>=128)
	{
		/* remove leading zeros */
		code >>= 6;
		Flush_Buffer(MBAtab1[code].len);

#ifdef TRACE
		if (d->Trace_Flag)
		{
			//Print_Bits(code,5,MBAtab1[code].len);
			printf("): %d\n",val+MBAtab1[code].val);
			ML_log("): %d\n",val+MBAtab1[code].val);
		}
#endif /* TRACE */


		return val + MBAtab1[code].val;
	}

	/* codes 00000011000 ... 0000111xxxx */
	code-= 24; /* remove common base */
	Flush_Buffer(MBAtab2[code].len);

#ifdef TRACE
	//if (d->Trace_Flag)
	//{
		//Print_Bits(code+24,11,MBAtab2[code].len);
		printf("): %d\n",val+MBAtab2[code].val);
		ML_log("): %d\n",val+MBAtab2[code].val);
	//}
#endif /* TRACE */

	return val + MBAtab2[code].val;
}


int Get_macroblock_type()
{
	if(d->picture_coding_type != I_TYPE){
		printf("Robert error: picture type not supported\n");
		d->Fault_Flag = 1;
	}
	return Get_I_macroblock_type();
}

int Get_I_macroblock_type()
{
	#ifdef TRACE
		printf("macroblock_type(I) ");
		ML_log("macroblock_type(I) ");
	#endif /* TRACE */
	if (Get_Bits1())
	{
		#ifdef TRACE
			printf("(1): Intra (1)\n");
			ML_log("(1): Intra (1)\n");
		#endif /* TRACE */
		return 1;
	}
	if (!Get_Bits1())
	{
		printf("Invalid macroblock_type code\n");
		ML_log("Invalid macroblock_type code\n");
		d->Fault_Flag = 1;
	}
	#ifdef TRACE
		printf("(01): Intra, Quant (17)\n");
		ML_log("(01): Intra, Quant (17)\n");
	#endif /* TRACE */
		return 17;
}

/* combined MPEG-1 and MPEG-2 stage. parse VLC and 
   perform dct_diff arithmetic.

   MPEG-1:  ISO/IEC 11172-2 section
   MPEG-2:  ISO/IEC 13818-2 section 7.2.1 
   
   Note: the arithmetic here is presented more elegantly than
   the spec, yet the results, dct_diff, are the same.
*/

int Get_Luma_DC_dct_diff()
{
	int code, size, dct_diff;

	#ifdef TRACE
	printf("dct_dc_size_luminance: (");
	ML_log("dct_dc_size_luminance: (");
	#endif /* TRACE */

	/* decode length */
	code = Show_Bits(5);

	if (code<31)
	{
		size = DClumtab0[code].val;
		Flush_Buffer(DClumtab0[code].len);
		#ifdef TRACE
		Print_Bits(code,5,DClumtab0[code].len);
		printf("): %d",size);
		ML_log("): %d",size);
		#endif /* TRACE */
	}
	else
	{
		code = Show_Bits(9) - 0x1f0;
		size = DClumtab1[code].val;
		Flush_Buffer(DClumtab1[code].len);

		#ifdef TRACE
		Print_Bits(code+0x1f0,9,DClumtab1[code].len);
		printf("): %d",size);
		ML_log("): %d",size);
		#endif /* TRACE */
	}

	#ifdef TRACE
	printf(", dct_dc_differential (");
	ML_log(", dct_dc_differential (");
	#endif /* TRACE */

	if (size==0)
		dct_diff = 0;
	else
	{
		dct_diff = Get_Bits(size);
		#ifdef TRACE
		Print_Bits(dct_diff,size,size);
		#endif /* TRACE */
		if ((dct_diff & (1<<(size-1)))==0)
			dct_diff-= (1<<size) - 1;
	}

	#ifdef TRACE
	printf("): %d\n",dct_diff);
	ML_log("): %d\n",dct_diff);
	#endif /* TRACE */

	return dct_diff;
}


int Get_Chroma_DC_dct_diff()
{
	int code, size, dct_diff;

	#ifdef TRACE
	printf("dct_dc_size_chrominance: (");
	ML_log("dct_dc_size_chrominance: (");
	#endif /* TRACE */

	/* decode length */
	code = Show_Bits(5);

	if (code<31)
	{
		size = DCchromtab0[code].val;
		Flush_Buffer(DCchromtab0[code].len);

		#ifdef TRACE
		Print_Bits(code,5,DCchromtab0[code].len);
		printf("): %d",size);
		ML_log("): %d",size);
		#endif /* TRACE */
	}
	else
	{
		code = Show_Bits(10) - 0x3e0;
		size = DCchromtab1[code].val;
		Flush_Buffer(DCchromtab1[code].len);

		#ifdef TRACE
		Print_Bits(code+0x3e0,10,DCchromtab1[code].len);
		printf("): %d",size);
		ML_log("): %d",size);
		#endif /* TRACE */
	}

	#ifdef TRACE
	printf(", dct_dc_differential (");
	ML_log(", dct_dc_differential (");
	#endif /* TRACE */

	if (size==0)
		dct_diff = 0;
	else
	{
		dct_diff = Get_Bits(size);
		#ifdef TRACE
		Print_Bits(dct_diff,size,size);
		#endif /* TRACE */
		if ((dct_diff & (1<<(size-1)))==0)
			dct_diff-= (1<<size) - 1;
	}

	#ifdef TRACE
	printf("): %d\n",dct_diff);
	ML_log("): %d\n",dct_diff);
	#endif /* TRACE */

	return dct_diff;
}

}; // namespace MPEG2
