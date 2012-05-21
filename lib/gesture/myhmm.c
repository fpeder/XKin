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
 * \file myhmm.c
 * \author Fabrizio Pedersoli
 *
 * This file defines the higher level function toward gesture HMM models
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core/core_c.h>

#include "const.h"
#include "myalgos.h"
#include "ptseq.h"
#include "rw.h"
#include "training.h"
#include "parametriz.h"
#include "myhmm.h"

/*!
 * \brief Crate an HMM from a gesture prototype.
 *
 * Given a prototype sequnce that defines a gesture is created the
 * relative HMM models via an automatic training.
 *
 * \param[in]  file where the prototype is stored
 * \return     HMM model 
 */
CvHMM cvhmm_from_gesture_proto (const char *infile)
{
	CvHMM mo;
	CvMat *training;
	ptseq proto;
	int N;

	proto = read_gesture_proto(infile, &N);
	mo = cvhmm_blr_init(N, NUM_SYMBOLS, .8, .2);
	training = make_training_set(proto, NUM_TRAINING_SEQ);
	cvhmm_reestimate(&mo, training);

	return mo;
}

/*!
 * \brief Classy a sequence of point (gesture).
 *
 * An observation sequence is classified with the model that
 * guarantees the high likelihood.
 *
 * \param[in]   array of HMM models
 * \param[in]   number of models
 * \param[in]   observation sequenc
 * \param[in]   flags to display overall lls
 * \return      classification index
 */
int cvhmm_classify_gesture (CvHMM *mo, int num, ptseq seq, int debug)
{
	CvMat *O;
	int i, argmax = -1;
	double ll, max = -1e8;

	O = ptseq_parametriz(seq);
	
	for (i=0; i<num; i++) {
		if ((ll = cvhmm_loglik(&(mo[i]), O)) > 1)
			ll = NAN;
		if (debug)
			printf("%d=%.2f ", i, ll);

		if (ll > max && !isnan(max)) {
			max = ll;
			argmax = i;
		}
	}
	if (debug)
		printf("\n");

	return argmax;
}

static double point_dist (CvPoint p1, CvPoint p2)
{
	return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
}

/*!
 * \brief Get the correct gesture sequence from video stream.
 *
 * This implent a state machine for collecting the most correct
 * sequnce of points that define a gesture. This machine has three
 * states: START, COLLECT and STOP. The default state is STOP, the
 * transition to START is done when the hand assume the closed
 * posture. The hand has to stay in this state for an X number of
 * frame, after that the machine goes in COLLECT state. In this state
 * are collected all the points related to the hand's (closed posture)
 * centroid. Sometimes happend that this sequence contains some noisy
 * point not related to the trajectory, this function filter out them
 * checking the intrapoint distance. the machine goes in STOP state
 * when for an Y number of frames an open posture is detected.
 *
 * \param[in]        posture classification index
 * \param[in]        hand centroid
 * \param[in,out]    sequence of point
 * \return           flag correct sequence ready
 */
int cvhmm_get_gesture_sequence (int posture, CvPoint pt, ptseq *seq)
{
	static int state = STOP;
	static int count = 0;
	static int miss = 0;
	static int tot = 0;
	static CvPoint prev;

	switch (state) {
	case START:
		if (posture == CLOSE) {
			if (++count >= 3) {
				state = COLLECT;
				count = 0;
				prev = cvPoint(pt.x, pt.y);
			}
		}
		break;
	case COLLECT:
		if (posture == CLOSE) {
			double dist = point_dist(pt, prev);

			if (dist <= 100 && dist >= 4) {
				ptseq_add(*seq, pt);
				prev = cvPoint(pt.x, pt.y);
				tot++;
			}
			miss = 0;
		} else {
			if (++miss >= 3) {
				state = STOP;
				if (tot >= 10) {
					ptseq_remove_tail(*seq, 5);
					return 1;
				} else {
					return 0;
				}
			}
		}
		break;
	case STOP:
		if (posture == CLOSE) {
			state = START;
			*seq = ptseq_reset(*seq);
			tot = 0;
			miss = 0;
		}
		count = 0;
		break;
	}
	return 0;
}

/*!
 * \brief Initialize an HMM model.
 *
 * With this function an HMM in initialized with default parameters
 * values. In particular a bounded left right model is creates,
 * probability of observation are set uniform and the initial state if
 * the first with prob = 1. This is an exmple of an 5 state 3 symbols
 * initial model: (x+y=1)
 *
 *       | x y 0 0 0 |
 *       | 0 x y 0 0 |
 *   A = | 0 0 x y 0 |   b = | 1/3 1/3 1/3 |  pi = | 1 0 0 0 0 |
 *       | 0 0 0 0 1 | 
 *
 * \param[in]   number of states
 * \param[in]   number of observation symbols
 * \param[in]   P_ii (prob to stay in the same staty i)
 * \param[in]   P_ij (prob to move to j stay) 
 * \return      HMM model 
 */
CvHMM cvhmm_blr_init (int N, int M, double pii, double pij)
{
	CvHMM mo;
	int i,j;

	mo.N = N;
	mo.M = M;
	mo.A = cvCreateMat(N,N,CV_64FC1);
	mo.b = cvCreateMat(N,M,CV_64FC1);
	mo.pi = cvCreateMat(1,N,CV_64FC1);

	cvZero(mo.A);
	cvZero(mo.pi);
	cvZero(mo.b);

	cvmSet(mo.pi, 0, 0, 1);

	if (N==1) {
		cvmSet(mo.A, 0, 0, 1);
		
	} else {
		for (i=0; i<N; i++) {
			if (i+1!=N) {
				cvmSet(mo.A, i, i, pii);
				cvmSet(mo.A, i, i+1, pij);
			} else {
				cvmSet(mo.A, i, i, 1.0);
			}
		}
	}
	
	for (i=0; i<N; i++) {
		for (j=0; j<M; j++) {
			cvmSet(mo.b, i, j, 1./M);
		}
	}
	
	return mo;
}

/*!
 * \brief Destroy the HMM model (free memory).
 *
 * \param[in]  HMM model
 */ 
void cvhmm_free (CvHMM mo)
{
	cvReleaseMat(&(mo.A));
	cvReleaseMat(&(mo.b));
	cvReleaseMat(&(mo.pi));
}

void cvhmm_print (CvHMM mo)
{
	int i,j;

	printf("pi:\n");
	for (i=0; i<mo.pi->cols; i++) {
		printf("%.2f  ", cvmGet(mo.pi,0,i));
	}

	printf("\n\nA:\n");
	for (i=0; i<mo.A->rows; i++) {
		for (j=0; j<mo.A->cols; j++) {
			printf("%.2f ", cvmGet(mo.A,i,j));
		}
		printf("\n");
	}

	printf("\nb:\n");
	for (i=0; i<mo.b->rows; i++) {
		for (j=0; j<mo.b->cols; j++) {
			printf("%.2f  ", cvmGet(mo.b,i,j));
		}
		printf("\n");
	}
	printf("\n");
}

