#ifndef _CLUSTERING_H_
#define _CLUSTERING_H_

void              get_hand_interval            (IplImage*, int*);
void              get_hand_interval_2          (IplImage *body, int *interval);
int               kmeans_clustering            (CvMat*, CvMat*, CvMat*);

#endif /* _CLUSTERING_H_ */
