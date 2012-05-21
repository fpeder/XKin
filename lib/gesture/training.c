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
 * \file training.c
 * \author Fabrizio Pedersoli
 *
 * Stuff for train the HMM models. 
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


CvRNG rng_state;

static void  add_awgn (ptseq, ptseq*);


/*!
 * \brief Generate a training set from a prototype sequence.
 *
 * The training set is create adding at each point of the prototype
 * sequence a gaussian noise.
 *
 * \param[in]   gesture prototype
 * \param[in]   number of seq in the training set
 * \return      training set
 */
CvMat* make_training_set (ptseq gesture, int num)
{
	ptseq *tmp;
	CvMat *training;

	rng_state = cvRNG(-1);
	tmp = (ptseq*)malloc(num * sizeof(ptseq));
	
	int i;
	for (i=0; i<num; i++) {
		tmp[i] = ptseq_init();

		add_awgn(gesture, tmp+i);
	}

	training = parametriz_training_set(tmp, num);

	return training;
}

/*!
 * \brief Add gaussian noise to a point sequence. 
 *
 * \param[in]    input point sequence
 * \param[out]   output point sequence (noisy)
 */
static void add_awgn (ptseq proto, ptseq *dst)
{

	int num = proto.ptr->total;
	CvMat *noise = cvCreateMat(num, 2, CV_32FC1);
	//CvMat *ang_bias = cvCreateMat(num, 2, CV_32FC1);

	cvRandArr(&rng_state, noise, CV_RAND_NORMAL,
		  cvScalar(0,0,0,0), cvScalar(XVAR,YVAR,0,0));

	int i;
	for (i=0; i<num; i++) {
		CvPoint p = ptseq_get(proto, i);
		
		p.x += (int)cvmGet(noise, i, 0);
		p.y += (int)cvmGet(noise, i, 1);

		ptseq_add(*dst, p);
	}
}
