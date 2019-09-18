#ifndef UTILS_H_
#define UTILS_H_
#include "V4l2Camera.h"

int uvc_yuyv2bgr(v4l2_frame_t *in, v4l2_frame_t *out);
int uvc_any2bgr(v4l2_frame_t *in, v4l2_frame_t *out);
long long getSecOfTime();
#endif
