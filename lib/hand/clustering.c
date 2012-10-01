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
 * \file clustreing.c
 * \author Fabrizio Pedersoli
 *
 * Stuff for clustreing the body depth image in: hand, rest of the
 * body.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <opencv2/core/core_c.h>

#include "const.h"
#include "clustering.h"


static int            init_step                 (CvMat*, CvMat*, CvMat*);
static int            assignment_step           (double, CvMat*);
static void           update_step               (double, int, CvMat*, CvMat*);
static void           fill_mat                  (IplImage*, CvMat*);
/* static void           mk_partition              (CvMat*, CvMat*, CvMat*); */
/* static int            get_cluster_var           (CvMat*, CvMat*); */


/*!
 * \brief Compute depth interval defined by the hand. 
 *
 * Starting from the depth body image this function computes the depth
 * interval related to the hand, these two depth values are later used
 * as thresholds for the binarization procedure. This interval is
 * calculated through a K-means clustering of the body image.
 *
 * \param[in]      body depth image
 * \param[out]  hand depth min max values 
 */
void get_hand_interval (IplImage *body, int *interval)
{
	CvMat  *data, *par, *means;
	double var;
	int    min, count = cvCountNonZero(body);

	data  = cvCreateMat(count, 1, CV_32FC1);
	par   = cvCreateMat(count, 1, CV_8UC1);
	means = cvCreateMat(K, 1, CV_32FC1);

	fill_mat(body, data);

	min = kmeans_clustering(data, means, par);

	//var = get_cluster_var(data, par);

	interval[0] = min;
	interval[1] = (int)cvmGet(means, 0, 0); //- var;

	cvReleaseMat(&data);
	cvReleaseMat(&par);
	cvReleaseMat(&means);
}

void get_hand_interval_2 (IplImage *body, int *interval)
{
	CvMat *data, *labels, *means;
	int count;

#define CLUSTERS 2
	
	count = cvCountNonZero(body);
	data = cvCreateMat(count, 1, CV_32FC1);
	labels = cvCreateMat(count, 1, CV_32SC1);
	means = cvCreateMat(CLUSTERS, 1, CV_32FC1);

	fill_mat(body, data);
	cvKMeans2(data, CLUSTERS, labels,
		  cvTermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 10.0),
		  1, 0, 0, means, 0);

	double tmp;
	cvMinMaxLoc(body, &tmp, NULL, NULL, NULL, NULL);
	interval[0] = tmp;
	cvMinMaxLoc(means, &tmp, NULL, NULL, NULL, NULL);
	interval[1] = tmp;
		  
	cvReleaseMat(&data);
	cvReleaseMat(&labels);
}

/*!
 * \brief Compute k-means clustering of the body image.
 *
 * This is an implementation of the K-means algorithm and it is used
 * to detect the hand within the body depth image. Body image is
 * clustered in two class: hand, and the rest of body. Clustering is
 * related only to depth values (scalar).
 *
 * \param[in]     matrix (vector) of depth values to cluster
 * \param[out]    vector of the cluster's means
 * \parma[out]    vector of partitions index
 * \return        min depth value 
 */
int kmeans_clustering (CvMat *data, CvMat *means, CvMat *par)
{
	CvMat *weights = cvCreateMat(K, 1, CV_32FC1);
	int min = init_step(data, means, weights);

	int i;
	for (i=0; i<data->rows; i++) {
		double val = cvmGet(data, i, 0);
		int idx = assignment_step(val, means);

		update_step(val, idx, means, weights);
	}

	//mk_partition(data, par, means);

	return min;
}

/*!
 * \biref Initial step of the K-means algorithm.
 *
 * The initial clusters means are initializied with the minimum and
 * maximum depth value of the body.
 *
 * \param[in]       vector of data
 * \param[in,out]   vecotor of cluster's means
 * \param[in,out]   vecotr of weights (number of elements per cluster)
 * \return          the min depth value
 */
static int init_step (CvMat *data, CvMat *means, CvMat *weights)
{
	double init[2];

	cvMinMaxLoc(data, &(init[0]), &(init[1]), NULL, NULL, NULL);

	int i;
	for (i=0; i<K; i++) {
		cvmSet(means, i, 0, init[i]);
		cvmSet(weights, i, 0, 1);
	}

	return (int)init[0];
}

/*!
 * \brief Assigment step of K-means algorithm.
 *
 * A new data is assigned to the nearest (L_1 dist) cluster.
 *
 * \param[in]   current data depth value 
 * \param[in]   vector cluster's means
 * \return      assigned cluster index 
 */
static int assignment_step (double val, CvMat *means)
{
	int i, j=0;
	double dist, min=1e6;

	for (i=0; i<K; i++) {
		dist = fabs(val - cvmGet(means, i, 0));

		if (dist < min) {
			min = dist;
			j = i;
		}
	}

	return j;
}

/*!
 * \brief Update step of K-measn algorithm.
 *
 * After the assigment of a new data are recomputed the weights
 * vectors and the means one.
 *
 * \param[in]       value of the assigned data 
 * \param[in]       index of the cluster data is assigned 
 * \param[in,out]   vectors of cluster's means 
 * \param[in,out]   vectors of cluster's weights
 */
static void update_step (double val, int idx, CvMat *means, CvMat *weights)
{
	double cur, w;

	cur = cvmGet(means, idx, 0);
	w = cvmGet(weights, idx, 0);

	cur = (cur*w + val)/(++w);

	cvmSet(means, idx, 0, cur);
	cvmSet(weights, idx, 0, w);
}

/* static void mk_partition (CvMat *data, CvMat *par, CvMat *means) */
/* { */
/* 	int i; */
	
/* 	for (i=0; i<data->rows; i++) { */
/* 		int idx; */
/* 		double val = cvmGet(data, i, 0); */

/* 		idx = assignment_step(val, means); */

/* 		(par->data.ptr)[i] = (uint8_t)idx; */
/* 	} */
/* } */

/* static int get_cluster_var (CvMat *data, CvMat *par) */
/* { */
/* 	int i; */
/* 	CvScalar sdv; */

/* 	cvAvgSdv(data, NULL, &sdv, par); */

/* 	return (int)sdv.val[0]; */
/* } */

static void fill_mat (IplImage *img, CvMat *data)
{
	int i,j=0;
	float val;

	for (i=0; i< img->width*img->height; i++) {
		val = (float)((uint8_t*)img->imageData)[i];

		if (val != 0) {
			cvmSet(data, j++, 0, val);
		}
	}
}
