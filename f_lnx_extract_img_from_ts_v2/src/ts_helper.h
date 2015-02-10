#ifndef __TS_HELPER_H__
#define __TS_HELPER_H__

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include <stdio.h>

enum btvErrorT {BTV_NO_ERROR = 0, BTV_ERR_GENERIC};

int goToNextPacket(FILE *file);

#endif

