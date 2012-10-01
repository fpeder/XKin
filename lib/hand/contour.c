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
 * \file contour.c
 * \author Fabrizio Pedersoli
 *
 * Stuff for extracting the contour of the hand given the binary hand
 * depth image.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>

#include "const.h"
#include "contour.h"
#include "transform.h"
#include "visualiz.h"


static CvSeq*           get_hand_contour                  (IplImage*);
static void             morphological_smooth              (IplImage*);
static CvSeq*           get_largest_contour               (CvSeq*);
static CvPoint          get_contour_centroid              (CvSeq*);
static CvPoint          get_hand_centroid                 (IplImage*);
static CvPoint          get_bounding_box_centroid         (CvSeq*);
static IplImage*        hand_rgb_segmentation             (IplImage*, CvRect);
static void             contour_add_offset                (CvSeq*, CvRect);


/*!
 * \brief Extract the hand contour and centroid in the binary hand
 * image.
 *
 * This is for the computations of what is called basic hand contour,
 * which is the contour of the hand in binary depth image. This
 * contour is used for the basic posture classification, its bounding
 * box is also the base for the color hand segmentation.
 *
 * \param[in]       binary hand image
 * \param[in,out]   hand's contour (basic contour)
 * \param[in,out]   hand's centroid
 * \return          corrent detection (1) 
 */
int get_hand_contour_basic (IplImage *hand, CvSeq **dst, CvPoint *cent)
{
	if ((*dst = get_hand_contour(hand)) == NULL) {
		return 0;
	}

	if (cent != NULL) {
		*cent = get_contour_centroid(*dst); 
	}

	return 1;
}

/*!
 * \brief Extract the hand contour and centroid within a ROI in the
 * color image.
 *
 * This function get a more precise making the detection of the hand
 * starting from a bounding box of the color image. The ROI is found
 * through a transformation of bounding box of the contour in the
 * binary hand depth image.
 *
 * \param[in]      binary hand depth image
 * \param[in]      kinect color image
 * \param[in]      depth of the hand (needed for depth -> color bb map)
 * \param[in,out]  hand's contour (avdanced contour) 
 * \param[in,out]  hand's centroid
 * \return         correnct classification (1)
 *
 */
int get_hand_contour_advanced (IplImage *hand, IplImage *rgb, int z,
			       CvSeq **dst, CvPoint *cent)
{
	CvRect bb;
	IplImage *asd;

	asd = cvCloneImage(hand);

	if ((*dst = get_hand_contour(asd)) == NULL) {
		return 0;
	}

	bb = get_rgb_hand_bbox_from_depth(*dst, z);

	if (bb.x<0 || bb.y<0 ||
	    bb.x+bb.width > hand->width ||
	    bb.y+bb.height > hand->height) {
		return 0;
	}
	
	asd = hand_rgb_segmentation(rgb, bb);

	if ((*dst = get_hand_contour(asd)) == NULL) {
		return 0;
	}

	contour_add_offset(*dst, bb);

	if (cent != NULL) {
		*cent = get_contour_centroid(*dst);
	}

	cvReleaseImage(&asd);
	
	return 1;
}

/*!
 * \brief Core precedure for finding the hand contour.
 *
 * This is the base of get_hand_contour_basic, here is done the
 * proper contour extraction.
 *
 * \param[in]  binary hand image
 * \return     hand's contour 
 */
CvSeq* get_hand_contour (IplImage *hand)
{
	CvSeq *contours, *contour;
	CvPoint cent;
	static CvMemStorage *str = NULL;

	if (str==NULL) {
		str = cvCreateMemStorage(0);
	} else {
		cvClearMemStorage(str);
	}

	morphological_smooth(hand);
	cvFindContours(hand, str, &contours, sizeof(CvContour),
		       CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE,
		       cvPoint(0,0));

	contour = get_largest_contour(contours);

	return contour;
}

/*!
 * \brief Hand segmentation in the color image.
 *
 * The hand segmentation is accomplished in a small ROI where it's
 * known the presence of the hand. This ROI is determined as a
 * transformation of the hand's bounding box in the binary hand depth
 * image. This procedure return a new hand binary image computed in
 * the color space.
 *
 * \param[in]  kinect color image 
 * \param[in]  hand's bounding box (in depth) 
 * \return     binary hand (from) color image 
 */
