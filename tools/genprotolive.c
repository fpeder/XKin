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
#include <pthread.h>
#include <string.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <libfreenect_sync.h>
#include <libfreenect_cv.h>

#include "../include/libbody.h"
#include "../include/libhand.h"
#include "../include/libposture.h"
#include "../include/libgesture.h"

enum {
        W=640,
        H=480,
        T=30,
};

char *outfile = NULL;
int N = 0;


IplImage* draw_depth_hand (CvSeq *cnt, int type);
void parse_args (int argc, char**argv);
void save_sequence (const char *file, ptseq seq, int N);
void usage ();


int main (int argc, char *argv[])
{
	CvHMM *models;
	char *win = "hand";
	int num, count=0, curr=1; 
	ptseq seq;
	
	parse_args(argc,argv);
	seq = ptseq_init();
	
	for (;;) {
		IplImage *depth, *tmp, *body, *hand;
		CvSeq *cnt;
		CvPoint cent;
		int z, p, k; 
		
		depth = freenect_sync_get_depth_cv(0);
		body = body_detection(depth);
		hand = hand_detection(body, &z);
		
		if (!get_hand_contour_basic(hand, &cnt, &cent))
			continue;
		
		if ((p = basic_posture_classification(cnt)) == -1)
			continue;
		
		if (cvhmm_get_gesture_sequence(p, cent, &seq)) {
			ptseq_draw(seq, 0);
			if ((k = cvWaitKey(0)) == 's') {
				save_sequence(outfile, seq, N);
				break;
			}
			seq = ptseq_reset(seq);
		}
		
		hand = draw_depth_hand(cnt, p);
		cvShowImage("hand", hand);

		if ((k = cvWaitKey(T)) == 'q')
			break;
	}

	freenect_sync_stop();
	cvDestroyAllWindows();

	return 0;
}
	
IplImage* draw_depth_hand (CvSeq *cnt, int type)
{
        static IplImage *img = NULL;
        CvScalar color[] = {CV_RGB(255,0,0), CV_RGB(0,255,0)};

        if (img == NULL)
                img = cvCreateImage(cvSize(W, H), 8, 3);

        cvZero(img);
        cvDrawContours(img, cnt, color[type], CV_RGB(0,0,255), 0,
                       CV_FILLED, 8, cvPoint(0,0));

        cvFlip(img, NULL, 1);

        return img;
}

void save_sequence (const char *file, ptseq seq, int N)
{
	CvFileStorage *fs;

	fs = cvOpenFileStorage(file, NULL, CV_STORAGE_WRITE, NULL);
	cvWriteInt(fs, "N", N);
	cvWrite(fs, "seq", seq.ptr, cvAttrList(0,0));
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
	if (outfile==NULL || N==0) {
		usage();
		exit(-1);
	}
}

void usage ()
{
	printf("usage: genprotoslive -o [file] -n [num]\n");
	printf("  -o  output file\n");
	printf("  -n  number of state\n");
}

