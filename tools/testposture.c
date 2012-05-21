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
#include <assert.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <libfreenect/libfreenect_sync.h>
#include <libfreenect/libfreenect_cv.h>

#include "../include/libbody.h"
#include "../include/libhand.h"
#include "../include/libposture.h"

enum {
        W=640,
        H=480,
        T=30,
};

char *infile = NULL;

int        load_posture_models      (char*, CvPostModel**);
void       usage                    (void);
void       parse_args               (int,char**);


int main (int argc, char *argv[])
{
	IplImage *rgb, *depth, *body, *hand;
	CvPostModel *models;
	int num, count=0;

	parse_args(argc,argv);

	rgb = cvCreateImage(cvSize(W,H), 8, 3);
	num = load_posture_models(infile, &models);

	for (;;) {
		IplImage *tmp;
		CvSeq *cnt;
		CvPoint cent;
		int p, z, k;

		tmp = freenect_sync_get_rgb_cv(0);
		cvCvtColor(tmp, rgb, CV_BGR2RGB);
		depth = freenect_sync_get_depth_cv(0);

		body = body_detection(depth);
		hand = hand_detection(body, &z);

		if (!get_hand_contour_advanced(hand, rgb, z, &cnt, &cent))
			continue;

		if ((p = advanced_posture_classification(cnt, models, num)) == -1)
			continue;

		draw_classified_hand(cnt, cent, p);

		if ((k = cvWaitKey(T)) == 'q')
			break;
	}

	freenect_sync_stop();

	cvDestroyAllWindows();
	cvReleaseImage(&rgb);
	cvReleaseImage(&depth);
	cvReleaseImage(&body);
	cvReleaseImage(&hand);
	
	return 0;
}

int load_posture_models (char *infile, CvPostModel **p)
{
	CvFileStorage *fs;
	CvFileNode *node;
	CvMat *tmp;
	char name[] = "posture-00";
	int i, total;

	fs = cvOpenFileStorage(infile, NULL, CV_STORAGE_READ, NULL);
	assert(fs);
	total = cvReadIntByName(fs, NULL, "total", 0);
	*p = (CvPostModel*)malloc(sizeof(CvPostModel)*total);

	for (i=0; i<total; i++) {
		node = cvGetFileNodeByName(fs, NULL, name);
		tmp = (CvMat*)cvReadByName(fs, node, "mean", NULL);
		(*p)[i].mean = cvClone(tmp);
		tmp = (CvMat*)cvReadByName(fs, node, "cov", NULL);
		(*p)[i].cov = cvClone(tmp);

		if (++name[9] == ':') {
			name[9] = '0';
			name[8]++;
		}
	}

	cvReleaseFileStorage(&fs);

	return total;
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
	printf("usage: testposture -i [file] [-h]\n");
	printf("  -i  posture models file\n");
	printf("  -h  show this message\n");
}


