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
 * \file myalgos.c
 * \author Fabrizio Pedersoli
 *
 * This file implements the foundamentals algorithms of an Hidden
 * Markov Model (HMM). Implementation is matrix oriented. 
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <opencv2/core/core_c.h>

#include "myhmm.h"
#include "visualiz.h"
#include "myalgos.h"

#define NORMALIZ(x) my_normalise(x);


double my_forward (CvHMM mo, CvMat *O, CvMat *alpha);
double my_forward_backward (CvHMM mo, CvMat *O, CvMat *gamma, CvMat *xi);
void my_baum_welch (CvHMM *mo, CvMat *O);
int get_observ_mask (CvMat *O, CvMat *mask, int o);
void selected_cols_sum (CvMat *src, CvMat *dst, CvMat *mask);
void cvhmm_update_params (CvHMM *mo, CvMat *entrans, CvMat *envisit, CvMat *enemit);
void my_make_stochastic (CvMat *src);
void my_normalise (CvMat *src);
int check_convergence (double curr, double prev);


/*!
 * \brief Compute a log likelihood of sequence given a model.
 *
 * \param[in]  HMM model
 * \param[in]  observation sequence
 * \return     log-likelihood
 */
double cvhmm_loglik (CvHMM *mo, CvMat *O)
{
	double ll = my_forward(*mo, O, NULL);

	return ll;
}

/*!
 * \brief wrapper for parameters reestimation.
 */
void cvhmm_reestimate (CvHMM *mo, CvMat *O )
{
	
	my_baum_welch (mo, O);
}

/*!
 * \brief Reestimate HMM's parameters.
 *
 * Implementation of the baum welch algorithm. This is used to train a
 * model given a sequence of observations.
 *
 * \param[in,out]   HMM model
 * \param[in]       observation sequence
 */
void my_baum_welch (CvHMM *mo, CvMat *O)
{
	int T,N,M;
	int iter=0;
	double ll, pll=EPS;
	CvMat *gamma, *xisum;
	CvMat *entrans, *envisit, *enemit;
	CvMat *tmp, *mask;
	
	N = mo->N;
	M = mo->b->cols;
	T = O->rows;

	gamma   = cvCreateMat(T, N, CV_64FC1);
	xisum   = cvCreateMat(N, N, CV_64FC1);
	entrans = cvCreateMat(N, N, CV_64FC1);
	envisit = cvCreateMat(1, N, CV_64FC1);
	enemit  = cvCreateMat(M, N, CV_64FC1);

	tmp  = cvCreateMat(1, N, CV_64FC1);
	mask = cvCreateMat(T, 1, CV_64FC1);
	
	cvZero(entrans);
	cvZero(envisit);
	cvZero(enemit);

	while (iter < MAX_ITER) {

		ll = my_forward_backward(*mo, O, gamma, xisum);

		cvAdd(entrans, xisum, entrans, NULL);

		cvGetRow(gamma, tmp, 0);
		cvAdd(envisit, tmp, envisit, NULL);

		int i;
		for (i=0; i<M; i++) {
			CvMat *tmp, *tmp2;

			tmp  = cvCreateMat(1, N, CV_64FC1);
			tmp2 = cvCreateMat(1, N, CV_64FC1);
			
			if (!get_observ_mask(O, mask, i)) {
				continue;
			}
			selected_cols_sum(gamma, tmp2, mask);
			cvGetRow(enemit, tmp, i);
			cvAdd(tmp, tmp2, tmp, NULL);
			
			cvReleaseMat(&tmp);
			cvReleaseMat(&tmp2);
		}

		cvhmm_update_params(mo, entrans, envisit, enemit);

		cvZero(entrans);
		cvZero(envisit);
		cvZero(enemit);

		if (check_convergence(ll, pll)) {
			break;
		}

		pll = ll;
		
		iter++;

	}

	cvReleaseMat(&gamma);
	cvReleaseMat(&xisum);
	cvReleaseMat(&entrans);
	cvReleaseMat(&enemit);
	cvReleaseMat(&envisit);
	cvReleaseMat(&tmp);
	cvReleaseMat(&mask);
}

/*!
 * \brief Set the new updated params to an HMM model.
 *
 * \param[in,out]   HMM model
 * \param[in]       expected #transistion matrix
 * \param[in]       expected #visit matrix
 * \param[in]       expected #emission matrix 
 */
