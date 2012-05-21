#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

typedef struct calibParams {
        double fx, fy;
        double cx, cy;
        double k1, k2;
        double p1, p2;
        double k3;
} calibParams;


CvRect        get_rgb_hand_bbox_from_depth       (CvSeq*,int);


#endif /* _TRANSFORM_H_ */

