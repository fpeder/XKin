#ifndef _LIBPOSTURE_H_
#define _LIBPOSTURE_H_

enum {
        FD_NUM=8
};

typedef struct CvPostModel {
	int type;
	CvMat *mean;
	CvMat *cov;
} CvPostModel;


CvMat*         get_fourier_descriptors           (CvSeq*);
int            basic_posture_classification      (CvSeq*);
int            advanced_posture_classification   (CvSeq*,CvPostModel*,int);

#endif /* _LIBPOSTURE_H_ */






