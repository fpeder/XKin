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
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <libfreenect/libfreenect_sync.h>
#include <libfreenect/libfreenect_cv.h>

#include "../include/libbody.h"
#include "../include/libhand.h"
#include "../include/libposture.h"

enum {
        W = 640,
        H = 480,
        T = 20,
	SCREENW = 1200,
	SCREENH = 800,
	OFFSETX = 400,
	OFFSETY = 200,
	NUM = 3,
	MAX_STR_LEN = 50,
	STOP_COUNT = 20,
	PAPER = 0,
	ROCK = 1,
	SCISSOR = 2
};

char *infile = NULL;
char *dir = NULL;

void       init_imgs                (IplImage**, IplImage**);
void       display_result           (IplImage**, int);
char*      fix_file_path            (char*);
int        load_posture_models      (char*, CvPostModel**);
void       usage                    (void);
void       parse_args               (int, char**);


int main (int argc, char *argv[])
{
	CvPostModel *models;
	IplImage *rgb, *depth, *body, *hand;
	IplImage *imgs[NUM], *emo[2];
	int num, count = 0, pp = -1, wincount = 0, loosecount = 0;
	char *win_user = "you";
	char *win_comp = "computer";
	char *win_resu = "results";
	char *win_rgb = "color";
	
	parse_args(argc,argv);

	srand(time(NULL));
	init_imgs(imgs, emo);
	rgb = cvCreateImage(cvSize(W,H), 8, 3);
	num = load_posture_models(infile, &models);

	printf("--------- ROCK - SCISSOR - PAPAER -----------\n");
	printf("move your hand to start the game\n");

	cvNamedWindow(win_user, CV_GUI_NORMAL|CV_WINDOW_AUTOSIZE);
	cvNamedWindow(win_comp, CV_GUI_NORMAL|CV_WINDOW_AUTOSIZE);
	cvNamedWindow(win_rgb, CV_GUI_NORMAL);
	cvMoveWindow(win_rgb,  SCREENW-OFFSETX, 2*OFFSETY);
	cvMoveWindow(win_user, SCREENW-OFFSETX, 0);
	cvMoveWindow(win_comp, SCREENW-OFFSETX, OFFSETY);
	
	for (;;) {
		IplImage *tmp;
		CvSeq *cnt;
		CvPoint cent;
		int p, z, k, win = 0;

		tmp = freenect_sync_get_rgb_cv(0);
		cvCvtColor(tmp, rgb, CV_BGR2RGB);
		depth = freenect_sync_get_depth_cv(0);
		
		body = body_detection(depth);
		hand = hand_detection(body, &z);

		if (!get_hand_contour_advanced(hand, rgb, z, &cnt, &cent))
			continue;

		if ((p = advanced_posture_classification(cnt, models, num)) == -1)
			continue;

		if (p == pp) {
			if (count++ == STOP_COUNT) {
				int r = rand() % NUM;
				
				cvShowImage(win_user, imgs[p]);
				cvShowImage(win_comp, imgs[r]);

				switch (p) {
				case PAPER:
					if (r == ROCK) {
						win = 1;
						wincount++;
					} else
						loosecount++;
					break;
				case ROCK :
					if (r == SCISSOR) {
						win = 1;
						wincount++;
					} else
						loosecount++;
					break;
				case SCISSOR :
					if (r == PAPER) {
						win = 1;
						wincount++;
					} else 
						loosecount++;
					break;
				default :
					win = 0;
					break;
				}

				display_result(emo, win);
			}
		} else 
			count = 0;
		pp = p;
		
		draw_classified_hand(cnt, cent, p);
		cvResizeWindow(win_rgb, W/2, H/2);
		cvShowImage(win_rgb, rgb);
		
		if ((k = cvWaitKey(T)) == 'q')
			break;
	}

	freenect_sync_stop();

	cvDestroyAllWindows();
	cvReleaseImage(&rgb);
	
	return 0;
}

void init_imgs (IplImage **imgs, IplImage **emo)
{
	int i, last;
	char *pics[] = {"paper.png", "rock.png", "scissor.png"};
	char *asd[] = {"win.png", "loose.png"};
	char *file;

	for (i=0; i<NUM; i++) {
		file = fix_file_path(pics[i]);
		imgs[i] = cvLoadImage(file, CV_LOAD_IMAGE_UNCHANGED);
		assert(imgs[i]);
	}

	for (i=0; i<2; i++) {
		file = fix_file_path(asd[i]);
		emo[i] = cvLoadImage(file, CV_LOAD_IMAGE_UNCHANGED);
		assert(emo[i]);
	}
}

void display_result (IplImage **emo, int win)
{
	char *win_resu = "result";

	cvNamedWindow(win_resu, CV_GUI_NORMAL|CV_WINDOW_AUTOSIZE);
	cvMoveWindow(win_resu, SCREENW-OFFSETX/2, 0);

	if (win)
		cvShowImage(win_resu, emo[0]);
	else 
		cvShowImage(win_resu, emo[1]);

	cvWaitKey(1000);
	cvDestroyWindow(win_resu);
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

int load_posture_models (char *infile, CvPostModel **p)
{
	CvFileStorage *fs;
	CvFileNode *node;
	CvMat *tmp;
	char name[] = "posture-00";
	int i, total;

	fs = cvOpenFileStorage(infile, NULL, CV_STORAGE_READ, NULL);
	assert(fs);
	total = cvReadIntByName(fs, NULL, "total", 0);
	*p = (CvPostModel*)malloc(sizeof(CvPostModel)*total);

	for (i=0; i<total; i++) {
		node = cvGetFileNodeByName(fs, NULL, name);
		tmp = (CvMat*)cvReadByName(fs, node, "mean", NULL);
		(*p)[i].mean = cvClone(tmp);
		tmp = (CvMat*)cvReadByName(fs, node, "cov", NULL);
		(*p)[i].cov = cvClone(tmp);

		if (++name[9] == ':') {
			name[9] = '0';
			name[8]++;
		}
	}

	cvReleaseFileStorage(&fs);

	return total;
}


void parse_args (int argc, char **argv)
{
	int c;

	opterr=0;
	while ((c = getopt(argc, argv, "i:d:h")) != -1) {
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
	printf("usage: demoposture -i [file] -d [dir] [-h]\n");
	printf("  -i  posture models file\n");
	printf("  -d  imgs dir path\n");
	printf("  -h  show this message\n");
}


