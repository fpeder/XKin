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
#include <assert.h>
#include <unistd.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
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

char *outfile = NULL;
int num = -1;

void       save_posture_model         (CvFileStorage*, CvMat*, CvMat*);
void       init_training_data         (CvMat**, int);
void       free_training_data         (CvMat**, int);
void       add_training_data          (CvMat*,CvMat*);
void       parse_args                 (int,char**);
void       usage                      (void);


int main (int argc, char *argv[])
{
	IplImage *rgb, *depth, *tmp, *body, *hand;
	CvMat **tr, *mean, *cov;
	CvFileStorage *fs;
	int count=0, p=0;

	parse_args(argc, argv);

	rgb = cvCreateImage(cvSize(W,H), 8, 3);
	tr = (CvMat**)malloc(sizeof(CvMat*)*num);
	init_training_data(tr,num);
	mean = cvCreateMat(1, FD_NUM, CV_64FC1);
	cov  = cvCreateMat(FD_NUM, FD_NUM, CV_64FC1);

	fs = cvOpenFileStorage(outfile, NULL, CV_STORAGE_WRITE, NULL);
	assert(fs);

	for (;;) {
		int z, k, c;
		CvMat *fd;
		CvSeq *cnt;
		
		tmp = freenect_sync_get_rgb_cv(0);
		cvCvtColor(tmp, rgb, CV_BGR2RGB);
		depth = freenect_sync_get_depth_cv(0);

		body = body_detection(depth);
		hand = hand_detection(body, &z);

		if (!get_hand_contour_advanced(hand, rgb, z, &cnt, NULL))
		 	continue; 

		fd = get_fourier_descriptors(cnt);
		add_training_data(tr[count], fd);

		if (++count == num) {
			int c;

			cvCalcCovarMatrix((void*)tr, count, cov, mean, CV_COVAR_NORMAL);
			cvInvert(cov, cov, CV_LU);

			save_posture_model(fs, mean, cov);

			p++;
			count = 0;
			
			printf("save and quit:s  exit:q  next:any  ");
			
			if ((c = getchar()) == 's') {
				break;
			} else if (c == 'q') {
				break;
			} else {
				continue;
			}
		}

		draw_contour(cnt);
		cvWaitKey(T);
	}

	cvWriteInt(fs, "total", p);

	freenect_sync_stop();

	free_training_data(tr,num);
	cvReleaseFileStorage(&fs);
	cvReleaseMat(&mean);
	cvReleaseMat(&cov);
	cvReleaseImage(&rgb);

	return 0;
}

void save_posture_model (CvFileStorage *fs, CvMat *mean, CvMat *cov)
{
	static int i=0;
	static char asd[]="posture-00";
	
	cvStartWriteStruct(fs, asd, CV_NODE_MAP, NULL, cvAttrList(0,0));
	cvWriteInt(fs, "type", i++);
	cvWrite(fs, "mean", (void*)mean, cvAttrList(0,0));
	cvWrite(fs, "cov", (void*)cov, cvAttrList(0,0));
	cvEndWriteStruct(fs);

	if (++asd[9] == ':') {
		asd[9] = '0';
		asd[8]++;
	}
		
}

void add_training_data (CvMat *tr, CvMat *data)
{
	static int c=0;
	int i;

	for (i=0; i<data->cols; i++) {
		cvmSet(tr, 0, i, cvmGet(data, 0, i));
	}
	c++;
}

void init_training_data (CvMat **tr, int num)
{
	int i;

	for (i=0; i<num; i++) {
		tr[i] = cvCreateMat(1, FD_NUM, CV_64FC1);
	}
	
}

void free_training_data (CvMat **tr, int num)
{
	int i;

	for (i=0; i<num; i++) {
		cvReleaseMat(&tr[i]);
	}

	free(tr);
}

void parse_args (int argc, char **argv)
{
	int c;

	opterr=0;
	while ((c = getopt(argc,argv,"o:n:h")) != -1) {
		switch (c) {
		case 'o':
			outfile = optarg;
			break;
		case 'n':
			num = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			exit(-1);
		}
	}
	if (outfile == NULL || num <= 0) {
		usage();
		exit(-1);
	}
}

void usage (void)
{
	printf("usage: trainposture -o [file] -n [num] [-h] \n");
	printf("  -o  posture models yml output file\n");
	printf("  -n  number of sequence per posture\n");
	printf("  -h  show this message\n");
}

