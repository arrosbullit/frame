#include <stdio.h>
#include <vector>
#include "mpeg2_dec.h"
//#include "mpeg2_gethdr.h"
#include "mini_logger.h"

using namespace std;

//#define FILE_NAME "test.m2v"
#define FILE_NAME "/home/robert/eclipse_workspace/f_lnx_extract_img_from_ts_v2/extracted_pic.m2v"

int main()
{
	FILE *file;
	vector<unsigned char> vec;
	unsigned char c;
	int read;
	int ret;
	CLittle_Bitstream bits;

	ML_openFile("mpeg2_log.txt", "wb");

	file = fopen(FILE_NAME, "rb");
	if(!file){
		printf("Main: Cannot open %s\n", FILE_NAME);
		printf("Main: Press return to finish\n");
		getchar();
		printf("Main: Bye!");
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
	ret = MPEG2::Decode_Bitstream(&bits);
	printf("Decode_Bitstream: %d\n", ret);
	if(ret == 1){
		ret = MPEG2::write_ppm_file("thumb.ppm");
		printf("write_ppm_file: %d\n", ret);
	}
	MPEG2::Uninitialize_Decoder();

	printf("Main: Size read %d\n", vec.size());

	ML_closeFile();

	printf("Main: Press return to finish\n");
	getchar();
	printf("Bye!");
	return 0;
}
