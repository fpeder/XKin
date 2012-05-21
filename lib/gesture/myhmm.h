#ifndef _MYHMM_H_
#define _MYHMM_H_

#include <opencv2/core/core_c.h>
#include "ptseq.h"

typedef struct CvHMM {
	int type;
	int N;
	int M;
	CvMat *A;
	CvMat *b;
	CvMat *pi;
} CvHMM;


CvHMM     cvhmm_from_gesture_proto     (const char *infile);
CvHMM     cvhmm_blr_init               (int N, int M, double pii, double pij);
void      cvhmm_free                   (CvHMM mo);
void      cvhmm_print                  (CvHMM mo);
int       cvhmm_classify_gesture       (CvHMM *mo, int num, ptseq seq, int debug);

#endif /* _MYHMM_H_ */
