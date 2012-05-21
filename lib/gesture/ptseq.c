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
 * \file ptseq.c
 * \author Fabrizio Pedersoli 
 *
 * This file implements an easy to use interface for handling sequence
 * of point (pointseq).
 *
 */

#include "config.h"

#include <stdio.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>

#include "const.h"
#include "ptseq.h"

/*!
 * \brief Initialize a pointseq.
 * 
 */
ptseq ptseq_init ()
{
	ptseq seq;
	
	seq.storage = cvCreateMemStorage(0);
	seq.ptr = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq),
			       sizeof(CvPoint), seq.storage);
	return seq;
}

/*!
 * \brief Destroy a pointseq.
 *
 * \param[in]  pointseq 
 */
void ptseq_free (ptseq seq)
{
	cvReleaseMemStorage(&(seq.storage));
}

/*!
 * \brief Convert a cvseq to ptseq.
 *
 * \paran[in]  opencv seqence pointer
 * \param[in]  opnecv sequence storage
 * \return     ptseq 
 */
ptseq ptseq_from_cvseq (CvSeq *in, CvMemStorage *str)
{
	ptseq out;

	out = ptseq_init();
	out.ptr = cvCloneSeq(in, out.storage);

	if (str != NULL) {
		cvReleaseMemStorage(&str);
	}

	return out;
}

/*!
 * \brief Add (push back) a point to a ptseq.
 *
 * \param[in,out]   pointseq
 * \return          point
 */
void ptseq_add (ptseq seq, CvPoint p)
{
	cvSeqPush(seq.ptr, &p);
}

void ptseq_remove_tail (ptseq seq, int num)
{
	int i;

	for (i=0; i<num; i++) {
		cvSeqPop(seq.ptr, NULL);
	}
}

/*!
 * \brief Get a point in a specific position from a ptseq.
 *
 * \param[in]   pointseq
 * \param[in]   position
 * \return      point 
 */
CvPoint ptseq_get (ptseq seq, int idx)
{
	CvPoint p;

	p = *(CvPoint*)cvGetSeqElem(seq.ptr, idx);

	return p;
}

/*!
 * \brief Get a *point (pointer) in a specific position from a ptseq.
 *
 * \param[in]   pointseq
 * \param[in]   position
 * \return      pointer to a point
 */
CvPoint *ptseq_get_ptr (ptseq seq, int idx)
{
	CvPoint *p;

	p = (CvPoint*)cvGetSeqElem(seq.ptr, idx);

	return p;
}

/*!
 * \brief Get the length of a ptseq.
 *
 * \param[in]  pointseq
 * \return     number of points inside
 */
int ptseq_len (ptseq seq)
{
	return seq.ptr->total;
}

/*!
 * \brief Conert a pointseqt to a vector.
 *
 * \param[in]  pointseq
 * \return     vector
 */
CvMat *ptseq_to_mat (ptseq seq)
{
	int num = seq.ptr->total;
	CvMat *mat = cvCreateMat(num, 2, CV_32FC1);

	int i;
	for (i=0; i<num; i++) {
		CvPoint p = ptseq_get(seq, i);

		cvmSet(mat, i, 0, p.x);
		cvmSet(mat, i, 1, p.y);
	}

	return mat;
}

/*!
 * \brief Print (stdout) the pointseq contentes.
 *
 * \param[in]  pointseq
 */
void ptseq_print (ptseq seq)
{
	int i;

	for (i=0; i<seq.ptr->total; i++) {
		CvPoint p;

		p = ptseq_get(seq, i);
		printf("%4d %4d\n", p.x, p.y);
	}
	
}

/*! 
 * \brief Reinitialise the pointseq (clear)
 *
 * \param[in]  pointseq 
 * \return     pointseq (empty)
 */
ptseq ptseq_reset (ptseq seq)
{
	ptseq_free(seq);
	
	return ptseq_init();
}

/*!
 * \brief Print (image) the pointseq content.
 *
 * This is a graphical representation of a pointseq, points are drawn
 * on a image.
 *
 * \param[in]   pointseq
 * \param[in]   time option visualization
  */
void ptseq_draw (ptseq seq, int flag)
{
	const char win[] = "point seq";
	IplImage *img = cvCreateImage(cvSize(WIDTH,HEIGHT), 8, 3);

	cvZero(img);
	
	int i;
	for (i=0; i<seq.ptr->total; i++) {
		CvPoint p = ptseq_get(seq, i);

		cvCircle(img, p, i ? R : 10, i ? CV_RGB(0,255,0) : CV_RGB(255,0,0),
			 -1, 8, 0);
	}

	cvShowImage(win, img);
		
	if (flag==1) 
		cvWaitKey(0);
	else if (flag >50)
		cvWaitKey(flag);
	
	cvReleaseImage(&img);
}

