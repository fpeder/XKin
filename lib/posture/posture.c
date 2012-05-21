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
 * \file posture.c
 * \author Fabrizio Pedersoli
 *
 * This file implentes the posture classification. Classification is
 * done in two ways: basic and advanced. Basic classification is
 * limited to two posture (open/close) but very robust since it uses
 * only the depth image. Advenced classification is direct to the
 * classification of an higher type of posture but it's less robust
 * than the previous.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>

#include "const.h"
#include "fourierdesc.h"
#include "posture.h"


static int        is_hand_closed                 (CvSeq*, CvSeq*);
static float      get_defects_mean_depth         (CvSeq*);
static int        validate_mean_defects_depth    (CvSeq*, CvSeq*);
static CvSeq*     contour_approximation          (CvSeq*);
static int        fd_argmin_distance             (CvMat*, CvPostModel*, int);
static int        majority_classification        (int, int);


/*!
 * \brief Classify an hand contour in open or close.
 *
 * Classification is done using the contour (basic contour) of the
 * hand depth image. It is based on the convexity defects of the
 * convexhull of the contour.
 *
 * \param[in]   hand's contour (basic)
 * \return      classification index 
 */
int basic_posture_classification (CvSeq *ctr)
{
	int posture=0;
	CvSeq *pol, *hull, *def;
	static CvMemStorage *s1=NULL, *s2=NULL;
	
	if (s1 == NULL && s2 == NULL) {
		s1 = cvCreateMemStorage(0);
		s2 = cvCreateMemStorage(0);
	} else {
		cvClearMemStorage(s1);
		cvClearMemStorage(s2);
	}

	pol  = contour_approximation(ctr);
	hull = cvConvexHull2(pol, s1, CV_CLOCKWISE, 0);
	def  = cvConvexityDefects(pol, hull, s2);
	posture = is_hand_closed(pol, def) ? HAND_CLOSE : HAND_OPEN;
	posture = majority_classification(posture, 2);
	
	return posture;
}

/*!
 * \brief Classify an hand contour in many posture (> 2).
 * 
 * Classification is done using the hand contour (advanced contour)
 * obtained in the color image. This is more defined and suitable
 * respect the contour computed in the depth image for a
 * classification. The classification is done according the minimum
 * distance criteria between points in a high dimensional space
 * (feature vector).
 *
 * \param[in]   hand's contour in the color image
 * \param[in]   array of posture models
 * \param[in]   number of models
 * \return      classification index 
 */
int advanced_posture_classification (CvSeq *cnt, CvPostModel *mo, int num)
{
	int posture;
	CvMat *fd;

	fd = get_fourier_descriptors(cnt);
	posture = fd_argmin_distance(fd, mo, num);
	posture = majority_classification(posture, num);
	
	return posture;
}

/*!
 * \brief Check if the hand is closed.
 *
 * To decide if the hand's contour rapresent a closed will be tested
 * some charachteristics of the convexity defects: #num, mean depth
 * (this is the depth respect to convexhull, length menaing).
 *
 * \param[in]   polygon approximation of the hand's contour
 * \param[in]   convexity defects point
 * \return      classification index
 */
static int is_hand_closed (CvSeq *pol, CvSeq *def)
{
	return !( validate_mean_defects_depth(pol, def) &&
		  def->total >= NUM_DEFECTS );
}

/*!
 * \brief Test convexity defects points.
 *
 * An open hand presents convexity defects points that are distant
 * respect the convexhull. Here is tested the mean depth respect the
 * bounding box dimension. 
 *
 * \param[in]   polygon approx of hand's contour
 * \param[in]   conveity defects
 * \return      classification flag 
 */
static int validate_mean_defects_depth (CvSeq *pol, CvSeq *def)
{
	float mean;
	CvRect r = cvBoundingRect(pol, 0);
	float asd = (r.width + r.height)/2;

	mean = get_defects_mean_depth(def);
	
	return mean >= asd/DEFECTS_DEPTH_FACTOR;
}

/*!
 * \brief Compute the mean depth of the convexity defects.
 *
 * \param[in]  convexity defects points
 * \return     mean depth 
 */
static float get_defects_mean_depth (CvSeq* def)
{
	int i;
	float mean=0;

	for (i=0; i<def->total; i++) {
		CvConvexityDefect *d;

		d = (CvConvexityDefect*)cvGetSeqElem(def, i);
		mean += d->depth;
	}
	mean /= def->total;

	return mean;
}

/*!
 * \brief Compute a polygon approximantion of a contour.
 *
 * \param[in]  contour
 * \return     polygon approximation 
 */
static CvSeq *contour_approximation (CvSeq *contour)
{
	static CvMemStorage *storage=NULL;
	CvSeq *poly;

	if (storage == NULL)
		storage = cvCreateMemStorage(0);
	else
		cvClearMemStorage(storage);
	
	poly = cvApproxPoly(contour, sizeof(CvContour), storage,
			    CV_POLY_APPROX_DP,
			    POLY_APPROX_PRECISION, 0);
	
	return poly;
}

/*!
 * \brief Classify a fourier descriptor  vector.
 *
 * The classification of an advanced posture it's done using the
 * Mahalanobis distance. Given a feature vector (fourier descriptors)
 * the classification is done respect the model which is a minor
 * distance.
 *
 * \param[in]   fourier descriptors vector 
 * \param[in]   array of posture models
 * \param[in]   number of posture models
 * \return      classification index 
 */
static int fd_argmin_distance (CvMat *fd, CvPostModel *mo, int num)
{
	int i, argmin=0;
	double min=1e6;

	for (i=0; i<num; i++) {
		double dist;
		
		dist = cvMahalanobis(fd, mo[i].mean, mo[i].cov);

		if (dist < min) {
			min = dist;
			argmin = i;
		}
	}

	return argmin;
}

/*!
 * \brief Majority decision respect a number of classifications.
 *
 * This implents a buffer of posture classifications, to make a
 * classification more stable and filter out some errors due to
 * noise. The current classification is done using BUFFLEN previous
 * classifications and the value is the one which appears more times
 * in the buffer.
 *
 * \param[in]   posture index
 * \param[in]   number of different postures
 * \return      classification index 
 */
int majority_classification (int p, int num)
{
	static int buffer[BUFFLEN];
	static int count=0;
	int *asd;

	asd = (int*)malloc(sizeof(int) * num);
	memset(asd, 0, sizeof(int)*num);

	if (count < BUFFLEN) {
		buffer[count++] = p;
		return -1;
	} else {
		int i,argmax=-1,max=-1;

		memcpy(buffer, buffer+1, sizeof(int)*BUFFLEN-1);
		buffer[BUFFLEN-1] = p;

		for (i=0; i<BUFFLEN; i++) {
			asd[buffer[i]]++;
		}
		for (i=0; i<num; i++) {
			if (asd[i] > max) {
				max = asd[i];
				argmax=i;
			}
		}

		free(asd);

		return argmax;
	}
}
