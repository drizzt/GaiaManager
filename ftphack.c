/* 
 * Copyleft (c) drizzt
 * Released in public domain
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRING_TO_REPLACE	"\x00/dev_hdd0/game/%s"
#define STRING_TO_SUBSTITUTE	"\x00/\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"

int main(int argc, char *argv[])
{
	FILE *fid;
	long len, i;
	char *buf;

	if (argc < 2){
		fprintf(stderr, "%s: insufficient arguments\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (NULL == (fid = fopen(argv[1], "r+b"))) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	fseek(fid, 0, SEEK_END);
	len = ftell(fid);
	fseek(fid, 0, SEEK_SET);

	if (NULL == (buf = malloc(len))) {
		perror("malloc");
		fclose(fid);
		exit(EXIT_FAILURE);
	}

	fread(buf, len, 1, fid);

	for (i = 0; i < len; i++) {
		if (!memcmp(&buf[i], STRING_TO_REPLACE, sizeof(STRING_TO_REPLACE)))
		{
			fprintf(stderr, "FTP root hack successfully applied on offset %ld!!\n", i);
			memcpy(&buf[i], STRING_TO_SUBSTITUTE, sizeof(STRING_TO_REPLACE));
			fseek(fid, 0, SEEK_SET);
			fwrite(buf, len, 1, fid);
			break;
		}
	}

	fclose(fid);
}
