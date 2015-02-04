#include <stdio.h>
#include <vector>
#include "mpeg2_dec.h"
#include "mpeg2_gethdr.h"
#include "mini_logger.h"

using namespace std;

#define FILE_NAME "test.m2v"

int main()
{
	FILE *file;
	vector<unsigned char> vec;
	unsigned char c;
	int read;
	CLittle_Bitstream bits;

	ML_openFile("my_log.txt", "wb");

	file = fopen(FILE_NAME, "rb");
	if(!file){
		printf("Cannot open %s\n", FILE_NAME);
		printf("Press return to finish\n");
		getchar();
		printf("Bye!");
	}

	for(;;){
		read = fread(&c, 1, 1, file);
		if(read <= 0){
			break;
		}
		vec.push_back(c);
	};

	bits.init(&vec[0], vec.size());



	MPEG2::Initialize_Decoder();
	MPEG2::Decode_Bitstream(&bits);

	printf("Size read %d\n", vec.size());

	ML_closeFile();

	printf("Press return to finish\n");
	getchar();
	printf("Bye!");
	return 0;
}
