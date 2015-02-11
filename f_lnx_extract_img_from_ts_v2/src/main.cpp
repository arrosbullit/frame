/*!
	@file		main.cpp
	@brief
	@author		Robert Lluís, 2015
	@note		Tests for extract_coded_img_from_ts.cpp
*/

#ifndef _WIN32
	typedef long long __int64;
	#define _ftelli64 ftello
	#define _fseeki64 fseeko
#endif

#include "ts_helper.h" // BTV_NO_ERROR
#include "extract_coded_img_from_ts.h"

using namespace std;

const char TARGET_FILENAME[] =
	"/media/sf_win_robert/Streams/test_suite_dc_at_11Mbps_in_mux_at_31Mbps.ts";
const unsigned TS_PACKET_HEADER_LENGTH = 4;
const unsigned TS_PACKET_SIZE = 188;


#ifndef TEST_ALL_FUNCS
// Here I test only the exported function while below
// I test all the functions includind the private ones.
// To test the private functions I have to make them accessible
// by changing the static keyword in the code.
int main() {
	vector<unsigned char> bigBuf;
	unsigned PID =  0x02d0;
	int re;
	// Test the only exported function.
	printf("Test getCodedImageFromFile\n");
	re = getCodedImageFromFile(TARGET_FILENAME, PID, bigBuf);
	if(re){
		printf("FAILED test getCodedImageFromFile\n");
	} else {
		printf("Pic size: %u\n", bigBuf.size());
		printf("SUCCESS test getCodedImageFromFile\n");
	}
	// Run the same test again
	printf("Test getCodedImageFromFile\n");
	re = getCodedImageFromFile(TARGET_FILENAME, PID, bigBuf);
	if(re){
		printf("FAILED test getCodedImageFromFile\n");
	} else {
		printf("Pic size: %u\n", bigBuf.size());
		printf("SUCCESS test getCodedImageFromFile\n");
	}

	// The End
	printf("Press return to finish\n");
	getchar();
	printf("Bye!");
	return 0;
}

# else

//#define STATIC static
#define STATIC extern
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
STATIC unsigned PID;
STATIC FILE *inputFile;

__int64 getFileSize(FILE *inputFile)
{
	__int64 fileSize;
	__int64 savedPos;
	savedPos = _ftelli64(inputFile);
	_fseeki64(inputFile, 0, SEEK_END);
	fileSize =  _ftelli64(inputFile);
	_fseeki64(inputFile, savedPos, SEEK_SET);
	return fileSize;
}

int savePicToFile(vector<unsigned char> &buf){
	FILE *outFile;
	outFile = fopen("extracted_pic.m2v", "wb");
	if(!outFile){
		printf("Cound not open out file\n");
		return BTV_ERR_GENERIC;
	}
	int written = fwrite(&buf[0], 1, buf.size(), outFile);
	if(written != buf.size()){
		printf("Error writing to file");
	}
	fclose(outFile);
	return BTV_NO_ERROR;
}

int main() {
	__int64 fileSize;
	int re;
	unsigned char packet[188];
	vector<unsigned char> payload;
	// This pid will be used all over
	PID = 0x02d0;
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
		printf("Pic size: %u\n", bigBuf.size());
		printf("SUCCEEDED getPic\n");
		savePicToFile(bigBuf);
	}
	// Test all pic sizes
	/*
	do{
		re = getPic(bigBuf);
		if(!re){
			printf("Pic size: %u\n", bigBuf.size());
		}
	}while(!re);
	*/
	// Test the only exported function.
	printf("Test getCodedImageFromFile\n");
	re = getCodedImageFromFile(TARGET_FILENAME, PID, bigBuf);
	if(re){
		printf("FAILED test getCodedImageFromFile\n");
	} else {
		printf("Pic size: %u\n", bigBuf.size());
		printf("SUCCESS test getCodedImageFromFile\n");
	}
	// Run the same test again
	printf("Test getCodedImageFromFile\n");
	re = getCodedImageFromFile(TARGET_FILENAME, PID, bigBuf);
	if(re){
		printf("FAILED test getCodedImageFromFile\n");
	} else {
		printf("Pic size: %u\n", bigBuf.size());
		printf("SUCCESS test getCodedImageFromFile\n");
	}

	// The End
	printf("Press return to finish\n");
	getchar();
	printf("Bye!");
	return 0;
}
#endif // TEST_ALL_FUNCS
