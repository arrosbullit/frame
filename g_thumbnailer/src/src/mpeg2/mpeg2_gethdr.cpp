/*!
	@file		LittleBitstream2.h
	@author		Robert Lluís, december 2014
	@brief		Based on MPEG
*/

#include <stdio.h>
#include "mpeg2_private.h"
#include "LittleBitstream2.h"
#include "mini_logger.h"

namespace MPEG2 {

/* private prototypes */
static void sequence_header ();
static void group_of_pictures_header ();
static void picture_header ();
static void extension_and_user_data ();
static void sequence_extension ();
static void sequence_display_extension ();
static void quant_matrix_extension ();
static void sequence_scalable_extension ();
static void picture_display_extension ();
static void picture_coding_extension ();
static void picture_spatial_scalable_extension ();
static void picture_temporal_scalable_extension ();
static int  extra_bit_information ();
static void copyright_extension ();
static void user_data ();
/* private variables */
static int Temporal_Reference_Base = 0;
static int True_Framenum_max  = -1;
static int Temporal_Reference_GOP_Reset = 0;
#define RESERVED    -1 
static double frame_rate_Table[16] =
{
	0.0,
	((23.0*1000.0)/1001.0),
	24.0,
	25.0,
	((30.0*1000.0)/1001.0),
	30.0,
	50.0,
	((60.0*1000.0)/1001.0),
	60.0,
	RESERVED,
	RESERVED,
	RESERVED,
	RESERVED,
	RESERVED,
	RESERVED,
	RESERVED
};
static CLittle_Bitstream *bits;
static unsigned char (*scan)[64];
static unsigned char *default_intra_quantizer_matrix;
static void next_start_code();
static void marker_bit(char *text);

/*
* decode headers from one input stream
* until an End of Sequence or picture start code
* is found
*/
int Get_Hdr(CLittle_Bitstream *bitsParam)
{
	unsigned int code;
	bits = bitsParam;
	scan = getScanTable();
	default_intra_quantizer_matrix = getDefaultIntraQuantizerMatrix();
	for (;;)
	{
		/* look for next_start_code */
		next_start_code();
		code = bits->GetBits(32);
		switch (code)
		{
		case SEQUENCE_HEADER_CODE:
			sequence_header();
			break;
		case GROUP_START_CODE:
			group_of_pictures_header();
			break;
		case PICTURE_START_CODE:
			picture_header();
			return 1;
			break;
		case SEQUENCE_END_CODE:
			return 0;
			break;
		default:
			break;
		}
	}
}


/* align to start of next next_start_code */

void next_start_code()
{
	int dbgCnt = 0;
	bits->get_byte_aligned();
	while (bits->ShowBits(24)!=0x01L){
		bits->GetBits(8);
		dbgCnt++;
	}
	#ifdef TRACE
	printf("next_start_code: %d\n", dbgCnt);
	#endif
}


/* decode sequence header */

static void sequence_header()
{
	struct MPEG2_parsed *ld = getStruct();
	struct MPEG2_parsed *d = getStruct();
	
	int i;

	d->horizontal_size_1 = bits->GetBits(12);
	d->vertical_size_1 = bits->GetBits(12);
	bits->GetBits(4);
	bits->GetBits(4);
	bits->GetBits(18);
	marker_bit("sequence_header()");
	bits->GetBits(10);
	bits->GetBits(1);

	if((ld->load_intra_quantizer_matrix = bits->GetBits(1)))
	{
		for (i=0; i<64; i++)
			ld->intra_quantizer_matrix[scan[ZIG_ZAG][i]] = bits->GetBits(8);
	}
	else
	{
		for (i=0; i<64; i++)
			ld->intra_quantizer_matrix[i] = default_intra_quantizer_matrix[i];
	}

	if((ld->load_non_intra_quantizer_matrix = bits->GetBits(1)))
	{
		for (i=0; i<64; i++)
			ld->non_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = bits->GetBits(8);
	}
	else
	{
		for (i=0; i<64; i++)
			ld->non_intra_quantizer_matrix[i] = 16;
	}

	/* copy luminance to chrominance matrices */
	for (i=0; i<64; i++)
	{
		ld->chroma_intra_quantizer_matrix[i] =
			ld->intra_quantizer_matrix[i];

		ld->chroma_non_intra_quantizer_matrix[i] =
			ld->non_intra_quantizer_matrix[i];
	}

#ifdef VERBOSE
	printf("sequence header\n");
	printf("  horizontal_size_1=%d\n",d->horizontal_size_1);
	printf("  vertical_size=%d\n",d->vertical_size_1);
	printf("  load_intra_quantizer_matrix=%d\n",ld->load_intra_quantizer_matrix);
	printf("  load_non_intra_quantizer_matrix=%d\n",ld->load_non_intra_quantizer_matrix);
	// log it
	ML_log("sequence header\n");
	ML_log("  horizontal_size=%d\n",d->horizontal_size_1);
	ML_log("  vertical_size=%d\n",d->vertical_size_1);
	ML_log("  load_intra_quantizer_matrix=%d\n",ld->load_intra_quantizer_matrix);
	ML_log("  load_non_intra_quantizer_matrix=%d\n",ld->load_non_intra_quantizer_matrix);
#endif	

	extension_and_user_data();

}

/* decode group of pictures header */
/* ISO/IEC 13818-2 section 6.2.2.6 */
static void group_of_pictures_header()
{
    ML_log("group of pictures\n");
	// M'ho peto tot
	extension_and_user_data();
}


/* decode picture header */

/* ISO/IEC 13818-2 section 6.2.3 */
static void picture_header()
{
	struct MPEG2_parsed *d = getStruct();
	d->temporal_reference  = bits->GetBits(10);
	d->picture_coding_type = bits->GetBits(3);
	
#ifdef VERBOSE
	printf("picture header\n");
	printf("  temporal_reference=%d\n",d->temporal_reference);
	printf("  picture_coding_type=%d\n",d->picture_coding_type);
	
	// Log it
	ML_log("picture header\n");
	ML_log("  temporal_reference=%d\n",d->temporal_reference);
	ML_log("  picture_coding_type=%d\n",d->picture_coding_type);
#endif	

	extension_and_user_data();
}


/* decode extension and user data */
/* ISO/IEC 13818-2 section 6.2.2.2 */
static void extension_and_user_data()
{
	int code,ext_ID;

	next_start_code();

	while ((code = bits->ShowBits(32))==EXTENSION_START_CODE || code==USER_DATA_START_CODE)
	{
		if (code==EXTENSION_START_CODE)
		{
			bits->GetBits(32);
			ext_ID = bits->GetBits(4);
			switch (ext_ID)
			{
			case SEQUENCE_EXTENSION_ID:
				sequence_extension();
				break;
			case SEQUENCE_DISPLAY_EXTENSION_ID:
				sequence_display_extension();
				break;
			case QUANT_MATRIX_EXTENSION_ID:
				quant_matrix_extension();
				break;
			case SEQUENCE_SCALABLE_EXTENSION_ID:
				sequence_scalable_extension();
				break;
			case PICTURE_DISPLAY_EXTENSION_ID:
				picture_display_extension();
				break;
			case PICTURE_CODING_EXTENSION_ID:
				picture_coding_extension();
				break;
			case PICTURE_SPATIAL_SCALABLE_EXTENSION_ID:
				picture_spatial_scalable_extension();
				break;
			case PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID:
				picture_temporal_scalable_extension();
				break;
			case COPYRIGHT_EXTENSION_ID:
				copyright_extension();
				break;
			default:
				break;
			}
			next_start_code();
		}
		else
		{
			bits->GetBits(32);
			ML_log("user data\n");
			user_data();
		}
	}
}


/* decode sequence extension */

/* ISO/IEC 13818-2 section 6.2.2.3 */
static void sequence_extension()
{
	struct MPEG2_parsed *d = getStruct();

	d->MPEG2_Flag = 1;

	d->scalable_mode = SC_NONE; /* unless overwritten by sequence_scalable_extension() */
	//d->layer_id = 0;                /* unless overwritten by sequence_scalable_extension() */

	d->profile_and_level_indication = bits->GetBits(8);
	d->progressive_sequence         = bits->GetBits(1);
	d->chroma_format                = bits->GetBits(2);
	d->horizontal_size_extension    = bits->GetBits(2);
	d->vertical_size_extension      = bits->GetBits(2);

	/* special case for 422 profile & level must be made */
	if((d->profile_and_level_indication>>7) & 1)
	{  /* escape bit of profile_and_level_indication set */

		/* 4:2:2 Profile @ Main Level */
		if((d->profile_and_level_indication&15)==5)
		{
			d->profile = PROFILE_422;
			d->level   = MAIN_LEVEL;  
		}
	}
	else
	{
		d->profile = d->profile_and_level_indication >> 4;  /* Profile is upper nibble */
		d->level   = d->profile_and_level_indication & 0xF;  /* Level is lower nibble */
	}

	d->horizontal_size = (d->horizontal_size_extension<<12) | 
		(d->horizontal_size_1&0x0fff);
	d->vertical_size = (d->vertical_size_extension<<12) | 
		(d->vertical_size_1&0x0fff);

#ifdef TRACE
	printf("sequence extension\n");
	printf("  profile_and_level_indication=%d\n",d->profile_and_level_indication);

	if (d->profile_and_level_indication<128)
	{
		printf("    profile=%d, level=%d\n",d->profile, d->level);
	}

	printf("  progressive_sequence=%d\n",d->progressive_sequence);
	printf("  chroma_format=%d\n",d->chroma_format);
	printf("  horizontal_size_extension=%d\n",d->horizontal_size_extension);
	printf("  vertical_size_extension=%d\n",d->vertical_size_extension);

	// Log it
	
	ML_log("sequence extension\n");
	ML_log("  profile_and_level_indication=%d\n",d->profile_and_level_indication);

	if (d->profile_and_level_indication<128)
	{
		ML_log("    profile=%d, level=%d\n",d->profile, d->level);
	}

	ML_log("  progressive_sequence=%d\n",d->progressive_sequence);
	ML_log("  chroma_format=%d\n",d->chroma_format);
	ML_log("  horizontal_size_extension=%d\n",d->horizontal_size_extension);
	ML_log("  vertical_size_extension=%d\n",d->vertical_size_extension);
	
#endif		
}


/* decode sequence display extension */

static void sequence_display_extension()
{

	struct MPEG2_parsed *d = getStruct();

	d->video_format      = bits->GetBits(3);
	d->color_description = bits->GetBits(1);

	if (d->color_description)
	{
		d->color_primaries          = bits->GetBits(8);
		d->transfer_characteristics = bits->GetBits(8);
		d->matrix_coefficients      = bits->GetBits(8);
	}

	d->display_horizontal_size = bits->GetBits(14);
	marker_bit("sequence_display_extension");
	d->display_vertical_size   = bits->GetBits(14);

#ifdef TRACE
	printf("sequence display extension\n");

	printf("  video_format=%d\n",d->video_format);
	printf("  color_description=%d\n",d->color_description);

	if (d->color_description)
	{
		printf("    color_primaries=%d\n",d->color_primaries);
		printf("    transfer_characteristics=%d\n",d->transfer_characteristics);
		printf("    matrix_coefficients=%d\n",d->matrix_coefficients);
	}
	printf("  display_horizontal_size=%d\n",d->display_horizontal_size);
	printf("  display_vertical_size=%d\n",d->display_vertical_size);
	
	// Log it
	
	ML_log("sequence display extension\n");

	ML_log("  video_format=%d\n",d->video_format);
	ML_log("  color_description=%d\n",d->color_description);

	if (d->color_description)
	{
		ML_log("    color_primaries=%d\n",d->color_primaries);
		ML_log("    transfer_characteristics=%d\n",d->transfer_characteristics);
		ML_log("    matrix_coefficients=%d\n",d->matrix_coefficients);
	}
	ML_log("  display_horizontal_size=%d\n",d->display_horizontal_size);
	ML_log("  display_vertical_size=%d\n",d->display_vertical_size);
#endif /* VERBOSE */
}


/* decode quant matrix entension */
/* ISO/IEC 13818-2 section 6.2.3.2 */
static void quant_matrix_extension()
{
	int i;

	struct MPEG2_parsed *ld = getStruct();
	struct MPEG2_parsed *d = getStruct();

	if((ld->load_intra_quantizer_matrix = bits->GetBits(1)))
	{
		for (i=0; i<64; i++)
		{
			ld->chroma_intra_quantizer_matrix[scan[ZIG_ZAG][i]]
			= ld->intra_quantizer_matrix[scan[ZIG_ZAG][i]]
			= bits->GetBits(8);
		}
	}

	if((ld->load_non_intra_quantizer_matrix = bits->GetBits(1)))
	{
		for (i=0; i<64; i++)
		{
			ld->chroma_non_intra_quantizer_matrix[scan[ZIG_ZAG][i]]
			= ld->non_intra_quantizer_matrix[scan[ZIG_ZAG][i]]
			= bits->GetBits(8);
		}
	}

	if((ld->load_chroma_intra_quantizer_matrix = bits->GetBits(1)))
	{
		for (i=0; i<64; i++)
			ld->chroma_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = bits->GetBits(8);
	}

	if((ld->load_chroma_non_intra_quantizer_matrix = bits->GetBits(1)))
	{
		for (i=0; i<64; i++)
			ld->chroma_non_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = bits->GetBits(8);
	}
#ifdef TRACE
    printf("quant matrix extension \n");
    printf("  load_intra_quantizer_matrix=%d\n",
      ld->load_intra_quantizer_matrix);
    printf("  load_non_intra_quantizer_matrix=%d\n",
      ld->load_non_intra_quantizer_matrix);
    printf("  load_chroma_intra_quantizer_matrix=%d\n",
      ld->load_chroma_intra_quantizer_matrix);
    printf("  load_chroma_non_intra_quantizer_matrix=%d\n",
      ld->load_chroma_non_intra_quantizer_matrix);
#endif 

}


/* decode sequence scalable extension */
/* ISO/IEC 13818-2   section 6.2.2.5 */
static void sequence_scalable_extension()
{
	struct MPEG2_parsed *ld = getStruct();
	
	/* values (without the +1 offset) of scalable_mode are defined in 
	Table 6-10 of ISO/IEC 13818-2 */
	ld->scalable_mode = bits->GetBits(2) + 1; /* add 1 to make SC_DP != SC_NONE */
#ifdef TRACE
	printf("sequence scalable extension\n");
	printf("  scalable_mode=%d\n",ld->scalable_mode-1);
#endif
}


/* decode picture display extension */
/* ISO/IEC 13818-2 section 6.2.3.3. */
static void picture_display_extension()
{
}


/* decode picture coding extension */
static void picture_coding_extension()
{
	struct MPEG2_parsed *ld = getStruct();
	struct MPEG2_parsed *d = getStruct();

	// f_codes ignorats
	bits->GetBits(4);
	bits->GetBits(4);
	bits->GetBits(4);
	bits->GetBits(4);

	d->intra_dc_precision         = bits->GetBits(2); 
	d->picture_structure          = bits->GetBits(2); 
	d->top_field_first            = bits->GetBits(1);
	d->frame_pred_frame_dct       = bits->GetBits(1);
	d->concealment_motion_vectors = bits->GetBits(1);
	ld->q_scale_type				= bits->GetBits(1);
	d->intra_vlc_format           = bits->GetBits(1);
	ld->alternate_scan				= bits->GetBits(1);
	d->repeat_first_field         = bits->GetBits(1);
	d->chroma_420_type            = bits->GetBits(1);
	d->progressive_frame          = bits->GetBits(1);
#ifdef TRACE
	printf("picture coding extension\n");
	printf("  intra_dc_precision=%d\n",d->intra_dc_precision);
	printf("  picture_structure=%d\n",d->picture_structure);
	printf("  top_field_first=%d\n",d->top_field_first);
	printf("  frame_pred_frame_dct=%d\n",d->frame_pred_frame_dct);
	printf("  concealment_motion_vectors=%d\n",d->concealment_motion_vectors);
	printf("  q_scale_type=%d\n",ld->q_scale_type);
	printf("  intra_vlc_format=%d\n",d->intra_vlc_format);
	printf("  alternate_scan=%d\n",ld->alternate_scan);
	printf("  repeat_first_field=%d\n",d->repeat_first_field);
	printf("  chroma_420_type=%d\n",d->chroma_420_type);
	printf("  progressive_frame=%d\n",d->progressive_frame);

	// Log it
	
	ML_log("picture coding extension\n");
	ML_log("  intra_dc_precision=%d\n",d->intra_dc_precision);
	ML_log("  picture_structure=%d\n",d->picture_structure);
	ML_log("  top_field_first=%d\n",d->top_field_first);
	ML_log("  frame_pred_frame_dct=%d\n",d->frame_pred_frame_dct);
	ML_log("  concealment_motion_vectors=%d\n",d->concealment_motion_vectors);
	ML_log("  q_scale_type=%d\n",ld->q_scale_type);
	ML_log("  intra_vlc_format=%d\n",d->intra_vlc_format);
	ML_log("  alternate_scan=%d\n",ld->alternate_scan);
	ML_log("  repeat_first_field=%d\n",d->repeat_first_field);
	ML_log("  chroma_420_type=%d\n",d->chroma_420_type);
	ML_log("  progressive_frame=%d\n",d->progressive_frame);
	
#endif	
}


/* decode picture spatial scalable extension */
/* ISO/IEC 13818-2 section 6.2.3.5. */
static void picture_spatial_scalable_extension()
{
	struct MPEG2_parsed *ld = getStruct();

	ld->pict_scal = 1; /* use spatial scalability in this picture */
#ifdef TRACE
	printf("picture spatial scalable extension\n");
#endif
}


/* decode picture temporal scalable extension
*
* not implemented
*/
/* ISO/IEC 13818-2 section 6.2.3.4. */
static void picture_temporal_scalable_extension()
{
}


/* decode extra bit information */
/* ISO/IEC 13818-2 section 6.2.3.4. */
static int extra_bit_information()
{
	int Byte_Count = 0;

	while (bits->GetBits(1))
	{
		bits->GetBits(8);
		Byte_Count++;
	}

	return(Byte_Count);
}

/* ISO/IEC 13818-2 section 5.3 */
/* Purpose: this function is mainly designed to aid in bitstream conformance
testing.  A simple bits->GetBits(1) would do */
void marker_bit(char *text)
{
	if(bits->GetBits(1) == 0){
	#ifdef TRACE
		printf("Error marker bit\n");
	#endif
	throw "Error marker bit";
	}
}

/* ISO/IEC 13818-2  sections 6.3.4.1 and 6.2.2.2.2 */
static void user_data()
{
	/* skip ahead to the next start code */
	next_start_code();
}



/* Copyright extension */
/* ISO/IEC 13818-2 section 6.2.3.6. */
/* (header added in November, 1994 to the IS document) */


static void copyright_extension()
{
}




}; // using namespace MPEG2
