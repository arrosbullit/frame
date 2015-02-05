/*!
	@file		mpeg2_private.h
	@author		Robert Lluís, december 2014
	@brief		Copied and modified from MPEG
*/
#ifndef __MPEG2_DECODER_PRIVATE_H__
#define __MPEG2_DECODER_PRIVATE_H__

#include <vector>
#include "LittleBitstream2.h"

namespace MPEG2 {

#define ERROR (-1)

#define PICTURE_START_CODE      0x100
#define SLICE_START_CODE_MIN    0x101
#define SLICE_START_CODE_MAX    0x1AF
#define USER_DATA_START_CODE    0x1B2
#define SEQUENCE_HEADER_CODE    0x1B3
#define SEQUENCE_ERROR_CODE     0x1B4
#define EXTENSION_START_CODE    0x1B5
#define SEQUENCE_END_CODE       0x1B7
#define GROUP_START_CODE        0x1B8
#define SYSTEM_START_CODE_MIN   0x1B9
#define SYSTEM_START_CODE_MAX   0x1FF

#define ISO_END_CODE            0x1B9
#define PACK_START_CODE         0x1BA
#define SYSTEM_START_CODE       0x1BB

#define VIDEO_ELEMENTARY_STREAM 0x1e0

/* scalable_mode */
#define SC_NONE 0
#define SC_DP   1
#define SC_SPAT 2
#define SC_SNR  3
#define SC_TEMP 4

/* picture coding type */
#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3
#define D_TYPE 4

/* picture structure */
#define TOP_FIELD     1
#define BOTTOM_FIELD  2
#define FRAME_PICTURE 3

/* macroblock type */
#define MACROBLOCK_INTRA                        1
#define MACROBLOCK_PATTERN                      2
#define MACROBLOCK_MOTION_BACKWARD              4
#define MACROBLOCK_MOTION_FORWARD               8
#define MACROBLOCK_QUANT                        16
#define SPATIAL_TEMPORAL_WEIGHT_CODE_FLAG       32
#define PERMITTED_SPATIAL_TEMPORAL_WEIGHT_CLASS 64

/* motion_type */
#define MC_FIELD 1
#define MC_FRAME 2
#define MC_16X8  2
#define MC_DMV   3

/* mv_format */
#define MV_FIELD 0
#define MV_FRAME 1

/* chroma_format */
#define CHROMA420 1
#define CHROMA422 2
#define CHROMA444 3

/* extension start code IDs */

#define SEQUENCE_EXTENSION_ID                    1
#define SEQUENCE_DISPLAY_EXTENSION_ID            2
#define QUANT_MATRIX_EXTENSION_ID                3
#define COPYRIGHT_EXTENSION_ID                   4
#define SEQUENCE_SCALABLE_EXTENSION_ID           5
#define PICTURE_DISPLAY_EXTENSION_ID             7
#define PICTURE_CODING_EXTENSION_ID              8
#define PICTURE_SPATIAL_SCALABLE_EXTENSION_ID    9
#define PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID  10

#define ZIG_ZAG                                  0

#define PROFILE_422                             (128+5)
#define MAIN_LEVEL                              8

/* Layers: used by Verbose_Flag, Verifier_Flag, Stats_Flag, and Trace_Flag */
#define NO_LAYER                                0
#define SEQUENCE_LAYER                          1
#define PICTURE_LAYER                           2
#define SLICE_LAYER                             3    
#define MACROBLOCK_LAYER                        4    
#define BLOCK_LAYER                             5
#define EVENT_LAYER                             6
#define ALL_LAYERS                              7

#define MB_WEIGHT                  32
#define MB_CLASS4                  64

struct MPEG2_parsed
{
	/* non-normative variables derived from normative elements */
	int Coded_Picture_Width;
	int Coded_Picture_Height;
	int Chroma_Width;
	int Chroma_Height;
	int block_count;
	int Second_Field;
	int profile, level;

	/* normative derived variables (as per ISO/IEC 13818-2) */
	int horizontal_size;
	int vertical_size;
	int mb_width;
	int mb_height;
	double bit_rate;
	double frame_rate; 

	/* ISO/IEC 13818-2 section 6.2.2.1:  sequence_header() */
	// Poso _1 per a deixar intuir que n'hi ha m�s
	int horizontal_size_1;
	int vertical_size_1;

	/* ISO/IEC 13818-2 section 6.2.2.3:  sequence_extension() */
	int profile_and_level_indication;
	int progressive_sequence;
	int chroma_format;
	int horizontal_size_extension;
	int vertical_size_extension;
	
	/* ISO/IEC 13818-2 section 6.2.2.4:  sequence_display_extension() */
	int video_format;  
	int color_description;
	int color_primaries;
	int transfer_characteristics;
	int matrix_coefficients;
	int display_horizontal_size;
	int display_vertical_size;

	/* ISO/IEC 13818-2 section 6.2.3: picture_header() */
	int temporal_reference;
	int picture_coding_type;

	/* ISO/IEC 13818-2 section 6.2.3.1: picture_coding_extension() header */
	int intra_dc_precision;
	int picture_structure;
	int top_field_first;
	int frame_pred_frame_dct;
	int concealment_motion_vectors;
	int intra_vlc_format;
	int repeat_first_field;
	int chroma_420_type;
	int progressive_frame;

	// Layer_Data. Former struct layer_data!!
	// Algunes d'aquestes dades surten de parsejar 
	// dir�tament van a parar aqu� enlloc d'anar 
	// als grups de variables d'aqu� a dalt.
	/* sequence header and quant_matrix_extension() */
	int intra_quantizer_matrix[64];
	int non_intra_quantizer_matrix[64];
	int chroma_intra_quantizer_matrix[64];
	int chroma_non_intra_quantizer_matrix[64];

	int load_intra_quantizer_matrix;
	int load_non_intra_quantizer_matrix;
	int load_chroma_intra_quantizer_matrix;
	int load_chroma_non_intra_quantizer_matrix;

	int MPEG2_Flag;
	/* sequence scalable extension */
	int scalable_mode;
	/* picture coding extension */
	int q_scale_type;
	int alternate_scan;
	/* picture spatial scalable extension */
	int pict_scal;
	/* slice/macroblock */
	int priority_breakpoint;
	int quantizer_scale;
	int intra_slice;
	short block[12][64];
		
	// More stuff. TODO: Mirar on alliberes aquesta mem
	std::vector<unsigned char> current_frame[3];
	std::vector<unsigned char> thumbnail[3];
	std::vector<unsigned char> Clip;
	
	/* decoder operation control flags */
	int Quiet_Flag;
	int Trace_Flag;
	int Fault_Flag;
	int Verbose_Flag;
};

struct MPEG2_parsed * getStruct();
void next_start_code(CLittle_Bitstream *bits);
unsigned char (*getScanTable())[64];
unsigned char *getDefaultIntraQuantizerMatrix();
void Print_Bits(int code, int bits, int len);

}; // namespace MPEG2

#endif
