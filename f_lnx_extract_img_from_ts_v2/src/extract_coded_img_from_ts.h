/*!
	@file		extract_coded_image_from_ts.cpp
	@brief		Extracts a coded image from a TS file.
	@author		Robert Lluís, 2015
	@note
*/

#ifndef EXTRACT_CODED_IMG_FROM_TS_H
#define EXTRACT_CODED_IMG_FROM_TS_H

#include <stdio.h>
#include <vector>


// Returns 0 if success
int getCodedImageFromFile(const char *fileName,
		unsigned pid,
		std::vector<unsigned char> &buf);


#endif
