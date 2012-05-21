#ifndef _RW_H_
#define _RW_H_

#include "ptseq.h"
#include "myhmm.h"

void cvhmm_write (const char *outfile, CvHMM *mo, int num);
CvHMM *cvhmm_read (const char *infile, int *total);
ptseq read_gesture_proto (const char *infile, int *N);
void write_gesture_proto (const char *outfile, CvSeq *seq, int N);


#endif /* _RW_H_ */
