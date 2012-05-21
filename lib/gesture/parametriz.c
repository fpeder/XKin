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
 * \file parametriz.c
 * \author Fabrizio Pedersoli
 *
 * This file deals with the parametrization of a gesture sequence. A
 * gesture sequnce is a sequence of points in a 2D space, in order to
 * obtain a robust classification which is invariant with respect to
 * translation scale this sequence must be parametriz.
 *
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core/core_c.h>

#include "const.h"
#include "ptseq.h"
#include "parametriz.h"
#include "visualiz.h"

static CvMat *diff (CvMat*);
static void quantize_angle(CvMat*, float);
static CvMat *reshape (CvMat **in, int num);

/*!
 * \brief Compute the parametrization of point sequnce (gesture).
 *
 * A sequence of (x,y) points is transformed in a sequnce of angles of
 * between succesive couple of points. \theat=arctan2((y2-y1)/(x2-x1)).
 *
 * \param[in]  point sequence
 * \return     vector of angles
 */
CvMat *ptseq_parametriz (ptseq in)
{
	CvMat *s, *x, *y, *rho, *theta;

	s = ptseq_to_mat(in);
	s = diff(s);

	x     = cvCreateMat(s->rows, 1, CV_32FC1);
	y     = cvClone(x);
	rho   = cvClone(x);
	theta = cvClone(x);

	cvGetCol(s, x, 0);
	cvGetCol(s, y, 1);
	
	cvCartToPolar(x, y, rho, theta, 1);

	quantize_angle(theta, 360./(float)NUM_SYMBOLS);

	cvReleaseMat(&x);
	cvReleaseMat(&y);
	cvReleaseMat(&rho);
	cvReleaseMat(&s);
	
	return theta;
}

/*!
 * \brief Compute parametrization of the entire training set.
 *
 * \param[in]  array of point sequence
 * \param[in]  number of sequence
 * \return     vector of all parametrized sequences. 
 */
CvMat *parametriz_training_set (ptseq *set, int num)
{
	CvMat **trset;
	CvMat *out;
	
	trset = (CvMat**)malloc(num * sizeof(CvMat*));
	
	int i;
	for (i=0; i<num; i++) {
		trset[i] = ptseq_parametriz(set[i]);
	}

	out = reshape(trset, num);
	
	return out;
}

/*!
 * \brief Convert a vector of vectors in a single vector.
 *
 * A seqeunce of observation sequences consists in a vector of
 * vectors, sequence are all of the same legnth but this functions can
 * handles vector of different length vectors...  This function
 * converts the training set in a single big vector (single big
 * observation sequence) later used for training.
 *
 * \param[in]   vector of vectors
 * \param[in]   number of vectors
 * \return      global vector
 */
static CvMat *reshape (CvMat **in, int num)
{
	CvMat *out;
	int i,j,k=0,tot=0;

	for (i=0; i<num; i++) {
		tot += in[i]->rows;
	}

	out = cvCreateMat(tot, 1, CV_32FC1);

	for (i=0; i<num; i++) {
		for (j=0; j<in[i]->rows; j++) {
			cvmSet(out, k++, 0, cvmGet(in[i], j, 0));
		}
	}

	free(in);
	
	return out;
}

/*!
 * \brief Quantize angles.
 *
 * The parametriz observation sequence consists in a sequence of
 * angles. Angles assumes a fixed number of directions -> angle is
 * quantized.
 *
 * \param[in,out]    vector of angles
 * \param[in]        quantization step
 */
static void quantize_angle (CvMat* theta, float step)
{
	int i;
	for (i=0; i<theta->rows; i++) {
		float v = cvmGet(theta, i, 0);

		v = cvRound(v/step);

		if (v == NUM_SYMBOLS) {
			v = 0;
		}
		
		cvmSet(theta, i, 0, v);
	}
}

/*!
 * \brief Computes diff of a vector -> x_i = x_{i+1} - x_i.
 *
 * \param[in]   input vector
 * \return      diff vector 
 */
static CvMat *diff (CvMat *in)
{
	CvMat *out  = cvCreateMat(in->rows-1, in->cols, CV_32FC1);
	CvMat *tmp1 = cvCreateMat(1, in->cols, CV_32FC1);
	CvMat *tmp2 = cvCreateMat(1, in->cols, CV_32FC1);
	
	int i;
	for (i=0; i<out->rows; i++) {

		cvGetRow(in, tmp1, i);
		cvGetRow(in, tmp2, i+1);

		cvSub(tmp2, tmp1, tmp1, NULL);
		
		int j;
		for (j=0; j<tmp1->cols; j++) {
			cvmSet(out, i, j, cvmGet(tmp1, 0, j));
		}
	}

	cvReleaseMat(&tmp1);
	cvReleaseMat(&tmp2);
	
	return out;
}

