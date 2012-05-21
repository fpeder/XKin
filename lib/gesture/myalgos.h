#ifndef _MYALGOS_H_
#define _MYALGOS_H_

#include <opencv2/core/core_c.h>

#include "myhmm.h"

#define MAX_ITER    10
#define THRESH      1E-4
#define EPS         2.2204E-16


double cvhmm_loglik       (CvHMM *mo, CvMat *O);
void   cvhmm_reestimate   (CvHMM *mo, CvMat *O);


#endif /* _MYALGOS_H_ */