static IplImage *hand_rgb_segmentation (IplImage* rgb, CvRect roi)
{
	IplImage *tmp, *asd;
	IplConvKernel *strel;

	tmp = cvCreateImage(cvSize(roi.width,roi.height), 8, 3);
	asd = cvCreateImage(cvSize(roi.width,roi.height), 8, 1);
	
	cvSetImageROI(rgb, roi);
	cvCopy(rgb, tmp, NULL);
	cvResetImageROI(rgb);
	cvCvtColor(tmp, tmp, CV_RGB2YCrCb);
	cvSplit(tmp, NULL, NULL, asd, NULL);
	cvSmooth(asd, asd, CV_GAUSSIAN, 7, 7, 0, 0);
	cvSmooth(asd, asd, CV_MEDIAN, 7, 7, 0 ,0);
	cvThreshold(asd, asd, 0, 255, CV_THRESH_OTSU);
	strel = cvCreateStructuringElementEx(3,3,0,0,CV_SHAPE_ELLIPSE, NULL);
	cvMorphologyEx(asd, asd, NULL, strel, CV_MOP_CLOSE, 3);

	cvReleaseImage(&tmp);

	return asd;
}

/*!
 * \brief Convert a contour from a ROI relative coordinates to
 * absolute coordinates. 
 *
 * It is added the top left corner coordinate of the bounding box to
 * each point of the contour sequence to recover the absolute
 * coordinate. 
 *
 * \param[in,out] hand's contour
 * \param[in] hand's color (transformed from depth) bounding box
 */
static void contour_add_offset (CvSeq *cnt, CvRect roi)
{
	int i;

	for (i=0; i<cnt->total; i++) {
		CvPoint *p = (CvPoint*)cvGetSeqElem(cnt, i);

		p->x += roi.x;
		p->y += roi.y;
	}
}

/*!
 * \brief Binary hand image enhancement
 *
 * The first operation is to remove impulsive noise with (small stain)
 * in the binary image with a median filter. The latter is to smooth
 * the hand shape with an morpholocial open and close.
 *
 * \param[in,out]  hand binary image
 */
static void morphological_smooth (IplImage *hand)
{
	IplConvKernel *elem;
	static IplImage *tmp=NULL;

	if (tmp==NULL) {
		tmp = cvCreateImage(cvGetSize(hand), 8, 1);
	} else {
		cvZero(tmp);
	}

	cvSmooth(hand, hand, CV_MEDIAN, MEDIAN_DIM, MEDIAN_DIM, 0, 0);
	elem = cvCreateStructuringElementEx(3, 3, 0, 0,
					    CV_SHAPE_ELLIPSE, NULL);

	cvMorphologyEx(hand, hand, tmp, elem, CV_MOP_OPEN, N_ITER);
	cvMorphologyEx(hand, hand, tmp, elem, CV_MOP_CLOSE, N_ITER);
}

/*!
 * \breif Hand's contour is the largest one. 
 *
 *  During the contour extraction in the binary hand image can happens
 *  that more contours are detected because of noise, hand contour is
 *  the largest one.
 *
 * \param[in]  Sequence of contours
 * \return     hand's contour 
 */
static CvSeq *get_largest_contour (CvSeq* contours)
{
	CvSeq *tmp, *larger=contours;
        float max=0;

        for (tmp=contours; tmp!=NULL; tmp=tmp->h_next) {
                double len = cvContourPerimeter(tmp);

                if (len > max) {
                        max = len;
                        larger = tmp;
                }
        }

        return larger;
}

/*!
 * \brief Calculate le centroid of the contour.
 *
 * \param[in]  contour
 * \retrun     centroid point 
 */
static CvPoint get_contour_centroid (CvSeq* contour)
{
	float x=0, y=0;
	int i, tot=contour->total;

	for (i=0; i<tot; i++) {
		CvPoint *p = (CvPoint*)cvGetSeqElem(contour, i);

		x += p->x;
		y += p->y;
	}
	x /= tot;
	y /= tot;

	return cvPoint((int)x, (int)y);
}
