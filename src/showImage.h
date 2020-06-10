#ifndef SHOWIMAGE_H_
#define SHOWIMAGE_H_
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"


void showImage(int width, int height, void* data);
void showImageMJPEG(int width, int height, void* data);
#endif
