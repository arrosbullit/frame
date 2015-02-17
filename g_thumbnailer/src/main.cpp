/*!
	@file		g_main.cpp
	@brief		Example of use of the thumbnailer.
	@author		Robert Lluís, 2015
	@note
*/

#include <stdio.h>
#include <string.h>
#include "thumbnailer.h"
//const char TARGET_FILENAME[] = "/media/sf_win_robert/Streams/test_suite_dc_at_11Mbps_in_mux_at_31Mbps.ts";
//unsigned PID_main =  0x02d0; // Good
//unsigned PID_main =  0x0244; // Good
const char *FILE_NAMES[3] = {
		"/media/sf_win_robert/Streams/BBC/Mux4_18_11_14_10min.ts",
		"/media/sf_win_robert/Streams/test_suite_dc_at_11Mbps_in_mux_at_31Mbps.ts",
		"" // To mark the end
};
unsigned pids[2][15] = {
	{
		0x65, // Good
		0x12d, // Error a la primera img. Agafo la segona.
		0xc9, // No va
		0x0191, // Error a la primera img. Agafo la segona.
		0x01F5, // Error a la primera img. Agafo la segona.
		0x0907, // Bé
		0x0a97, // Error a la primera img. Agafo la segona.
		0x0b55, // Error a la primera img. Agafo la segona.
		0x083f, // Bé
		0x0af1, // Bé
		0x0849, // Error a la primera img. Agafo la segona.
		0x026d, // Bé
		0x08fd, // Error a la primera img. Agafo la segona.
		0x0961, // Bé
		0 // Zero per indicar el final
	},
	{
		0x02d0,
		0x244,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	}
};

int main()
{
	int ret;
	int j = 0;
	int i = 0;
	char outFileName[100];
	while(strlen(FILE_NAMES[i])){
		j = 0;
		while(pids[i][j]){
			sprintf(outFileName, "%d_%d_thumbnail.ppm", i, j);
			ret = doThumbnail((const char *)FILE_NAMES[i], pids[i][j],
					(const char *) outFileName);
			if(ret){
				printf("FAILED doThumbnail.\n");
			}
			j++;
		}
		i++;
	}
	printf("Press return to finish.\n");
	getchar();
	return 0;
}

