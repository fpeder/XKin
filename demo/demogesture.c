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
#include <pthread.h>
#include <string.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <libfreenect/libfreenect_sync.h>
#include <libfreenect/libfreenect_cv.h>

#include "../include/libbody.h"
#include "../include/libhand.h"
#include "../include/libposture.h"
#include "../include/libgesture.h"

#define SCREENX 1200
#define SCREENY 800
#define F 1.25
#define WIN_TYPE CV_GUI_NORMAL | CV_WINDOW_AUTOSIZE

enum {
        W=640,
        H=480,
	GW=128,
	GH=128,
        T=20,
	MAX_STR_LEN=50,
	NUM=3,
	UP=0,
	DOWN=1,
	LEFT=3,
	RIGHT=2,
};

int update = 1;
char *infile = NULL;
char *dir = NULL;

typedef struct GState {
	int zoom;
	int rot;
} GState; 

char*          fix_file_path           (char*);
IplImage*      draw_depth_hand         (CvSeq*, int);
void           gallery_init            (IplImage**, GState*);
void           gallery_free            (IplImage**);
IplImage*      draw_current_image      (IplImage*, GState);
int            buffered_classfy        (int);
void           draw_trajectory         (IplImage*,CvSeq*);
void           parse_args              (int,char**);
void           usage                   (void);


int main (int argc, char *argv[])
{
	IplImage *gallery[NUM], *color;
	CvHMM *models;
	GState state[NUM];
	int num, idx=0, zoom=0;
	ptseq seq;
	const char *win_gallery = "gallery";
	const char *win_color = "color image";
	const char *win_hand = "depth hand";

	parse_args(argc,argv);

	seq = ptseq_init();
	models = cvhmm_read(infile, &num);
	color = cvCreateImage(cvSize(W, H), 8, 3);
	gallery_init(gallery, state);

	cvNamedWindow(win_gallery, WIN_TYPE);
	cvNamedWindow(win_color, WIN_TYPE);
	cvNamedWindow(win_hand, WIN_TYPE);
	cvMoveWindow(win_gallery, 0, 0);
	cvMoveWindow(win_color, SCREENX-W/2, 0);
	cvMoveWindow(win_hand, SCREENX-W/2, H/2);
	cvResizeWindow(win_color, W/2, H/2);
	
	for (;;) {
		IplImage *depth, *tmp, *body, *hand;
		IplImage *a, *b;
		CvSeq *cnt;
		CvPoint cent;
		int z, p, k; 
		
		tmp = freenect_sync_get_rgb_cv(0);
		cvCvtColor(tmp, color, CV_RGB2BGR);
		depth = freenect_sync_get_depth_cv(0);
		
		body = body_detection(depth);
		hand = hand_detection(body, &z);

		if (!get_hand_contour_basic(hand, &cnt, &cent))
			continue;

		if ((p = basic_posture_classification(cnt)) == -1)
			continue;

		if (cvhmm_get_gesture_sequence(p, cent, &seq)) {
			
			int g = cvhmm_classify_gesture(models, num, seq, 0);

			switch (g) {
			case LEFT:
				idx = --idx <= 0 ? 0 : idx;
				break;
			case RIGHT:
				idx = ++idx >= NUM ? NUM-1 : idx;
				break;
			case UP:
				++state[idx].zoom;
				break;
			case DOWN:
				state[idx].zoom = --state[idx].zoom <= 0 ? 0 :
				state[idx].zoom;
				break;
			}

			printf("state: idx=%d, zoom=%d\n", idx, state[idx].zoom);
			update = 1;
		} else {
			update = 0;
		}

		a = draw_depth_hand(cnt, p);
		b = draw_current_image(gallery[idx], state[idx]);

		cvShowImage(win_color, color);
		cvResizeWindow(win_color, W/2, H/2);
		cvShowImage(win_hand, a);
		cvResizeWindow(win_hand, W/2, H/2);
		cvShowImage(win_gallery, b);
		
		if ((k = cvWaitKey(T)) == 'q')
			break;
	}

	freenect_sync_stop();

	cvDestroyAllWindows();
	gallery_free(gallery);

	return 0;
}

char *fix_file_path (char *name)
{
	static char file[MAX_STR_LEN];
	int last;

	last = strlen(dir)-1;
	if (dir[last] == '/')
		dir[last] = '\0';
	strcpy(file, dir);
	strcat(file, "/");
	strcat(file, name);
	
	return file;
}

void gallery_init (IplImage **gallery, GState *state)
{
	int i;
	char *name;
	char *img[] = {"lena.tif", "peppers.tif", "sail.tif"};

	for (i=0; i<NUM; i++) {
		name = fix_file_path(img[i]);
		gallery[i] = cvLoadImage(name, CV_LOAD_IMAGE_COLOR);
		assert(gallery[i]);
		state[i].zoom = 0;
	}
}

void gallery_free (IplImage **gallery)
{
	int i;

	for (i=0; i<NUM; i++) {
		cvReleaseImage(&(gallery[i]));
	}
}

IplImage* draw_depth_hand (CvSeq *cnt, int type)
{
	static IplImage *img = NULL;
	CvScalar color[] = {CV_RGB(255,0,0), CV_RGB(0,255,0)};

	if (img == NULL)
		img = cvCreateImage(cvSize(W, H), 8, 3);

	cvZero(img);
	cvDrawContours(img, cnt, color[type], CV_RGB(0,0,255), 0,
		       CV_FILLED, 8, cvPoint(0,0));

	cvFlip(img, NULL, 1);

	return img;
}


IplImage* draw_current_image (IplImage *img, GState state)
{
	static IplImage *tmp = NULL;
	int w, h;
	float f = F;

	if(!update)
		return tmp;
	
	if (tmp != NULL) 
		cvReleaseImage(&tmp);
	
	if (state.zoom >= 1) {
		w = GW * (f + state.zoom);
		h = GH * (f + state.zoom);
	} else {
		w=GW;
		h=GH;
	}
	
	tmp = cvCreateImage(cvSize(w, h), 8, 3);
	cvResize(img, tmp, CV_INTER_LINEAR);

	return tmp;
}

void parse_args (int argc, char **argv)
{
	int c;

	opterr=0;
	while ((c = getopt(argc,argv,"i:d:h")) != -1) {
		switch (c) {
		case 'i':
			infile = optarg;
			break;
		case 'd':
			dir = optarg;
			break;
		case 'h':
		default:
			usage();
			exit(-1);
		}
	}
	if (infile == NULL || dir == NULL) {
		usage();
		exit(-1);
	}
}

void usage (void)
{
	printf("usage: demogesture -i [file] -d [dir] [-h]\n");
	printf("  -i  gestures models yml file\n");
	printf("  -d  imgs direcotry\n");
	printf("  -h  show this message\n");
}
