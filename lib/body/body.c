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

/*!
 * \file body.c
 * \author Fabrizio Pedersoli 
 *
 * This file contains all the body related functions, the most
 * important is body_detection.
 *
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>

#include "const.h"
#include "visualiz.h"
#include "body.h"


static CvHistogram*       get_depth_histogram          (IplImage*);
static void               get_body_depth_interval      (CvHistogram*, int*);
static void               get_body_image               (IplImage*, IplImage*, int*);


/*!
 * \brief Detect the body in depth image.
 *
 * It takes the depth image produced by kinect and return a new depth
 * image which presents only body depth values.
 *  
 * \param[in]   depth image
 * \param[out]  body depth image
 */
IplImage *body_detection (IplImage *depth)
{
	CvHistogram *hist;
	int interval[2];
	static IplImage *body=NULL;
	static IplImage *tmp=NULL;

	if (body==NULL && tmp==NULL) {
		body = cvCreateImage(cvGetSize(depth), 8, 1);
                tmp = cvCreateImage(cvGetSize(depth), 8, 1);
	} else {
		cvZero(body);
		cvZero(tmp);
	}
	
	cvConvertScale(depth, tmp, 255./2048., 0);
	hist = get_depth_histogram(tmp);
	get_body_depth_interval(hist, interval);
	get_body_image(tmp, body, interval);

	return body;
}

/*!
 * \brief Compute histogram of the depth image.
 *
 * \param[in]  depth image
 * \return     histogram
 */
static CvHistogram *get_depth_histogram (IplImage *img)
{
	int hist_size[] = {NBINS};
	float range[] = {0, 255};
	float *ranges[] = {range};
	CvHistogram *hist;
	
	hist = cvCreateHist(1, hist_size, CV_HIST_ARRAY, ranges, 1);

	cvCalcHist(&img, hist, 0, 0);
	cvNormalizeHist(hist, 1.0);

	return hist;
}

/*!
 * \brief Compute depth values interval defined by the body.
 *
 * Body depth interval is considered as the nearst (respect Kinect)
 * support of the depth historgram.
 *
 * \param[in]  depth histrogram
 * \param[out] interval array (2 elements: min and max depth values)
 */
static void get_body_depth_interval (CvHistogram *hist, int *interval)
{
	int i, asd=1;
	float val;

	for (i=0; i<NBINS-1; i++) {

		val = cvGetReal1D(hist->bins, i);

		if (asd) {
			if (val !=0) {
				interval[0] = i;
				asd = 0;
			}
		} else {
			if (val == 0) {
				interval[1] = i-1;
				break;
			}
		}
	}
}

/*!
 * \brief Mask the depth image according the body interval.
 *
 * Body is isolatede in the depth image forcing to zero every pixels
 * which values is outside the body interval.
 *
 * \param[in]    depth image
 * \param[out]   body depth image
 * \params[in]   body depth interval
 */
static void get_body_image (IplImage *img, IplImage *body, int *interval)
{
	cvThreshold(img, body, interval[0], 0, CV_THRESH_TOZERO);
	cvThreshold(img, body, interval[1], 0, CV_THRESH_TOZERO_INV);
}
