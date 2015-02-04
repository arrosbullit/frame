/*!
	@file		mini_logger.cpp
	@brief		
	@author		Robert Lluís, 2015
	@note		
*/

#include <stdio.h>
#include <stdarg.h>
#include "mini_logger.h"
#include <string>

using namespace std;

static FILE *file;
static string str;

void ML_log(const char* format, ...) 
{
	if(!file){
		return;
	}
	va_list arg;
	va_start(arg, format);
	vfprintf(file, format, arg);	
	va_end(arg);
}

void ML_openFile(const char* fileName, const char* mode)
{
	file = fopen(fileName, mode);
	if(!file){
		printf("Could not open the log_file.\n");
		printf("Press return to continue without a log file.\n");
		getchar();
	}
}

void ML_closeFile()
{
	fclose(file);
}

const char* ML_readLine()
{
	char c;
	int nRead;
	str.clear();
	for(;;){
		nRead = fread(&c, 1, 1, file);
		if(nRead == 0){
			if(str.size()){
				return str.c_str();
			}
			return NULL;
		}
		//printf("%c", c);
		if(c == '\n'){
			if(str.size()){
				return str.c_str();
			}
			return NULL;
		}
		str.push_back(c);
	}
}
