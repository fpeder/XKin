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
 * \file rw.c
 * \author Fabrizio Pedersoli
 *
 * This file contains function for reading/writing out HMM models and
 * gesture prototype.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <opencv2/core/core_c.h>

#include "myhmm.h"
#include "rw.h"

/*!
 * \brief Write an array of HMM models to file.
 *
 * Models parameters (N, A, b, pi) are stored according yaml syntax.
 *
 * \param[in]   output file
 * \param[in]   HMM models array
 * \param[in]   number of array
 */
void cvhmm_write (const char *outfile, CvHMM *mo, int num)
{
	int i;
	char name[]="hmm-00";
	CvFileStorage *fs;

	fs = cvOpenFileStorage(outfile, NULL, CV_STORAGE_WRITE, NULL);
	assert(fs);

	for (i=0; i<num; i++) {
		cvStartWriteStruct(fs, name, CV_NODE_MAP, NULL, cvAttrList(0,0));
		cvWriteInt(fs, "N", mo[i].N);
		cvWrite(fs, "pi", mo[i].pi, cvAttrList(0,0));
		cvWrite(fs, "A",  mo[i].A,  cvAttrList(0,0));
		cvWrite(fs, "b",  mo[i].b,  cvAttrList(0,0));
		cvEndWriteStruct(fs);

		if (++name[5] == ':' ) {
			name[5] = '0';
			name[4]++;
		}
	}

	cvWriteInt(fs, "total", num);
	cvReleaseFileStorage(&fs);
}


/*!
 * \brief Read an HMM models from file.
 *
 * \param[in]   input file
 * \param[out]  number of models in the file. 
 */
CvHMM *cvhmm_read (const char *infile, int *total)
{
	int i,N;
	char name[]="hmm-00"; //100 gesture are engough for me
	CvFileStorage *fs;
	CvFileNode *node;
	CvHMM *mo;
	
	fs = cvOpenFileStorage(infile, NULL, CV_STORAGE_READ, NULL);
	assert(fs);
	*total = cvReadIntByName(fs, NULL, "total", 0);

	mo = (CvHMM*)malloc(sizeof(CvHMM) * (*total));
	
	for (i=0; i<*total; i++) {
		CvMat *tmp;
		int N;
		
		node = cvGetFileNodeByName(fs, NULL, name);
		N = cvReadIntByName(fs, node, "N", 0);
		mo[i].N = N;
		tmp = (CvMat*)cvReadByName(fs, node, "A", NULL);
		mo[i].A = cvCloneMat(tmp);
		tmp = (CvMat*)cvReadByName(fs, node, "b", NULL);
		mo[i].b = cvCloneMat(tmp);
		tmp = (CvMat*)cvReadByName(fs, node, "pi", NULL);
		mo[i].pi = cvCloneMat(tmp);
		
		if (++name[5] == ':' ) {
			name[5] = '0';
			name[4]++;
		}

		cvReleaseMat(&tmp);
	}
	
	cvReleaseFileStorage(&fs);

	return mo;
}

ptseq read_gesture_proto (const char *infile, int *N)
{
	CvSeq *tmp;
	CvFileStorage *fs;
	ptseq proto;
	
	fs = cvOpenFileStorage(infile, NULL, CV_STORAGE_READ, NULL);
	assert(fs);

	if (N != NULL)
		*N = cvReadIntByName(fs, 0, "N", 1);

	tmp = (CvSeq*)cvReadByName(fs, 0, "seq", NULL);
	proto = ptseq_from_cvseq(tmp, NULL);

	cvReleaseFileStorage(&fs);
	
	return proto;
}

void write_gesture_proto (const char *outfile, CvSeq *seq, int N)
{
	CvFileStorage *fs;

	fs = cvOpenFileStorage(outfile, NULL, CV_STORAGE_WRITE, NULL);
	assert(fs);

	cvWriteInt(fs, "N", N);
	cvWrite(fs, "seq", seq, cvAttrList(0,0));

	cvReleaseFileStorage(&fs);
}

