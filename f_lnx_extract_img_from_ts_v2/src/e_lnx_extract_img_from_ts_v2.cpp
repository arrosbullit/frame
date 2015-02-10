//============================================================================
// Name        : e_lnx_extract_img_from_ts_v2.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

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

const char TARGET_FILENAME[] =
	"/media/sf_win_robert/Streams/test_suite_dc_at_11Mbps_in_mux_at_31Mbps.ts";

const unsigned TS_PACKET_HEADER_LENGTH = 4;
const unsigned TS_PACKET_SIZE = 188;

#define SEQUENCE_HEADER_CODE    0x1B3
#define PICTURE_START_CODE      0x100

#ifdef DEBUG
	#define PRINT(A) printf(A);
#else
	#define PRINT(A)
#endif

static __int64 getFileSize(FILE* inputFile);
static int getNextPacketOfTargetPid(unsigned char packet[188]);
static int hasPusi(unsigned char packet[188]);
static void getPayload(unsigned char packet[188], vector<unsigned char> &payload);
static int getNextPayloadOfTargetPid(vector<unsigned char> &payload);
static int appendChunkToBuffer(vector<unsigned char> &buf);
static int goToNextPusi();
static int getSequenceHeaderIdx(vector<unsigned char> &buf);
static int getPicIdx(vector<unsigned char> &buf, unsigned startIdx);
static int getPic(vector<unsigned char> &buf);
static int savePicToFile(vector<unsigned char> &buf);


using namespace std;

unsigned PID = 0x02d0;
FILE *inputFile;

int main() {
	__int64 fileSize;
	int re;
	unsigned char packet[188];
	vector<unsigned char> payload;
	inputFile = fopen(TARGET_FILENAME, "rb");
	if(!inputFile){
		printf("Error by fopen. Press Return.\n");
		getchar();
		return 0;
	}
	fileSize = getFileSize(inputFile);
	printf("File size %lld bytes, %lld packets\n", fileSize,
			fileSize/TS_PACKET_SIZE);
	// Test goToNextPacket
	printf("Test goToNextPacket()\n");
	re = goToNextPacket(inputFile);
	if(re){
		printf("Error by goToNextPacket()\n");
	} else {
		printf("SUCCESS goToNextPacket()\n");
	}
	// Test getNextPacketOfTargetPid
	printf("Test getNextPacketOfTargetPid\n");
	re = getNextPacketOfTargetPid(packet);
	if(re){
		printf("Error by getNextPacketOfTargetPid\n");
	} else {
		printf("SUCCESS getNextPacketOfTargetPid()\n");
	}
	// Test hasPusi
	printf("Test hasPusi()\n");
	while(1){
		re = getNextPacketOfTargetPid(packet);
		if(re){
			printf("Error by getNextPacketOfTargetPid()\n");
			break;
		}
		if(hasPusi(packet)){
			printf("PUSI found\n");
			printf("SUCCESS hasPusi\n");
			break;
		}
	}
	// Test getPayload
	printf("Test getPayload\n");
	do{
		re = getNextPacketOfTargetPid(packet);
		if(re){
			printf("2 Error by getNextPacketOfTargetPid()\n");

		}
		getPayload(packet, payload);
		if(!payload.size()){
			printf("Error by getPayload()\n");
		} else {
			printf("SUCCESS getPayload()\n");
		}
	}while(0);
	// Test goToNextPusi()
	printf("Test goToNextPusi()\n");
	do{
		printf("Call goToNextPusi()\n");
		re = goToNextPusi();
		if(re){
			printf("FAILED test goToNextPusi(). Error by goToNextPusi.\n");
			break;
		}
		printf("Call getNextPacketOfTargetPid()\n");
		re = getNextPacketOfTargetPid(packet);
		if(re){
			printf("FAILED test goToNextPusi(). Error by getNextPacketOfTargetPid.\n");
			break;
		}
		if(!hasPusi(packet)){
			printf("FAILED test goToNextPusi(). PUSI not found.\n");
			break;
		}
		printf("SUCCESS goToNextPussi\n");

	} while(0);
	// Test appendChunkToBuffer
	printf("Test appendChunkToBuffer\n");
	vector<unsigned char> bigBuf;
	re = appendChunkToBuffer(bigBuf);
	if(re){
		printf("FAILED appendChunkToBuffer(bigBuf)\n");
	} else {
		printf("Buf size: %lu\n", bigBuf.size());
		if(bigBuf.size() < 4096){
			printf("FAILED appendChunkToBuffer(). Error in buf size.\n");
		} else {
			printf("SUCCESS appendChunkToBffer()\n");
		}
	}
	// Test getSequenceHeaderIdx
	printf("Test getSequenceHeaderIdx\n");
	do{
		re = goToNextPusi();
		if(re){
			printf("FAILED Error by goToNextPusi.\n");
			break;
		}
		bigBuf.clear();
		re = appendChunkToBuffer(bigBuf);
		if(re){
			printf("FAILED Error by appendChunkToBuffer(bigBuf).\n");
			break;
		}
		re = getSequenceHeaderIdx(bigBuf);
		if(re < 0){
			printf("FAILED Error by getSequenceHeaderIdx");
			break;
		}
		printf("Index of seq header in buf: %d\n", re);
		printf("SUCCESS getSequenceHeaderIdx!!\n");

	} while(0);
	// Test getPicIdx
	// This test reuses the previous bibBuf which must be filled with data
	re = getPicIdx(bigBuf, 0);
	if(re < 0){
		printf("FAILED getPicIdx\n");
	} else {
		printf("Index of pic start: %d\n", re);
		printf("SUCCESS getPicIdx\n");
	}
	// Test get pic
	re = getPic(bigBuf);
	if(re){
		printf("FAILED getPic\n");
	} else {
		printf("Pic size: %d\n", bigBuf.size());
		printf("SUCCEEDED getPic\n");
		savePicToFile(bigBuf);
	}
	// Test all pic sizes
	do{
		re = getPic(bigBuf);
		if(!re){
			printf("Pic size: %d\n", bigBuf.size());
		}
	}while(!re);

	// The End
	printf("Press return to finish\n");
	getchar();
	printf("Bye!");
	return 0;
}

