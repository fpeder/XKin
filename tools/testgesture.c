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
  
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <libfreenect/libfreenect_sync.h>
#include <libfreenect/libfreenect_cv.h>

#include "../include/libbody.h"
#include "../include/libhand.h"
#include "../include/libposture.h"
#include "../include/libgesture.h"

enum {
        W=640,
        H=480,
        T=20,
};


void        parse_args                      (int, char**);
void        usage                           (void);


char *infile = NULL;


int main (int argc, char *argv[])
{
	IplImage *depth, *body, *hand, *tmp;
	CvHMM *models;
	int num;
	ptseq seq;

	parse_args(argc,argv);

	seq = ptseq_init();
	models = cvhmm_read(infile, &num);

	for (;;) {
		CvSeq *cnt;
		CvPoint cent;
		int z, p, g, k;

		depth = freenect_sync_get_depth_cv(0);
		body = body_detection(depth);
		hand = hand_detection(body, &z);

		if (!get_hand_contour_basic(hand, &cnt, &cent))
			continue;

		if ((p = basic_posture_classification(cnt)) == -1)
			continue;

		draw_classified_hand(cnt, cent, p);

		if(cvhmm_get_gesture_sequence(p, cent, &seq))
			ptseq_draw(seq, 20);

		if ((k = cvWaitKey(T)) == 'q')
			break;
	}

	freenect_sync_stop();

	cvDestroyAllWindows();
	cvReleaseImage(&depth);
	cvReleaseImage(&body);
	cvReleaseImage(&hand);
	
	return 0;
}




void parse_args (int argc, char **argv)
{
	int c;

	opterr=0;
	while ((c = getopt(argc,argv,"i:h")) != -1) {
		switch (c) {
		case 'i':
			infile = optarg;
			break;
		case 'h':
		default:
			usage();
			exit(-1);
		}
	}
	if (infile == NULL) {
		usage();
		exit(-1);
	}
}

void usage (void)
{
	printf("usage: testgesture -i [file] -h\n");
	printf("  -i  gestures models yml file\n");
	printf("  -h  show this message\n");
			
}
