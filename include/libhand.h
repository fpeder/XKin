#ifndef _LIBHAND_H_
#define _LIBHAND_H_

#ifdef __cplusplus
extern "C" {
#endif

IplImage*      hand_detection                (IplImage*,int*);
int            get_hand_contour_basic        (IplImage*, CvSeq**, CvPoint*);
int            get_hand_contour_advanced     (IplImage*, IplImage*, int,
					      CvSeq**, CvPoint*);

void           draw_detected_hand             (CvSeq*,CvPoint,int);
void           draw_contour                   (CvSeq*); 
/* void           draw_point_sequence            (CvSeq*); */
/* void           draw_cvxhull_and_cvxdefects    (CvSeq*, CvSeq*); */
/* void           draw_histogram                 (CvMat*); */
/* void           print_mat                      (const CvMat*); */
/* void           print_fdesc                    (double*) */;

#ifdef __cplusplus	
}
#endif

#endif /* _LIBHAND_H_ */