__int64 getFileSize(FILE* inputFile)
{
	__int64 fileSize;
	__int64 savedPos;
	savedPos = _ftelli64(inputFile);
	_fseeki64(inputFile, 0, SEEK_END);
	fileSize =  _ftelli64(inputFile);
	_fseeki64(inputFile, savedPos, SEEK_SET);
	return fileSize;
}

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
	int firstPicIdx;
	int secondPicIdx;
	int seqHeaderIdx;
	vector<unsigned char> buf;
 	retBuf.clear();
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
		PRINT("FAILED Error by getSequenceHeaderIdx");
		return BTV_ERR_GENERIC;
	}
	seqHeaderIdx = re;
	re = getPicIdx(buf, re);
	if(re < 0){
		PRINT("FAILED Error by getPicIdx");
		return BTV_ERR_GENERIC;
	}
	// Salto el primer pic code
	firstPicIdx = re;
	while(buf.size() < 1024 * 1024){
		re = getPicIdx(buf, firstPicIdx + 4);
		if(re >= 0){
			//printf("Pic code idx: %d\n", re);
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

int savePicToFile(vector<unsigned char> &buf){
	FILE *outFile;
	outFile = fopen("extracted_pic.m2v", "wb");
	if(!outFile){
		PRINT("Cound not open out file\n");
		return BTV_ERR_GENERIC;
	}
	int written = fwrite(&buf[0], 1, buf.size(), outFile);
	if(written != buf.size()){
		PRINT("Error writing to file");
	}
	fclose(outFile);
	return BTV_NO_ERROR;
}

