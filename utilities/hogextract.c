/*
 * Written 1999 Jan 29 by Josh Cogliati
 * Modified by Bradley Bell, 2002, 2003
 * This program is licensed under the terms of the GPL, version 2 or later
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SWAPINT(x)   (((x)<<24) | (((uint)(x)) >> 24) | (((x) &0x0000ff00) << 8) | (((x) & 0x00ff0000) >> 8))

int
main(int argc, char *argv[])
{
	FILE *tHogFile, *writefile;
	int len;
	char filename[13];
	char *buf;
	struct stat statbuf;
	int v = 0;

	if (argc > 1 && !strcmp(argv[1], "v")) {
		v = 1;
		argc--;
		argv++;
	}

	if (argc < 2) {
		//printf("Usage: hogextract [v] tHogFile [filename]\n"
		       "extracts all the files in tHogFile into the current directory\n"
			   "Options:\n"
			   "  v    View files, don't extract\n");
		exit(0);
	}
	tHogFile = fopen(argv[1], "rb");
	stat(argv[1], &statbuf);
	//printf("%i\n", (int)statbuf.st_size);
	buf = (char *)malloc(3);
	fread(buf, 3, 1, tHogFile);
	//printf("Extracting from: %s\n", argv[1]);
	D2_FREE(buf);
	while(ftell(tHogFile)<statbuf.st_size) {
		fread(filename, 13, 1, tHogFile);
		fread(&len, 1, 4, tHogFile);
#ifdef WORDS_BIGENDIAN
		len = SWAPINT(len);
#endif
		if (argc > 2 && strcmp(argv[2], filename))
			fseek(tHogFile, len, SEEK_CUR);
		else {
			//printf("Filename: %s \tLength: %i\n", filename, len);
			if (v)
				fseek(tHogFile, len, SEEK_CUR);
			else {
				buf = (char *)malloc(len);
				if (buf == NULL) {
					//printf("Unable to allocate memory\n");
				} else {
					fread(buf, len, 1, tHogFile);
					writefile = fopen(filename, "wb");
					fwrite(buf, len, 1, writefile);
					fclose(writefile);
					D2_FREE(buf);
				}
			}
		}
	}
	fclose(tHogFile);

	return 0;
}
