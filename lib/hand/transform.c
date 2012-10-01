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
 * \file transform.c
 * \author Fabrizio Pedersoli
 *
 * This file contains all the necessary stuff for mapping pixels from
 * depth to color. See [0] for a more complete description. The
 * transformation is characterized by the camera intrisic parameteres.
 *
 * [0] http://nicolas.burrus.name/index.php/Research/KinectCalibration
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>

#include "const.h"
#include "transform.h"

#define XOFF 20 //20
#define YOFF 20 //20

const calibParams depthcal = { 5.9421434211923247e+02,
			       5.9104053696870778e+02,
                               3.3930780975300314e+02,
			       2.4273913761751615e+02,
                               -2.6386489753128833e-01,
			       9.9966832163729757e-01,
                               9.9966832163729757e-01,
			       5.0350940090814270e-03,
                               -1.3053628089976321e+00 };

const calibParams rgbcal = { 5.2921508098293293e+02,
			     5.2556393630057437e+02,
                             3.2894272028759258e+02,
			     2.6748068171871557e+02,
                             2.6451622333009589e-01,
			     -8.3990749424620825e-01,
                             -1.9922302173693159e-03,
			     1.4371995932897616e-03,
                             9.1192465078713847e-01 };

const double T[] = { 1.9985242312092553e-02,
		     -7.4423738761617583e-04,
		     -1.0916736334336222e-02 };

const double R[][3] = { 9.9984628826577793e-01,
			1.2635359098409581e-03,
			-1.7487233004436643e-02,
                        -1.4779096108364480e-03,
			9.9992385683542895e-01,
			-1.2251380107679535e-02,
                        1.7470421412464927e-02,
			1.2275341476520762e-02,
			9.9977202419716948e-01 };


static CvRect      map_depth_bbox_to_rgb      (CvRect,double);
static CvPoint     map_depth_point_to_rgb     (CvPoint,double);


/*!
 * \brief Get hand's bounding box in color. 
 *
 * From the hand's depth contour (basic contour) it is calculated its
 * bounding box, then it is computed its equivalent in the color
 * image. The transformation consist in mapping the 4 point that
 * define the bounding box.  It is also added an offset an offset in
 * order to get a bit larger region of the hand.  
 *
 * \param[in]   hand's depth contour
 * \param[in]   hand's depth value
 * \return      rect that defines the bounding box in color image
 */
CvRect get_rgb_hand_bbox_from_depth (CvSeq *depth_cnt, int z)
{
	CvRect depth_bbox, rgb_bbox;

	depth_bbox = cvBoundingRect(depth_cnt,0);
	rgb_bbox = map_depth_bbox_to_rgb(depth_bbox, z);

	rgb_bbox.x -= XOFF;
	rgb_bbox.y -= YOFF;
	rgb_bbox.width += XOFF;
	rgb_bbox.height += YOFF;
	
	return rgb_bbox;
}

/*!
 * \brief Map a bounding box from depth to color.
 *
 * \param[in]  hand's bounding box in depth
 * \param[in]  depth hand value
 * \return     hand's bounding box in color 
 */
static CvRect map_depth_bbox_to_rgb (CvRect r, double z)
{
        CvPoint tl, tr, bl, br;
        CvRect out;
        
        tl = cvPoint(r.x, r.y);
        tr = cvPoint(r.x + r.width, r.y);
        bl = cvPoint(r.x, r.y + r.height);
        br = cvPoint(r.x + r.width, + r.y + r.height);

        tl = map_depth_point_to_rgb(tl, z);
        tr = map_depth_point_to_rgb(tr, z);
        bl = map_depth_point_to_rgb(bl, z);
        br = map_depth_point_to_rgb(br, z);

        out.x = tl.x;
        out.y = tl.y;
        out.width = abs(tl.x - tr.x);
        out.height = abs(tl.y - bl.y);
        
        return out;
}

/*!
 * \brief Single point maps from depth to color.
 *
 * \param[in]  depth point
 * \param[in]  depth value
 * \return     color point
 */
static CvPoint map_depth_point_to_rgb (CvPoint p, double z)
{
	int i, j;
        CvPoint out;
        double tmp1[3], tmp2[3];
        
        tmp1[0] = ((double)p.x - depthcal.cx) * z / depthcal.fx + T[0];
        tmp1[1] = ((double)p.y - depthcal.cy) * z / depthcal.fy + T[1];
        tmp1[2] = z + T[2];

        for (i=0; i<3; i++) {
                double sum=0;

                for (j=0; j<3; j++) {
                        sum += R[i][j] * tmp1[j];
                }
                tmp2[i] = sum;
                sum=0;
        }

        out.x = (int)((tmp2[0] * rgbcal.fx / tmp2[2]) + rgbcal.cx);
        out.y = (int)((tmp2[1] * rgbcal.fy / tmp2[2]) + rgbcal.cy);

        return out;
}