void cvhmm_update_params (CvHMM *mo, CvMat *entrans, CvMat *envisit, CvMat *enemit)
{
	CvMat *tmp = cvCreateMat(enemit->cols, enemit->rows, CV_64FC1);
	
	my_make_stochastic(entrans);
	cvTranspose(enemit, tmp);
	my_make_stochastic(tmp);
	NORMALIZ(envisit);

	mo->A = cvCloneMat(entrans);
	mo->pi = cvCloneMat(envisit);
	mo->b = cvCloneMat(tmp);

	cvReleaseMat(&tmp);
}

/*!
 * \brief Check baum welch convergence.
 *
 *  This is used to terminate the restimation procedure before the
 *  maximum iteration when the algorithm converges checking likelihood
 *  improvment.
 *
 * \param[in]   current likelihood
 * \param[in]   previous likelihood
 * \return      convergences flag
 */
int check_convergence (double curr, double prev)
{
	double delta, avg;

	delta = fabs(curr - prev);
	avg = (fabs(curr) + fabs(prev) + EPS)/2;

	return delta/avg < THRESH;
}

/*!
 * \brief forward backward procedure.
 *
 * This is used to computes quanties needed by the baum-welch: gamma
 * and xisum.
 *
 * \param[in]        HMM model 
 * \param[in]        observation sequnce  
 * \param[in,out]    gamma 
 * \param[in,out]    xisum
 * \return           log-likelihood
 */
double my_forward_backward (CvHMM mo, CvMat *O, CvMat *gamma, CvMat *xisum)
{
	int t,T,N,i;
	CvMat *A, *b, *pi;
	CvMat *alpha, *beta;
	CvMat *br, *bpr, *obs, *ar, *gr;
	CvMat *tmp1, *tmp2, *tmp3;

	T = O->rows;
	N = mo.N;
	A = mo.A;
	pi = mo.pi;
	
	b = cvCreateMat(mo.b->cols, mo.b->rows, CV_64FC1);
	cvTranspose(mo.b, b);
	
	alpha = cvCreateMat(T, N, CV_64FC1);
	beta  = cvCreateMat(T, N, CV_64FC1);

	cvZero(beta);
	cvZero(gamma);
	cvZero(xisum);

	br   = cvCreateMat(1, N, CV_64FC1);
	bpr  = cvCreateMat(1, N, CV_64FC1);
	ar   = cvCreateMat(1, N, CV_64FC1);
	gr   = cvCreateMat(1, N, CV_64FC1);
	obs  = cvCreateMat(1, N, CV_64FC1);
	tmp1 = cvCreateMat(1, N, CV_64FC1);
	tmp2 = cvCreateMat(N, N, CV_64FC1);
	tmp3 = cvCreateMat(N, 1, CV_64FC1);
	
	double ll = my_forward(mo, O, alpha);

	for (t=T-1; t>=0; t--) {
		if (t==T-1) {
			for (i=0; i<N; i++) {
				cvmSet(beta, t, i, 1.0);
			}
			
			cvGetRow(alpha, ar, t);
			cvGetRow(beta, br, t);
			cvGetRow(gamma, gr, t);
			cvMul(ar, br, gr, 1);
			NORMALIZ(gr);
			
		} else {
			cvGetRow(b, obs, (int)cvmGet(O,t+1,0));
			cvGetRow(beta, bpr, t+1);
			cvMul(bpr, obs, tmp1, 1);

			cvGetRow(beta, br, t);
			cvGEMM(A, tmp1, 1, NULL, 0, tmp3, CV_GEMM_B_T);
			cvTranspose(tmp3, br);
			NORMALIZ(br);

			cvGetRow(gamma, gr, t);
			cvGetRow(alpha, ar, t);
			cvMul(ar, br, gr, 1);
			NORMALIZ(gr);

			cvGEMM(ar, tmp1, 1, NULL, 0, tmp2, CV_GEMM_A_T);
			cvMul(A, tmp2, tmp2, 1);
			NORMALIZ(tmp2);
			cvAdd(xisum, tmp2, xisum, NULL);
		}
	}

	cvReleaseMat(&alpha);
	cvReleaseMat(&beta);
	cvReleaseMat(&br);
	cvReleaseMat(&bpr);
	cvReleaseMat(&ar);
	cvReleaseMat(&gr);
	cvReleaseMat(&b);
	cvReleaseMat(&tmp1);
	cvReleaseMat(&tmp2);
	cvReleaseMat(&tmp3);

	return ll;
}

