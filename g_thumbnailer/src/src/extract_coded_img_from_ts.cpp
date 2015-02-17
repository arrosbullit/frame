/*!
	@file		extract_coded_image_from_ts.cpp
	@brief		Extracts a coded image from a TS file.
	@author		Robert Lluís, 2015
	@note
*/

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <string.h>
#include <vector>
#include "ts_helper.h"

using namespace std;

#ifndef _WIN32
	typedef long long __int64;
	#define _ftelli64 ftello
	#define _fseeki64 fseeko
#endif

const unsigned TS_PACKET_HEADER_LENGTH = 4;
const unsigned TS_PACKET_SIZE = 188;

#define SEQUENCE_HEADER_CODE    0x1B3
#define PICTURE_START_CODE      0x100

#ifdef TRACE
	#define PRINT(A) printf(A);
#else
	#define PRINT(A)
#endif


using namespace std;

#define STATIC static
//#define STATIC
// Funcs
STATIC int getNextPacketOfTargetPid(unsigned char packet[188]);
STATIC int hasPusi(unsigned char packet[188]);
STATIC void getPayload(unsigned char packet[188], vector<unsigned char> &payload);
STATIC int getNextPayloadOfTargetPid(vector<unsigned char> &payload);
STATIC int appendChunkToBuffer(vector<unsigned char> &buf);
STATIC int goToNextPusi();
STATIC int getSequenceHeaderIdx(vector<unsigned char> &buf);
STATIC int getPicIdx(vector<unsigned char> &buf, unsigned startIdx);
STATIC int getPic(vector<unsigned char> &buf);
// Vars
STATIC unsigned PID;
STATIC FILE *inputFile;

// Appends a chunk of payloads to the buffer argument.
// The size of the chunk will be above 4KB. It cannot be
// exactly 4KB because the size of a payload is variable.
// Returns BTV_NO_ERROR on success
int appendChunkToBuffer(vector<unsigned char> &buf)
{
	int chunkSize = 0;
	int oldBufSize;
	vector<unsigned char > payload;
	int re;
	while(chunkSize < 4096){
		re = getNextPayloadOfTargetPid(payload);
		if(re){
			return re;
		}
		oldBufSize = buf.size();
		buf.resize(buf.size() + payload.size());
		memcpy(&buf[oldBufSize], &payload[0], payload.size());
		chunkSize += payload.size();
	}
	return BTV_NO_ERROR;
}

// Returns 0 if success and copies payload to payload argument
int getNextPayloadOfTargetPid(vector<unsigned char> &payload)
{
	static unsigned char packet[188];
	int re;
	re = getNextPacketOfTargetPid(packet);
	if(re){
		return re;
	}
	getPayload(packet, payload);
	return BTV_NO_ERROR;
}

// Returns the payload in the argument buffer payload.
// If error the size of the payload is zero.
void getPayload(unsigned char packet[188], vector<unsigned char> &payload)
{
	unsigned adapt_field;
	bool hasAdaptationFieldAndPayload;
	unsigned char dataStartPos;
	unsigned sizeToCopy;

	payload.clear();
	adapt_field = (packet[3] & 0x30) >> 4;
	hasAdaptationFieldAndPayload = (adapt_field == 3);
	dataStartPos = TS_PACKET_HEADER_LENGTH;
	if (hasAdaptationFieldAndPayload) {
		dataStartPos += 1 + packet[dataStartPos]; //adaptation_field_len
	}
	if(dataStartPos >= TS_PACKET_SIZE){
		return;
	}
	sizeToCopy = TS_PACKET_SIZE - dataStartPos;
	payload.resize(sizeToCopy);
	memcpy(&payload[0], &packet[dataStartPos], sizeToCopy);
}

// Uses the globals PID and inputFile
// Returns BTV_NO_ERROR or BTV_ERR_GENERIC and fills the buffer argument
int getNextPacketOfTargetPid(unsigned char packet[188])
{
	int re;
	unsigned pid;
	unsigned  adapt_field;
	while(1){
		re = goToNextPacket(inputFile);
		if(re){
			return re;
		}
		fread(packet, 1, 188, inputFile);
		// Discard non target PIDs
		pid = ((packet[1] & 0x1F) << 8) | (packet[2]);
		if(pid != PID){
			continue;
		}
		adapt_field = (packet[3] & 0x30) >> 4;
		// Discard packets in error, scrambled or invalid adapt_filed
		if (((packet[1] & 0x80) != 0) || ((packet[3] & 0xC0) != 0) ||
			(adapt_field == 0)) {
			continue;
		}
		// Discard packets with no paylaod
		if(adapt_field == 2){
			// No data -> nothing to copy
			// The CC does not change -> no CC to update
			continue;
		}
		return BTV_NO_ERROR;
	}
}

