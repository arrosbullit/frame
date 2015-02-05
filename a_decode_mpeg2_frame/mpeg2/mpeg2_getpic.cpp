/*!
	@file		MPEG2_getpic.cpp
	@author		Robert Lluís, december 2014
	@brief		Copied and modified from MPEG
	@note		bits: is set at the module entry and used module wide
				MBA: macroblock address. Macroblock idx in reading order.
	
*/

#include "mpeg2_private.h"
#include "mpeg2_getvlc.h"
#include "mpeg2_getblk.h"
#include "mini_logger.h"
#include <stdio.h>

namespace MPEG2 {

#define Show_Bits(A) bits->ShowBits(A)
#define Flush_Buffer(A) bits->GetBits(A)
#define Get_Bits1() bits->GetBits(1)
#define Get_Bits(A) bits->GetBits(A)

static void picture_data();
static int slice(int MBAmax);
static int start_of_slice(
	int MBAmax,
	int *MBA,
	int *MBAinc,
	int dc_dct_pred[3]);
static int slice_header();
static void Clear_Block (int comp);
static int decode_macroblock(int *macroblock_type, int *dct_type,
		int *dc_dct_pred);
static void motion_compensation(int MBA, int macroblock_type,
								int dct_type);
static void Add_Block(int comp, int bx, int by, int dct_type, int MBA);
static void Saturate(short *Block_Ptr);
static void Write_Frame();
static void YUV_to_RGB(unsigned char yp, unsigned char up, unsigned char vp,
		unsigned char *r, unsigned char *g, unsigned char *b);

static CLittle_Bitstream *bits;
static struct MPEG2_parsed *d;
struct MPEG2_parsed *ld;

void Decode_Picture(CLittle_Bitstream *bitsParam)
{
	bits = bitsParam;
	d = ld = getStruct();

	/* decode picture data ISO/IEC 13818-2 section 6.2.3.7 */
	picture_data();

	/* write or display current or previously decoded reference frame */
	/* ISO/IEC 13818-2 section 6.1.1.11: Frame reordering */
	//frame_reorder(bitstream_framenum, sequence_framenum);
	Write_Frame();
}

/* decode all macroblocks of the current picture */
/* stages described in ISO/IEC 13818-2 section 7 */
static void picture_data()
{
	int MBAmax;
	int ret;

	/* number of macroblocks per picture */
	MBAmax = d->mb_width * d->mb_height;

	if (d->picture_structure!=FRAME_PICTURE)
		MBAmax>>=1; /* field picture has half as mnay macroblocks as frame */

	for(;;)
	{
		if((ret=slice(MBAmax))<0)
			return;
	}

}

// returns -1 when finished all the macroblocks
/* decode all macroblocks of the current picture */
/* ISO/IEC 13818-2 section 6.3.16 */
static int slice(int MBAmax)
{
	int MBA; 
	int MBAinc;
	int macroblock_type, dct_type;
	int dc_dct_pred[3];
	//int dmvector[2];
	//int stwtype, stwclass;
	int ret;

	MBA = 0; /* macroblock address */
	MBAinc = 0;

	if((ret=start_of_slice(MBAmax, &MBA, &MBAinc, dc_dct_pred))!=1)
		return(ret);

	//if (Two_Streams && enhan.scalable_mode==SC_SNR)
	//{
	//	SNRMBA=0;
	//	SNRMBAinc=0;
	//}

	d->Fault_Flag=0;

	for (;;)
	{

		/* this is how we properly exit out of picture */
		if (MBA>=MBAmax)
			return(-1); /* all macroblocks decoded */

#ifdef TRACE
		//if (d->Trace_Flag)
			printf("slice: MB %d\n",MBA);
			ML_log("slice: MBA %d\n",MBA);
#endif /* TRACE */

#ifdef DISPLAY
		if (!progressive_frame && picture_structure==FRAME_PICTURE 
			&& MBA==(MBAmax>>1) && framenum!=0 && Output_Type==T_X11 
			&& !Display_Progressive_Flag)
		{
			Display_Second_Field();
		}
#endif

		//ld = &base;

		if (MBAinc==0)
		{

			if (!bits->ShowBits(23) || d->Fault_Flag) /* next_start_code or fault */
			{
resync: /* if Fault_Flag: resynchronize to next next_start_code */
				d->Fault_Flag = 0;
				return(0);     /* trigger: go to next slice */
			}
			else /* neither next_start_code nor Fault_Flag */
			{
				/* decode macroblock address increment */
				MBAinc = Get_macroblock_address_increment(bits);

				if (d->Fault_Flag) goto resync;
			}
		}

		if (MBA>=MBAmax)
		{
			/* MBAinc points beyond picture dimensions */
			if (!d->Quiet_Flag)
				printf("Too many macroblocks in picture\n");
			return(-1);
		}

		if (MBAinc==1) /* not skipped */
		{
			ret = decode_macroblock(&macroblock_type, &dct_type, dc_dct_pred);

			if(ret==-1)
				return(-1);

			if(ret==0)
				goto resync;

		}
		else /* MBAinc!=1: skipped macroblock */
		{      
			/* ISO/IEC 13818-2 section 7.6.6 */
			// TODO
			//skipped_macroblock(dc_dct_pred, PMV, &motion_type, 
			//	motion_vertical_field_select, &stwtype, &macroblock_type);
		}

	    /* ISO/IEC 13818-2 section 7.6 */
		//TODO load macroblock_type, dct_type
	    motion_compensation(MBA, macroblock_type, dct_type);

		/* advance to next macroblock */
		MBA++;
		MBAinc--;

		if (MBA>=MBAmax)
			return(-1); /* all macroblocks decoded */
	}
}

/* return==-1 means go to next picture */
/* the expression "start of slice" is used throughout the normative
   body of the MPEG specification */
static int start_of_slice(
						  int MBAmax,
						  int *MBA,
						  int *MBAinc,
						  int dc_dct_pred[3]
						  )
{
	unsigned int code;
	int slice_vert_pos_ext;

	d->Fault_Flag = 0;

	next_start_code(bits);
	code = bits->ShowBits(32);

	if (code<SLICE_START_CODE_MIN || code>SLICE_START_CODE_MAX)
	{
		/* only slice headers are allowed in picture_data */
		//if (!d->Quiet_Flag)
			printf("start_of_slice(): Premature end of picture\n");
			ML_log("start_of_slice(): Premature end of picture\n");

		return(-1);  /* trigger: go to next picture */
	}

	bits->GetBits(32); 

	/* decode slice header (may change quantizer_scale) */
	slice_vert_pos_ext = slice_header();

	/* decode macroblock address increment */
	*MBAinc = Get_macroblock_address_increment(bits);

	if (d->Fault_Flag) 
	{
		printf("start_of_slice(): MBAinc unsuccessful\n");
		ML_log("start_of_slice(): MBAinc unsuccessful\n");
		return(0);   /* trigger: go to next slice */
	}

	/* set current location */
	/* NOTE: the arithmetic used to derive macroblock_address below is
	*       equivalent to ISO/IEC 13818-2 section 6.3.17: Macroblock
	*/
	// Robert
	printf("start_of_slice\n");
	printf("  MBAinc %d\n", *MBAinc);
 
	// Log it
	ML_log("start_of_slice\n");
	ML_log("  MBAinc %d\n", *MBAinc);
	   
	*MBA = ((slice_vert_pos_ext<<7) + (code&255) - 1)*d->mb_width + *MBAinc - 1;
	*MBAinc = 1; /* first macroblock in slice: not skipped */

	// Robert  
	printf("  MBA %d\n", *MBA);
	ML_log("  MBA %d\n", *MBA);

	/* reset all DC coefficient and motion vector predictors */
	/* reset all DC coefficient and motion vector predictors */
	/* ISO/IEC 13818-2 section 7.2.1: DC coefficients in intra blocks */
	dc_dct_pred[0]=dc_dct_pred[1]=dc_dct_pred[2]=0;

	/* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
	//PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;
	//PMV[0][1][0]=PMV[0][1][1]=PMV[1][1][0]=PMV[1][1][1]=0;

	/* successfull: trigger decode macroblocks in slice */
	return(1);
}

/* ISO/IEC 13818-2 section 6.2.4 */
int slice_header()
{
	int slice_vertical_position_extension;
	int slice_picture_id_enable = 0;
	int slice_picture_id = 0;
	int extra_information_slice = 0;

	slice_vertical_position_extension =
		(ld->MPEG2_Flag && d->vertical_size>2800) ? bits->GetBits(3) : 0;

	if (ld->scalable_mode==SC_DP)
		ld->priority_breakpoint = bits->GetBits(7);

	bits->GetBits(5); // quantizer_scale_code

	/* slice_id introduced in March 1995 as part of the video corridendum
	(after the IS was drafted in November 1994) */
	if (bits->GetBits(1))
	{
		// TODO Possiblement falti agafar un bit aqu� si comparo aix�
		// amb el pdf de la spec!!!
		bits->GetBits(8);
		// extra_bit_slice. Al codi de MPEG extra_bit_information();
		while (bits->GetBits(1)){
			bits->GetBits(8);
		}	
	}

#ifdef VERBOSE
	printf("slice header\n");
	if (ld->MPEG2_Flag && d->vertical_size>2800)
		printf("  slice_vertical_position_extension=%d\n",slice_vertical_position_extension);
	
	// Log it
	ML_log("slice header\n");
	if (ld->MPEG2_Flag && d->vertical_size>2800)
		ML_log("  slice_vertical_position_extension=%d\n",slice_vertical_position_extension);
#endif /* VERBOSE */

	return slice_vertical_position_extension;
}

/* ISO/IEC 13818-2 section 6.3.17.1: Macroblock modes */
static void macroblock_modes(int *pmacroblock_type, int *pdct_type)
{
	int macroblock_type;
	int dct_type;

	/* get macroblock_type */
	macroblock_type = Get_macroblock_type();

	if (d->Fault_Flag) return;

	/* get dct_type (frame DCT / field DCT) */
	dct_type = (d->picture_structure==FRAME_PICTURE)
		&& (!d->frame_pred_frame_dct)
		&& (macroblock_type & (MACROBLOCK_PATTERN|MACROBLOCK_INTRA))
		? Get_Bits(1)
		: 0;

#ifdef TRACE
	if (d->Trace_Flag  && (d->picture_structure==FRAME_PICTURE)
		&& (!d->frame_pred_frame_dct)
		&& (macroblock_type & (MACROBLOCK_PATTERN|MACROBLOCK_INTRA)))
		printf("dct_type (%d): %s\n",dct_type,dct_type?"Field":"Frame");
#endif /* TRACE */

	/* return values */
	*pmacroblock_type = macroblock_type;
	*pdct_type = dct_type;
}

/* ISO/IEC 13818-2 sections 7.2 through 7.5 */
static int decode_macroblock(int *macroblock_type, int *dct_type, int *dc_dct_pred)
{
	//int macroblock_type_var;
	//int dct_type_var;
	//int *macroblock_type = &macroblock_type_var;
	//int *dct_type = &dct_type_var;

	/* locals */
	int quantizer_scale_code; 
	int comp;

	//int motion_vector_count; 
	//int mv_format; 
	//int dmv; 
	//int mvscale;
	int coded_block_pattern;

	/* ISO/IEC 13818-2 section 6.3.17.1: Macroblock modes */
	macroblock_modes(macroblock_type, dct_type);

	// Robert 
	if(*macroblock_type & MACROBLOCK_INTRA){
		printf("MACROBLOCK_INTRA\n");
		ML_log("MACROBLOCK_INTRA\n");
	}
	if(*macroblock_type & MACROBLOCK_PATTERN){
		printf("MACROBLOCK_PATTERN\n");
		ML_log("MACROBLOCK_PATTERN\n");
	}
	if(*macroblock_type & MACROBLOCK_MOTION_BACKWARD){
		printf("MACROBLOCK_MOTION_BACKWARD\n");
		ML_log("MACROBLOCK_MOTION_BACKWARD\n");
	}
	if(*macroblock_type & MACROBLOCK_MOTION_FORWARD){
		printf("MACROBLOCK_MOTION_FORWARD\n");
		ML_log("MACROBLOCK_MOTION_FORWARD\n");
	}
	if(*macroblock_type & MACROBLOCK_QUANT){
		printf("MACROBLOCK_QUANT\n");
		ML_log("MACROBLOCK_QUANT\n");
	}
	if(*macroblock_type & SPATIAL_TEMPORAL_WEIGHT_CODE_FLAG){
		printf("SPATIAL_TEMPORAL_WEIGHT_CODE_FLAG\n");
		ML_log("SPATIAL_TEMPORAL_WEIGHT_CODE_FLAG\n");
	}
	if(*macroblock_type & PERMITTED_SPATIAL_TEMPORAL_WEIGHT_CLASS){
		printf("PERMITTED_SPATIAL_TEMPORAL_WEIGHT_CLASS\n");
		ML_log("PERMITTED_SPATIAL_TEMPORAL_WEIGHT_CLASS\n");
	}

	if (d->Fault_Flag) return(0);  /* trigger: go to next slice */

	if (*macroblock_type & MACROBLOCK_QUANT)
	{
		quantizer_scale_code = Get_Bits(5);

		#ifdef TRACE
			printf("quantiser_scale_code (");
			ML_log("quantiser_scale_code (");
			Print_Bits(quantizer_scale_code,5,5);
			printf("): %d\n",quantizer_scale_code);
			ML_log("): %d\n",quantizer_scale_code);
		#endif /* TRACE */
	}

	/* motion vectors: FORA */

	if ((*macroblock_type & MACROBLOCK_INTRA) && d->concealment_motion_vectors)
		Flush_Buffer(1); /* remove marker_bit */

	/* macroblock_pattern: Fora pq nom�s accepto intra i quant */
	coded_block_pattern = (*macroblock_type & MACROBLOCK_INTRA) ? 
	(1<<d->block_count)-1 : 0;

	if (d->Fault_Flag) return(0);  /* trigger: go to next slice */

	/* decode blocks */
	for (comp=0; comp<d->block_count; comp++)
	{
		/* SCALABILITY: Data Partitioning */
		//if (base.scalable_mode==SC_DP)
		//	ld = &base;

		Clear_Block(comp);

		if (coded_block_pattern & (1<<(d->block_count-1-comp)))
		{
			if (*macroblock_type & MACROBLOCK_INTRA)
			{
				if (ld->MPEG2_Flag)
					Decode_MPEG2_Intra_Block(comp,dc_dct_pred, bits, d);
				//else
				//	Decode_MPEG1_Intra_Block(comp,dc_dct_pred);
			}
			else
			{
				// Robert: nom�s intra
				d->Fault_Flag = 1;
			}

			if (d->Fault_Flag) return(0);  /* trigger: go to next slice */
		}
	}

	/* successfully decoded macroblock */
	return(1);

} /* decode_macroblock */

/* IMPLEMENTATION: set scratch pad macroblock to zero */
static void Clear_Block(int comp)
{
	short *Block_Ptr;
	int i;

	Block_Ptr = d->block[comp];

	for (i=0; i<64; i++)
		*Block_Ptr++ = 0;
}

/* ISO/IEC 13818-2 section 7.6 */
static void motion_compensation(int MBA, int macroblock_type,
								int dct_type)
{
  int bx, by;
  int comp;
  int i;

  /* derive current macroblock position within picture */
  /* ISO/IEC 13818-2 section 6.3.1.6 and 6.3.1.7 */
  bx = 16*(MBA%d->mb_width);
  by = 16*(MBA/d->mb_width);

  /* copy or add block data into picture */
  for (comp=0; comp<d->block_count; comp++)
  {

    /* MPEG-2 saturation and mismatch control */
    /* base layer could be MPEG-1 stream, enhancement MPEG-2 SNR */
    /* ISO/IEC 13818-2 section 7.4.3 and 7.4.4: Saturation and Mismatch control */
    // TODO pos eso
    if (d->MPEG2_Flag)
      Saturate(ld->block[comp]);

    /* ISO/IEC 13818-2 section Annex A: inverse DCT */
    //if (Reference_IDCT_Flag)
    //Reference_IDCT(ld->block[comp]); // No funciona!
    //else
    //  Fast_IDCT(ld->block[comp]);

    /* ISO/IEC 13818-2 section 7.6.8: Adding prediction and coefficient data */
    Add_Block(comp, bx, by, dct_type, MBA);
  }

}

static void addToThumbnail(int comp, int cc, int pos)
{
	int val;
	// Pseudo IDCT
	val = d->block[comp][0];
	//printf("1 debug val: %d\n", val);
	//val = (int) ((double) val * 0.1231 - 0.3344);
	//printf("2 debug val: %d\n", val);
	// Clipping. Veure més avall.
	//val = d->Clip[val + 128];
	//printf("3 debug val: %d\n", val);
	// Prova map
	if(cc == 0){
		val = (int) ((double) val * 0.06225 + 127.5);
	} else {
		val = (int) ((double) val * 0.06225 + 127.5);
		val = val / 4;
	}
		//val = val * 0.1;
	//printf("5 debug val: %d\n", val);
	// Ho poso a lloc
	d->thumbnail[cc][pos] = val;
	printf("4 pos: %d\n", pos);
}

/* move/add 8x8-Block from block[comp] to backward_reference_frame */
/* copy reconstructed 8x8 block from block[comp] to current_frame[]
 * ISO/IEC 13818-2 section 7.6.8: Adding prediction and coefficient data
 * This stage also embodies some of the operations implied by:
 *   - ISO/IEC 13818-2 section 7.6.7: Combining predictions
 *   - ISO/IEC 13818-2 section 6.1.3: Macroblock
*/
static void Add_Block(int comp, int bx, int by, int dct_type, int MBA)
{
	int cc,i, j;
	//int iincr;
	unsigned char *rfp;
	short *bp;
	int pos;
	int val;
	//unsigned char * current_frame;
	/* derive color component index */
	/* equivalent to ISO/IEC 13818-2 Table 7-1 */
	cc = (comp<4) ? 0 : (comp&1)+1; /* color component index */
	//current_frame = &(d->current_frame[cc][0]);
	if (cc==0)
	{
		/* luminance */

		if (d->picture_structure==FRAME_PICTURE){
			if (dct_type)
			{
				/* field DCT coding */
				//rfp = current_frame[0]
				//      + Coded_Picture_Width*(by+((comp&2)>>1)) + bx + ((comp&1)<<3);
				//iincr = (Coded_Picture_Width<<1) - 8;
				// Robert
				bx = bx >> 3; by = by >> 3;
				pos = (d->Coded_Picture_Width >> 3)*(by+((comp&2)>>1)) + bx + ((comp&1)<<3);
				pos = pos >> 3;
				addToThumbnail(comp, cc, pos);
				// M 'invento el bottom field
				pos = pos + (d->Coded_Picture_Width>>3);
				if(pos < d->thumbnail[cc].size()){
					addToThumbnail(comp, cc, pos);
				}
			}
			else
			{
				/* frame DCT coding */
				//rfp = current_frame[0]
				//					+ Coded_Picture_Width*(by+((comp&2)<<2)) + bx + ((comp&1)<<3);
				//iincr = Coded_Picture_Width - 8;
				// Robert
				bx = bx >> 3; by = by >> 3;
				//printf("comp: %d, (%d, %d)\n", comp, bx, by);
				pos = (d->Coded_Picture_Width >> 3)*(by+((comp&2)>>1)) + bx + ((comp&1));
				addToThumbnail(comp, cc, pos);
			}
		}
		else
		{
			/* field picture */
			//rfp = current_frame[0]
			//					+ (Coded_Picture_Width<<1)*(by+((comp&2)<<2)) + bx + ((comp&1)<<3);
			//iincr = (Coded_Picture_Width<<1) - 8;
			// Robert
			pos = (d->Coded_Picture_Width<<1)*(by+((comp&2)<<2)) + bx +
					((comp&1)<<3);
			pos = pos >> 3;
			addToThumbnail(comp, cc, pos);
			// M 'invento el bottom field
			pos = pos + (d->Coded_Picture_Width>>3);
			if(pos < d->thumbnail[cc].size()){
				addToThumbnail(comp, cc, pos);
			}
		}
	}
	else
	{
		/* chrominance */
		// Robert test
		/*
		if(comp == 4 || comp == 5){
			// Robert
			bx = bx >> 3; by = by >> 3;
			//printf("comp: %d, (%d, %d)\n", comp, bx, by);
			pos = (d->Coded_Picture_Width >> 3)*(by+((comp&2)>>1)) + bx + ((comp&1));
			addToThumbnail(comp, cc, pos);
			addToThumbnail(comp, cc, pos + 1);
			addToThumbnail(comp, cc, pos + (d->Coded_Picture_Width >> 3));
			addToThumbnail(comp, cc, pos + (d->Coded_Picture_Width >> 3) + 1);

		}
		*/

		// scale coordinates
		/*
		if (chroma_format!=CHROMA444){
			bx >>= 1;
		}
		if (chroma_format==CHROMA420){
			by >>= 1;
		}
		if (picture_structure==FRAME_PICTURE)
		{
			if (dct_type && (chroma_format!=CHROMA420))
			{
				// field DCT coding
				rfp = current_frame[cc]
									+ Chroma_Width*(by+((comp&2)>>1)) + bx + (comp&8);
				iincr = (Chroma_Width<<1) - 8;
			}
			else
			{
				// frame DCT coding
				rfp = current_frame[cc]
									+ Chroma_Width*(by+((comp&2)<<2)) + bx + (comp&8);
				iincr = Chroma_Width - 8;
			}
		}
		else
		{
			// field picture
			rfp = current_frame[cc]
								+ (Chroma_Width<<1)*(by+((comp&2)<<2)) + bx + (comp&8);
			iincr = (Chroma_Width<<1) - 8;
		}
		*/
	}

//	bp = ld->block[comp];
//
//	if (addflag)
//	{
//		for (i=0; i<8; i++)
//		{
//			for (j=0; j<8; j++)
//			{
//				*rfp = Clip[*bp++ + *rfp];
//				rfp++;
//			}
//
//			rfp+= iincr;
//		}
//	}
//	else
//	{
//		for (i=0; i<8; i++)
//		{
//			for (j=0; j<8; j++)
//				*rfp++ = Clip[*bp++ + 128];
//
//			rfp+= iincr;
//		}
//	}
}

/* limit coefficients to -2048..2047 */
/* ISO/IEC 13818-2 section 7.4.3 and 7.4.4: Saturation and Mismatch control */
static void Saturate(short *Block_Ptr)
{
  int i, sum, val;

  sum = 0;

   /* ISO/IEC 13818-2 section 7.4.3: Saturation */
  //for (i=0; i<64; i++) // Robert. Només faig el primer
  for (i=0; i<1; i++)
  {
    val = Block_Ptr[i];

    if (val>2047)
      val = 2047;
    else if (val<-2048)
      val = -2048;

    Block_Ptr[i] = val;
    sum+= val;
  }

  /* ISO/IEC 13818-2 section 7.4.4: Mismatch control */
  //if ((sum&1)==0)
  //  Block_Ptr[63]^= 1;

}

/*
	d->thumbnail[cc][pos] = val;

	size = d->Coded_Picture_Width*d->Coded_Picture_Height;

	d->thumbnail[0].resize(size);

*/


void Write_Frame()
{
	FILE *fp= fopen("thumbnail.ppm", "wb"); /* b - binary mode */
	int dimx = d->Coded_Picture_Width >> 3;
	int dimy = d->Coded_Picture_Height >> 3;
	unsigned char r, g ,b;
	fprintf(fp, "P6\n%d %d\n255\n", dimx, dimy);
	int size = dimx * dimy;
	for (unsigned i = 0; i < size; ++i){
		YUV_to_RGB(d->thumbnail[0][i],
				d->thumbnail[1][i],
				d->thumbnail[2][i],
				&r, &g, &b);
		printf("(%d,%d,%d)->(%d,%d,%d)\n",d->thumbnail[0][i],
				d->thumbnail[1][i],
				d->thumbnail[2][i],
				r,g,b);
		//fwrite(&vec[i].color, 1, 3, fp);
		fwrite(&r, 1, 1, fp);
		fwrite(&g, 1, 1, fp);
		fwrite(&b, 1, 1, fp);
	}
	fclose(fp);
}


int Inverse_Table_6_9[8][4]
{
  {117504, 138453, 13954, 34903}, /* no sequence_display_extension */
  {117504, 138453, 13954, 34903}, /* ITU-R Rec. 709 (1990) */
  {104597, 132201, 25675, 53279}, /* unspecified */
  {104597, 132201, 25675, 53279}, /* reserved */
  {104448, 132798, 24759, 53109}, /* FCC */
  {104597, 132201, 25675, 53279}, /* ITU-R Rec. 624-4 System B, G */
  {104597, 132201, 25675, 53279}, /* SMPTE 170M */
  {117579, 136230, 16907, 35559}  /* SMPTE 240M (1987) */
};

void YUV_to_RGB(unsigned char yp, unsigned char up, unsigned char vp,
		unsigned char *r, unsigned char *g, unsigned char *b)
{
	double y, u ,v;
	short tmp;
	y = yp;
	u = up;
	v = vp;
	//http://encyclopedia2.thefreedictionary.com/YUV%2FRGB+conversion+formulas
	tmp = y + 1.14 * v;
	*r = (unsigned char) ((tmp > 255) ? 255 : ((tmp < 0) ? 0 : tmp));
	tmp = y - 0.395 * u - 0.581 * v;
	*g = (unsigned char) ((tmp > 255) ? 255 : ((tmp < 0) ? 0 : tmp));
	tmp = y + 2.032 * u;
	*b = (unsigned char) ((tmp > 255) ? 255 : ((tmp < 0) ? 0 : tmp));
}

void YUV_to_RGB_official(unsigned char yp, unsigned char up, unsigned char vp,
		unsigned char *r, unsigned char *g, unsigned char *b)
{
	int crv, cbu, cgu, cgv;
	int y, u, v;

	/* matrix coefficients */
	crv = Inverse_Table_6_9[d->matrix_coefficients][0];
	cbu = Inverse_Table_6_9[d->matrix_coefficients][1];
	cgu = Inverse_Table_6_9[d->matrix_coefficients][2];
	cgv = Inverse_Table_6_9[d->matrix_coefficients][3];

    u = up - 128;
    v = vp - 128;
    y = 76309 * (yp - 16); /* (255/219)*65536 */
    *r = d->Clip[(y + crv*v + 32768)>>16];
    *g = d->Clip[(y - cgu*u - cgv*v + 32768)>>16];
    *b = d->Clip[(y + cbu*u + 32786)>>16];
}



}; // namespace MPEG2




