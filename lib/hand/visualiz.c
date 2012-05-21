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
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/imgproc/imgproc_c.h>

#include "visualiz.h"
#include "const.h"


void draw_classified_hand (CvSeq *cnt, CvPoint cent, int type)
{
	CvRect box;
	CvScalar color[] = {CV_RGB(255,0,0),
			    CV_RGB(0,255,0),
	                    CV_RGB(0,0,255),
	                    CV_RGB(255,0,255),
	                    CV_RGB(255,255,0),
	                    CV_RGB(0,255,255),
	                    CV_RGB(128,0,0),
	                    CV_RGB(0,128,0),
	                    CV_RGB(0,0,128),
	                    CV_RGB(128,0,128),
	                    CV_RGB(128,128,0),
	                    CV_RGB(0,128,128)};

	static IplImage *img=NULL;

	if (img==NULL) {
		img = cvCreateImage(cvSize(WIDTH,HEIGHT), 8, 3);
	} 
	cvZero(img);
	
	cvDrawContours(img, cnt, color[type], CV_RGB(0,0,255), 0,
		       CV_FILLED, 8, cvPoint(0,0));

	cvFlip(img,NULL,1);

	cvNamedWindow("detection", CV_WINDOW_AUTOSIZE|CV_GUI_NORMAL);
	cvMoveWindow("detection", 0, 0);
	cvShowImage("detection", img);
}

void draw_contour (CvSeq *seq)
{
	IplImage *img = cvCreateImage(cvSize(WIDTH, HEIGHT), 8, 3);

	cvZero(img);
	cvDrawContours(img, seq, CV_RGB(255,0,0), CV_RGB(0,255,0),
		       0, 1, 8, cvPoint(0,0));

	cvShowImage("poly", img);

	cvReleaseImage(&img);
}

void draw_point_sequence (CvSeq *seq)
{
	IplImage *img  = cvCreateImage(cvSize(WIDTH, HEIGHT), 8, 3);
	CvScalar color = CV_RGB(255,0,0);
	size_t   i;

	cvZero(img);
	for (i=0; i<seq->total; i++) {
		CvPoint *p;

		p = (CvPoint*)cvGetSeqElem(seq, i);
		cvSet2D(img, p->y, p->x, color);
	}
	
	cvShowImage("ptseq", img);

	cvReleaseImage(&img);
}

void draw_histogram (CvMat *hist)
{
	int scale=10, H=128;
	size_t i;
	IplImage *qwe = cvCreateImage(cvSize((hist->cols)*scale, H), 8, 3);

	cvZero(qwe);
	for (i=0; i<hist->cols; i++) {
		int val = (int)(cvmGet(hist, 0, i) * H);

		cvRectangle(qwe, cvPoint(i*scale, H), cvPoint((i+1)*scale, H-val),
			    CV_RGB(255,200,200), -1, 8, 0);
	}

	cvShowImage("qwe", qwe);

	cvReleaseImage(&qwe);
}

void draw_cvxhull_and_cvxdefects (CvSeq *hull, CvSeq* def)
{
	int i, R=3;
	IplImage *qwe = cvCreateImage(cvSize(WIDTH,HEIGHT), 8, 3);

	cvZero(qwe);
	cvDrawContours(qwe, hull, CV_RGB(255,0,0), CV_RGB(0,255,0),
		       0, 1, 8, cvPoint(0,0));

	for (i=0; i<def->total; i++) {
		CvConvexityDefect *d;

		d = (CvConvexityDefect*)cvGetSeqElem(def, i);
		
		cvCircle(qwe, *(d->start),       R, CV_RGB(255,0,0), -1, 8, 0);
		cvCircle(qwe, *(d->end),         R, CV_RGB(0,255,0), -1, 8, 0);
		cvCircle(qwe, *(d->depth_point), R, CV_RGB(0,0,255), -1, 8, 0);
	}

	cvShowImage("qwe", qwe);
	cvReleaseImage(&qwe);
}

void print_mat (const CvMat *mat)
{
	int i,j;

	for (i=0; i<mat->rows; i++) {
		for (j=0; j<mat->cols; j++) {
			printf("%.2f ", cvmGet(mat, i, j));
		}
		printf("\n");
	}
	printf("\n");
}