// Returns true if PUSI found
int hasPusi(unsigned char packet[188])
{
	return (packet[1] & 0x40);
}

// Moves file pointer to the first target PID packet with PUSI.
// Returns BTV_NO_ERROR on success.
int goToNextPusi()
{
	int re;
	static unsigned char packet[TS_PACKET_SIZE];
	while(1){
		re = getNextPacketOfTargetPid(packet);
		if(re){
			return BTV_ERR_GENERIC;
		}
		if(hasPusi(packet)){
			fseeko(inputFile, -188, SEEK_CUR);
			return BTV_NO_ERROR;
		}
	}
}

// ret idx to start code or -1 if not found
int getSequenceHeaderIdx(vector<unsigned char> &buf)
{
	if(!buf.size()){
		return -1;
	}
	int max = (int) buf.size() - 4;
	for(int i = 0; i < max; i++){
		if(((buf[i]<<24)|(buf[i+1]<<16)|buf[i+2]<<8|buf[i+3]) ==
				SEQUENCE_HEADER_CODE){
			return i;
		}
	}
	return -1;
}

int getPicIdx(vector<unsigned char> &buf, unsigned startIdx)
{
	if(!buf.size() || buf.size() < 4){
		return -1;
	}
	int max = (int) buf.size() - 4;
	for(int i = startIdx; i < max; i++){
		if(((buf[i]<<24)|(buf[i+1]<<16)|buf[i+2]<<8|buf[i+3]) ==
				PICTURE_START_CODE){
			return i;
		}
	}
	return -1;
}

int getPic(vector<unsigned char> &retBuf)
{
	int re;
	int picIdx;
	int seqHeaderIdx;
	vector<unsigned char> buf;
	int maxPusiSearch = 100;
	int pusiCnt = 0;
	retBuf.clear();

	while(1){
		pusiCnt++;
		if(pusiCnt > maxPusiSearch){
			PRINT("Error: reached maximum PUSI search.\n")
			return BTV_ERR_GENERIC;
		}
		buf.clear();
		re = goToNextPusi();
		if(re){
			PRINT("Error by goToNextPusi\n");
			return BTV_ERR_GENERIC;
		}
		re = appendChunkToBuffer(buf);
		if(re){
			PRINT("FAILED Error by appendChunkToBuffer\n");
			return BTV_ERR_GENERIC;
		}
		re = getSequenceHeaderIdx(buf);
		if(re < 0){
			continue;
		}
		seqHeaderIdx = re;
		re = getPicIdx(buf, re);
		if(re < 0){
			PRINT("INFO Error by getPicIdx");
			continue;
		}
		#ifdef TRACE
		printf("Seq header idx: %d\n", seqHeaderIdx);
		#endif
		break;
	}
	// Salto el primer pic code
	picIdx = re;
	while(buf.size() < 1024 * 1024){
		re = getPicIdx(buf, picIdx + 4);
		if(re >= 0){
			int size = re - seqHeaderIdx;
			retBuf.resize(size);
			memcpy(&retBuf[0], &buf[seqHeaderIdx], size);
			return BTV_NO_ERROR;
		}
		re = appendChunkToBuffer(buf);
		if(re){
			PRINT("Errro by appendChunkToBuffer");
			return BTV_ERR_GENERIC;
		}
	}
	PRINT("Error exceeded max size.");
	return BTV_ERR_GENERIC;
}

int getCodedImageFromFile(const char *fileName,
		unsigned pid,
		std::vector<unsigned char> &buf)
{
	int re;
	PID = pid;
	inputFile = fopen(fileName, "rb");
	if(!inputFile){
		return BTV_ERR_GENERIC;
	}
	re = getPic(buf);
	if(re){
		fclose(inputFile);
		return BTV_ERR_GENERIC;
	}
	fclose(inputFile);
	#ifdef TRACE
	printf("Debug coded image size: %d\n", buf.size());
	#endif
	return BTV_NO_ERROR;
}

int openInputFile(const char *fileName)
{
	inputFile = fopen(fileName, "rb");
	if(!inputFile){
		return BTV_ERR_GENERIC;
	}
	return BTV_NO_ERROR;
}

int getCodedImage(unsigned pid,
		std::vector<unsigned char> &buf)
{
	int re;
	PID = pid;
	re = getPic(buf);
	if(re){
		return BTV_ERR_GENERIC;
	}
	#ifdef TRACE
	printf("Debug coded image size: %d\n", buf.size());
	#endif
	return BTV_NO_ERROR;
}

void closeInputFile()
{
	fclose(inputFile);
}

