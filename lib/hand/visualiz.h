#ifndef _VISUALIZ_H_
#define _VISUALIZ_H_


void          draw_contour                   (CvSeq*);
void          draw_point_sequence            (CvSeq*);
void          draw_cvxhull_and_cvxdefects    (CvSeq*, CvSeq*);
void          draw_histogram                 (CvMat*);
void          print_mat                      (const CvMat*);
void          draw_detected_hand             (CvSeq*,CvPoint,int);


#endif /* _VISUALIZ_H_ */

