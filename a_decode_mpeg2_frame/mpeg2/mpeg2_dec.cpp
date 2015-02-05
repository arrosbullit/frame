/*!
	@file		mpeg2_dec.cpp
	@author		Robert Lluís, december 2014
	@brief		Copied and modified from MPEG
*/
#include "mpeg2_private.h"
#include "mpeg2_dec.h"
#include "mpeg2_getpic.h"
#include "mini_logger.h"
#include "LittleBitstream2.h"
#include <stdio.h>

namespace MPEG2 {

int Get_Hdr(CLittle_Bitstream *bitsParam);

static void Initialize_Sequence();
static int video_sequence(CLittle_Bitstream *bitsParam);

//static CLittle_Bitstream *bits;

int Decode_Bitstream(CLittle_Bitstream *bitsParam)
{
	int ret;

	// Enable traces	
	getStruct()->Trace_Flag = 1;
	
	ret = Get_Hdr(bitsParam);

	if(ret==1)
	{
		ret = video_sequence(bitsParam);
	}
	
	return(ret);
}

static int video_sequence(CLittle_Bitstream *bitsParam)
{

	Initialize_Sequence();

	/* decode picture whose header has already been parsed in 
	 Decode_Bitstream() */


	Decode_Picture(bitsParam);

	return 0;	
}

/* mostly IMPLEMENTAION specific rouintes */
void Initialize_Sequence()
{
	int cc, size;
	static int Table_6_20[3] = {6,8,12};
	struct MPEG2_parsed *d = getStruct();

	/* force MPEG-1 parameters for proper decoder behavior */
	/* see ISO/IEC 13818-2 section D.9.14 */
	if (!d->MPEG2_Flag)
	{
		d->progressive_sequence = 1;
		d->progressive_frame = 1;
		d->picture_structure = FRAME_PICTURE;
		d->frame_pred_frame_dct = 1;
		d->chroma_format = CHROMA420;
		d->matrix_coefficients = 5;
	}

	/* round to nearest multiple of coded macroblocks */
	/* ISO/IEC 13818-2 section 6.3.3 sequence_header() */
	d->mb_width = (d->horizontal_size+15)/16;
	d->mb_height = (d->MPEG2_Flag && !d->progressive_sequence) ? 2*((d->vertical_size+31)/32)
		: (d->vertical_size+15)/16;

	d->Coded_Picture_Width = 16*d->mb_width;
	d->Coded_Picture_Height = 16*d->mb_height;

	/* ISO/IEC 13818-2 sections 6.1.1.8, 6.1.1.9, and 6.1.1.10 */
	d->Chroma_Width = (d->chroma_format==CHROMA444) ? d->Coded_Picture_Width
		: d->Coded_Picture_Width>>1;
	d->Chroma_Height = (d->chroma_format!=CHROMA420) ? d->Coded_Picture_Height
		: d->Coded_Picture_Height>>1;

	/* derived based on Table 6-20 in ISO/IEC 13818-2 section 6.3.17 */
	d->block_count = Table_6_20[d->chroma_format-1];

	// Robert
	printf("Initialize_Sequence\n");
	printf("  mb_width %d\n", d->mb_width);
	printf("  mb_height %d\n", d->mb_height);
	printf("  Coded_Picture_Width %d\n", d->Coded_Picture_Width);
	printf("  Coded_Picture_Height %d\n", d->Coded_Picture_Height);
	printf("  block_count %d\n", d->block_count);

	// Log it 
	ML_log("Initialize_Sequence\n");
	ML_log("  mb_width %d\n", d->mb_width);
	ML_log("  mb_height %d\n", d->mb_height);
	ML_log("  Coded_Picture_Width %d\n", d->Coded_Picture_Width);
	ML_log("  Coded_Picture_Height %d\n", d->Coded_Picture_Height);
	ML_log("  block_count %d\n", d->block_count);

	/**/
	for (cc=0; cc<3; cc++)
	{
		if (cc==0)
			size = d->Coded_Picture_Width*d->Coded_Picture_Height;
		else
			size = d->Chroma_Width*d->Chroma_Height;

		d->current_frame[cc].resize(size);
	}
	/**/
	/**/
	// Robert
	size = d->Coded_Picture_Width*d->Coded_Picture_Height;
	size = size >> 3;
	d->thumbnail[0].resize(size);
	d->thumbnail[1].resize(size);
	d->thumbnail[2].resize(size);
	/**/
}

/* IMPLEMENTAION specific rouintes */
void Initialize_Decoder()
{
	int i;
	struct MPEG2_parsed *d = getStruct();

	/* Clip table */
	//if (!(Clip=(unsigned char *)malloc(1024)))
	//	Error("Clip[] malloc failed\n");

	if(d->Clip.size() >= 1024){
		return;
	}
	d->Clip.resize(1024);

	unsigned char *Clip = &(d->Clip[0]);
	Clip += 384;

	for (i=-384; i<640; i++){
		Clip[i] = (i<0) ? 0 : ((i>255) ? 255 : i);
	}

	/* IDCT */
	//if (Reference_IDCT_Flag)
	//	Initialize_Reference_IDCT();
	//else
	//	Initialize_Fast_IDCT();

}

}; // namespace MPEG2





