#ifndef _PTSEQ_H_
#define _PTSEQ_H_

#include <opencv2/core/core_c.h>

/*!
 * \brief ptseq structure.
 *
 * pointseq is the base element for handling in a easier way opencv
 * sequence of point. It encloses both pointer and storage in a single
 * componentes. 
 */
typedef struct ptseq {
	CvSeq *ptr;
	CvMemStorage *storage;
} ptseq;

ptseq        ptseq_init               (void);
void         ptseq_free               (ptseq);
ptseq        ptseq_reset              (ptseq);
ptseq        ptseq_from_cvseq         (CvSeq*, CvMemStorage*);
void         ptseq_add                (ptseq, CvPoint);
void         ptseq_remove_tail        (ptseq, int);
CvPoint      ptseq_get                (ptseq, int);
CvPoint*     ptseq_get_ptr            (ptseq, int);
CvMat*       ptseq_to_mat             (ptseq);
void         ptseq_print              (ptseq);
void         ptseq_draw               (ptseq, int);
int          ptseq_len                (ptseq);


#endif /* _PTSEQ_H_ */
