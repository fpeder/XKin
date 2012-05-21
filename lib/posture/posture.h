#ifndef _POSTURE_H_
#define _POSTURE_H_

/*!
 * \brief Posture model structure.
 */
typedef struct CvPostModel {
	int type;      //!< model id 
	CvMat *mean;   //!< mean vector 
	CvMat *cov;    //!< covariance matrix 
} CvPostModel;

int        basic_posture_classification        (CvSeq*);
int        advanced_posture_classification     (CvSeq*, CvPostModel*, int);

#endif /* _POSTURE_H_ */

