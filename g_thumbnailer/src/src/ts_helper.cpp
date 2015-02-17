#include "ts_helper.h"

#ifndef _WIN32
	typedef long long __int64;
	#define _ftelli64 ftello
	#define _fseeki64 fseeko
#endif
/*!
	@note advances the read/write pointer of the file
	to the sync byte of a ts packet
	@returns BTV_NO_ERROR if ok
	
*/
int goToNextSyncByte(FILE *file)
{
	unsigned char byte = 0;
	unsigned bytesRead;

	while(byte != 0x47){
		bytesRead = fread(&byte, 1, 1, file);
		if(bytesRead == 0){
			return BTV_ERR_GENERIC;
		}
	}
	//fseek(file, -1, SEEK_CUR);
	_fseeki64(file, -1, SEEK_CUR);
	return BTV_NO_ERROR;
}

/*!
	@note advances the read/write pointer of the file
	to the sync byte of a ts packet and makes sure
	a full packet is there 
	@returns BTV_NO_ERROR if ok
	
*/
int goToNextPacket(FILE *file)
{
	unsigned char byte = 0;
	unsigned bytesRead;
	unsigned char tsPacket[189];
	/*
	static unsigned dbgCounter = 0;
	if(true){
		if(dbgCounter % 100 == 0){
			printf("%d TSP\n", dbgCounter);
		}
		dbgCounter++;
	}
	*/
	while(1){
		//Find the next sync byte
		if(goToNextSyncByte(file) != BTV_NO_ERROR){
			return BTV_ERR_GENERIC;
		}
		//Read the complete ts packet
		bytesRead = fread(tsPacket, 1, 189, file);
		if(bytesRead < 188){
			//Not a full packet
			printf("End of file reached\n");
			return BTV_ERR_GENERIC;
		}
		if(bytesRead == 188){
			//This is the last packet of the file	
			//fseek(file, -188, SEEK_CUR);
			_fseeki64(file, -188, SEEK_CUR);
			return BTV_NO_ERROR;
		}
		//check if I am really synched
		if(tsPacket[188] == 0x47){
			//yes, i am synched
			//fseek(file, -189, SEEK_CUR);
			_fseeki64(file, -189, SEEK_CUR);
			return BTV_NO_ERROR;
		}
		printf("Lost sync\n");
		//If I am not synched go search the next sync byte
	}
}