/*!
 * \brief Compute the forward algorithm.
 *
 * \param[in]   HMM model
 * \param[in]   observation sequence
 * \param[out]  alfa matrix
 */
double my_forward (CvHMM mo, CvMat *O, CvMat *alpha)
{
	int i,t,T,N;
	int clear=0;
	double ll;
	CvMat *A, *b, *pi;
	CvMat *curr, *prev, *c, *obs;

	N = mo.N;
	A = mo.A;
	pi = mo.pi;
	T = O->rows;

	b = cvCreateMat(mo.b->cols, mo.b->rows, CV_64FC1);
	cvTranspose(mo.b, b);

	if (alpha == NULL) {
		alpha = cvCreateMat(T, N, CV_64FC1);
		clear=1;
	}
	
	c    = cvCreateMat(T, 1, CV_64FC1); 
	curr = cvCreateMat(1, N, CV_64FC1);
	prev = cvCreateMat(1, N, CV_64FC1);
	obs   = cvCreateMat(1, N, CV_64FC1);

	cvZero(alpha);
	cvZero(c);

	for (t=0; t<T; t++) {
		cvGetRow(b, obs, (int)cvmGet(O,t,0));
		
		if (t==0) {
			cvGetRow(alpha, curr, t);
			cvMul(pi, obs, curr, 1);
			cvmSet(c, t, 0, cvSum(curr).val[0]);
			NORMALIZ(curr);

		} else {
			CvMat *tmp1, *tmp2;

			tmp1 = cvCreateMat(N,1,CV_64FC1);
			tmp2 = cvCreateMat(1,N,CV_64FC1);

			cvGetRow(alpha, prev, t-1);
			cvGEMM(A, prev, 1, NULL, 0, tmp1, CV_GEMM_A_T|CV_GEMM_B_T);
			cvTranspose(tmp1, tmp2);

			cvGetRow(alpha, curr, t);
			cvMul(tmp2, obs, curr, 1);
			cvmSet(c, t, 0, cvSum(curr).val[0]);
			NORMALIZ(curr);

			cvReleaseMat(&tmp1);
			cvReleaseMat(&tmp2);
		}
	}

	cvLog(c,c);
	ll = cvSum(c).val[0];

	if (clear) {
		cvReleaseMat(&alpha);
	}
	cvReleaseMat(&c);
	cvReleaseMat(&curr);
	cvReleaseMat(&prev);
	cvReleaseMat(&b);
	cvReleaseMat(&obs);
	
	return ll;
}

/*!
 * \brief Make a matrix stochastic.
 *
 * Each row of the matrix must sum to 1.
 *
 * \param[in,out]  matrix
 */
void my_make_stochastic (CvMat *src)
{
	int i;
	CvMat *tmp = cvCreateMat(1, src->cols, CV_64FC1);

	for (i=0; i<src->rows; i++) {
		cvGetRow(src, tmp, i);
		NORMALIZ(tmp);
	}
}

/*!
 * \brief implementaion trick
 */ 
int get_observ_mask (CvMat *O, CvMat *mask, int o)
{
	int i, num=0;
	
	cvZero(mask);
	
	for (i=0; i<O->rows; i++) {
		if (cvmGet(O, i, 0) == o) {
			cvmSet(mask, i, 0, 1);
			num++;
		}
	}

	return num;
}

/*!
 * \brief implementation trick
 */
void selected_cols_sum (CvMat *src, CvMat *dst, CvMat *mask)
{
	int i;
	CvMat *tmp = cvCreateMat(1, src->cols, CV_64FC1);

	cvZero(dst);
	
	for (i=0; i<src->rows; i++) {
		if (cvmGet(mask, i, 0) == 1) {
			cvGetRow(src, tmp, i);
			cvAdd(dst, tmp, dst, NULL);
		}
	}

	cvReleaseMat(&tmp);
}

/*! Normalize a vector to sum 1.
 *
 * \param[in,out]  vector
 */
void my_normalise (CvMat *src)
{
	double sum = cvSum(src).val[0];
	double scale = sum ? sum : 1;

	cvConvertScale(src, src, 1./scale, 0);
}

