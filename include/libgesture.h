#ifndef _LIBGESTURE_H_
#define _LIBGESTURE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <opencv2/core/core_c.h>

typedef struct ptseq{
	CvSeq *ptr;
	CvMemStorage *storage;
} ptseq;

ptseq        ptseq_init                 (void);
void         ptseq_free                 (ptseq);
ptseq        ptseq_reset                (ptseq);
ptseq        ptseq_from_cvseq           (CvSeq*, CvMemStorage*);
void         ptseq_add                  (ptseq, CvPoint);
CvPoint      ptseq_get                  (ptseq, int);
CvPoint*     ptseq_get_ptr              (ptseq, int);
CvMat*       ptseq_to_mat               (ptseq);
void         ptseq_print                (ptseq);
void         ptseq_draw                 (ptseq, int);
void         ptseq_remove_tail          (ptseq, int);
CvMat*       ptseq_parametriz           (ptseq);
int          ptseq_len                  (ptseq);

void      write_gesture_proto    (const char *outfile, CvSeq *seq, int N);
ptseq     read_gesture_proto     (const char *infile, int *N);

typedef struct CvHMM {
	int type;
	int N;
	int M;
	CvMat *A;
	CvMat *b;
	CvMat *pi;
} CvHMM;

CvHMM       cvhmm_from_gesture_proto     (const char *infile);
int         cvhmm_classify_gesture       (CvHMM *mo, int num, ptseq seq, FILE *pf);
int         cvhmm_get_gesture_sequence   (int posture, CvPoint pt, ptseq *seq);
CvHMM       cvhmm_blr_init               (int N, int M, double pii, double pij);
void        cvhmm_free                   (CvHMM mo);
void        cvhmm_print                  (CvHMM mo);
double      cvhmm_loglik                 (CvHMM *mo, CvMat *O);
void        cvhmm_reestimate             (CvHMM *mo, CvMat *O);
void        cvhmm_write                  (const char *outfile, CvHMM *mo, int num);
CvHMM*      cvhmm_read                   (const char *infile, int *total);

#ifdef __cplusplus
}
#endif 

#endif /* _LIBGESTURE_H_ */






