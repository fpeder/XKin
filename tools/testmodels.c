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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>

#include "../include/libgesture.h"

#define WIDTH 640
#define HEIGHT 480

char *infile=NULL;
IplImage *img;
ptseq seq;

void  parse_args  (int, char**);
void  usage       (void);
void  reset       (void);
void  on_mouse    (int, int, int, int, void*);



int main (int argc, char *argv[]) 
{
	CvHMM *mo;
	char win[]="gesture test";
	int num, count=1;
	double ll;

	parse_args(argc, argv);

	img = cvCreateImage(cvSize(WIDTH,HEIGHT), 8, 3);
	seq = ptseq_init();
	mo = cvhmm_read(infile, &num);	
	
	cvNamedWindow(win, 0);
	cvSetMouseCallback(win, on_mouse, 0);

	printf("q:quit  c:clear seq  r:classify seq\n");
	
	for (;;) {
		int c, r;
		
		cvShowImage(win, img);
		c = cvWaitKey(50);
		
		if (c == 'q') {
			break;
		} else if (c == 'c')
			reset();
		else if (c == 'p') {
			printf("point seq:\n");
			ptseq_print(seq);
		}
		else if (c == 'r') {
			int asd;
			
			asd = cvhmm_classify_gesture(mo, num, seq, stdout);
			printf("-->%d\n", asd);
			reset();
		}
	}

	cvReleaseImage(&img);
	
	return 0;
}

void on_mouse (int event, int x, int y, int asd, void *caz)
{
	CvPoint pt;
	
	if (event != CV_EVENT_LBUTTONDOWN)
		return;

	pt = cvPoint(x,y);
	ptseq_add(seq, cvPoint(x,y));

	cvCircle(img, pt, 5, CV_RGB(255,0,0), -1, 8, 0);
}

void reset (void)
{
	cvZero(img);
	seq = ptseq_reset(seq);
}

void parse_args (int argc, char **argv)
{
	int c;

	opterr = 0;
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
	printf("usage: testmodels -i [file] [-h]\n");
	printf("  -i  gesture models file\n");
	printf("  -h  show this message\n");
}

