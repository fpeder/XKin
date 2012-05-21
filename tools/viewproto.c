/*
 * Copyright (c) 2012, Fabrizio Pedersoli
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *     
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>

#include "../include/libgesture.h"

#define R 3
#define WIDTH 640
#define HEIGHT 480

char *infile=NULL;

void       usage           (void);
void       parse_args      (int,char**);


int main (int argc, char *argv[]) 
{
	IplImage *img;
	CvFileStorage *fs;
	CvSeq* seq;
	CvMemStorage *str;
	int N,i;
	
	parse_args(argc,argv);

	str = cvCreateMemStorage(0);
	img = cvCreateImage(cvSize(WIDTH,HEIGHT), 8, 3);
	cvZero(img);
	fs = cvOpenFileStorage(infile, str, CV_STORAGE_READ, NULL);
	assert(fs);

	N = cvReadIntByName(fs, NULL, "N", 0);
	seq = cvReadByName(fs, NULL, "seq", 0);

	printf("N=%d\n", N);
	for (i=0; i<seq->total; i++) {
		CvPoint p;

		p = *(CvPoint*)cvGetSeqElem(seq, i);
		cvCircle(img, p, R, CV_RGB(255,0,0), -1, 8, 0);
	}

	cvShowImage("proto", img);
	cvWaitKey(0);
	
	cvReleaseFileStorage(&fs);
	cvReleaseImage(&img);
	
	return 0;
}

void parse_args (int argc, char **argv)
{
	int c;

	opterr=0;
	while ((c = getopt(argc, argv, "i:h")) != -1) {
		switch (c) {
		case 'i':
			infile = optarg;
			break;
		case 'h':
		default:
			usage();
			exit(-1);
		}
	}
	if (infile == NULL) {
		usage();
		exit(-1);
	}
}

void usage (void)
{
	printf("usage: viewproto -i [file] -h\n");
	printf("  -i  gesture proto file\n");
}



