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
 * \file fourirerdesc.c
 * \author Fabrizio Pedersoli
 *
 * This file implements the descriptors [0] used for the posture
 * classification which is done on the contour of the hand.
 *
 * [0] http://fourier.eng.hmc.edu/e161/lectures/fd/node1.html
 */

#include "config.h"

#include <stdio.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <fftw3.h>

#include "const.h"
#include "fourierdesc.h"


static void        get_coefficients      (fftw_complex*, double*);
static void        fftw_fill_data        (CvSeq*, fftw_complex*);
static void        fftw_fill_data        (CvSeq*, fftw_complex*);
static void        cvmat_fill_data       (CvMat*, double*);
static CvSeq*      contour_sampling      (CvSeq*, int);
static void        seq_to_mat            (CvSeq*, CvMat*, CvMat*);
static void        mat_to_seq            (CvMat*, CvMat*, CvSeq*, int);


/*!
 * \breif Computes fourier descriptors of a contour.
 *
 * Fourier descriptors are based on the modules of the dft of a
 * complex signale `z[n] = x[n] + j*y[n] where x and y are the
 * coordinates the points of the contours. 
 *
 * \param[in]  contour
 * \return     vector of descriptors
 */
CvMat *get_fourier_descriptors (CvSeq *cnt)
{
	CvSeq *samples;
	static CvMat *desc=NULL;
	double fd[FD_NUM];
	fftw_complex *in, *out;
	fftw_plan forward;

	if (desc==NULL) {
		desc = cvCreateMat(1, FD_NUM, CV_64FC1);
	} else {
		cvZero(desc);
	}

	samples = contour_sampling(cnt, SAMPLES_NUM);
		
	in  = (fftw_complex*)fftw_malloc(sizeof(fftw_complex)*SAMPLES_NUM);
	out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex)*SAMPLES_NUM);
	fftw_fill_data(samples, in);

	forward = fftw_plan_dft_1d(SAMPLES_NUM, in, out,
				   FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_execute(forward);
	
	get_coefficients(out, fd);
	cvmat_fill_data(desc, fd);

	return desc;
}

/*!
 * \brief Computes the fourier coefficients.
 *
 * \param[in]   complex signal
 * \param[out]  descriptors
 */
static void get_coefficients (fftw_complex *data, double *desc)
{
	size_t i;
	double C1 = sqrt( pow(data[1][0],2) + pow(data[1][1],2) );
	
	for (i=0; i<FD_NUM; i++) {
		double tmp;

		tmp = sqrt( pow(data[i+2][0],2) + pow(data[i+2][1],2) );
		desc[i] =  tmp/C1;
	}	
}

static void fftw_fill_data (CvSeq *seq, fftw_complex *data)
{
	size_t i;

	for (i=0; i<seq->total; i++) {
		CvPoint *p;

		p = CV_GET_SEQ_ELEM(CvPoint, seq, i);
		data[i][0] = p->x;
		data[i][1] = p->y;
	}
}

static void cvmat_fill_data (CvMat *M, double *data)
{
	size_t i;

	for (i=0; i<FD_NUM; i++) {
		cvmSet(M, 0, i, data[i]);
	}
}

/*!
 * \brief resample a contour to a fixed number of points.
 *
 * Each contour must have a fixed number of points because the complex
 * signal have must have always the same number of samples. For this
 * the contour must be redefined through proper interpolation.
 *
 * \param[in]  contour
 * \param[in]  target number of points
 * \return     resampled contour     
 */
static CvSeq *contour_sampling (CvSeq *contour, int N)
{
	CvSeq *samples;
	static CvMemStorage *str=NULL;
	CvMat *xi = cvCreateMat(1, contour->total, CV_64FC1);
	CvMat *yi = cvCreateMat(1, contour->total, CV_64FC1);
	CvMat *xf = cvCreateMat(1, N, CV_64FC1);
	CvMat *yf = cvCreateMat(1, N, CV_64FC1);

	seq_to_mat(contour, xi, yi);

	cvResize(xi, xf, CV_INTER_LINEAR);
	cvResize(yi, yf, CV_INTER_LINEAR);

	if (str == NULL) {
		str = cvCreateMemStorage(0);
	} else {
		cvClearMemStorage(str);
	}
	
	samples = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq),
			      sizeof(CvPoint), str);

	mat_to_seq(xf, yf, samples, N);

	cvReleaseMat(&xi);
	cvReleaseMat(&yi);
	cvReleaseMat(&xf);
	cvReleaseMat(&yf);
	
	return samples;
}

static void seq_to_mat (CvSeq *cont, CvMat *Mx, CvMat *My)
{
	size_t i;
	
	for (i=0; i<cont->total; i++) {
		CvPoint *p = CV_GET_SEQ_ELEM(CvPoint, cont, i);

		((double*)Mx->data.fl)[i] = p->x;
		((double*)My->data.fl)[i] = p->y;
	}
}

static void mat_to_seq (CvMat *Mx, CvMat *My, CvSeq *cont, int N)
{
	size_t i;
	int x,y;
	CvPoint p;
	
	for (i=0; i<N; i++) {
		x = (int)((double*)Mx->data.fl)[i];
		y = (int)((double*)My->data.fl)[i];

		p = cvPoint(x,y);
		cvSeqPush(cont, &p);
	}
}
