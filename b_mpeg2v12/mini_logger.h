/*!
	@file		mini_logger.h
	@brief		
	@author		Robert Lluís, 2015
	@note		
*/
#ifndef __MINI_LOGGER_H__
#define __MINI_LOGGER_H__

#ifdef __cplusplus
extern "C" {
#endif

void ML_log(const char* format, ...);

void ML_openFile(const char* fileName, const char* mode);

void ML_closeFile();

const char* ML_readLine();

#ifdef __cplusplus
}
#endif

#endif
