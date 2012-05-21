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
 * \file hand.c
 * \author Fabrizio Pedersoli
 */
 
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>

#include "const.h"
#include "contour.h"
#include "clustering.h"	
#include "visualiz.h"
#include "hand.h"


static void      get_hand_image         (IplImage*, IplImage*, int*);
static int       eval_hand_depth        (int*);


/*!
 * \brief Detect the hand straring from the body depth image.
 *
 * the function produces a binary image that contoins only the shpade
 * of the hand starting from the bodpy depth image. 
 *
 * \param[in]    depth body image
 * \param[out]   mean depth value of the hand
 * \return       binary hand image 
 */
IplImage* hand_detection (IplImage *body, int *z)
{
	int thrs[2], valid;
	CvSeq *ctr, *prev=NULL, *curr;
	static IplImage *hand=NULL;
	
	if (hand == NULL) {
		hand = cvCreateImage(cvGetSize(body), 8, 1);
	}
	else {
		cvZero(hand);
	}

	get_hand_interval(body, thrs);
	get_hand_image(body, hand, thrs);

	*z = eval_hand_depth(thrs);
       
	return hand;
}

/*!
 * \brief Compute the mean hand depth value.
 *
 * \param[in]   hand's depth interval
 * \return      mean depth 
 */
static int eval_hand_depth (int *thrs)
{
	float mean;

	mean = ((float)thrs[0] + (float)thrs[1])/2;

	return (int)mean;
}

/*!
 * \brief Make a binary image from the body image.
 *
 * According the hand's dept interval values the body image is two
 * times thresholded in order to get a binary image that contains as
 * foreground only the hand.
 *
 * \param[in]   body depth image
 * \param[out}  binary hand image
 * \param[in]   depth intarvals 
 */
static void get_hand_image (IplImage *body, IplImage *hand, int *thrs)
{
	cvThreshold(body, hand, thrs[0], 0, CV_THRESH_TOZERO);
	cvThreshold(body, hand, thrs[1]+2, 0, CV_THRESH_TOZERO_INV);
	cvThreshold(hand, hand, 0, 255, CV_THRESH_BINARY);
}


