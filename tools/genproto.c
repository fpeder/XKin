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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>

#define WIDTH 640
#define HEIGHT 480

char *outfile=NULL;
int N=-1;	
IplImage *img;
CvSeq *seq;
CvMemStorage *str;

void on_mouse (int event, int x, int y, int asd, void* caz);
void save_sequence (const char *file, int N);
void parse_args (int argc, char**argv);
void usage (void);
void help (void);


int main (int argc, char *argv[]) 
{
	char win[] = "proto";

	parse_args(argc,argv);
	
	str = cvCreateMemStorage(0);
	img = cvCreateImage(cvSize(WIDTH,HEIGHT), 8, 3);
	seq  = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq),
			   sizeof(CvPoint), str);

	help();
	
	cvNamedWindow(win, 0);
	cvSetMouseCallback(win, on_mouse, 0);
	
	for (;;) {
		int c;
		
		cvShowImage("proto", img);
		c = cvWaitKey(50);
		
		if (c == 'q') {
			return 0;
		}
		else if (c == 'c') {
			cvZero(img);
			cvClearSeq(seq);
		}
		else if (c == 's') {
			save_sequence(outfile, N);

			return 0;
		}

	}
	
	cvReleaseImage(&img);

	return 0;
}

void on_mouse (int event, int x, int y, int asd, void* caz)
{
	CvPoint pt;
	
	if (event != CV_EVENT_LBUTTONDOWN) {
		return;
	}

	pt = cvPoint(x,y);
	cvSeqPush(seq, &pt);
	cvCircle(img, pt, 5, CV_RGB(255,0,0), -1, 8, 0);
}

void save_sequence (const char *file, int N)
{
	CvFileStorage *fs;

	fs = cvOpenFileStorage(file, NULL, CV_STORAGE_WRITE, NULL);
	cvWriteInt(fs, "N", N);
	cvWrite(fs, "seq", seq, cvAttrList(0,0));
	cvReleaseFileStorage(&fs);
}

void parse_args (int argc, char**argv)
{
	int c;
	
	opterr = 0;
	while ((c = getopt(argc, argv, "o:n:h")) != -1) {
		switch (c) {
		case 'o':
			outfile = optarg;
			break;
		case 'n':
			N = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			exit(-1);
		}
	}
	if (outfile==NULL || N==-1) {
		usage();
		exit(-1);
	}
}

void usage ()
{
	printf("usage: genproto -o [file] -n [num]\n");
	printf("  -o  output file\n");
	printf("  -n  number of state\n");
}
	
void help ()
{
	printf("quit:q\tclear:c\tsave:s\n");
}
