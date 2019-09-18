#include "showImage.h"

void showImage(int width, int height, void* data)
{
	IplImage * cvImg;
	cvImg = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 3);
	cvSetData(cvImg, data, width * 3);
	cvNamedWindow("CV", CV_WINDOW_AUTOSIZE);
	cvShowImage("CV", cvImg);
	cvWaitKey(10);
	cvReleaseImageHeader(&cvImg);
}
void showImageMJPEG(int width, int height, void* data)
{
	IplImage * cvImg;
	CvMat CvMatcvmat = cvMat(height, width, CV_8UC3, (void*) data);	//将帧内容赋值给CvMat格式的数据;
	cvImg = cvDecodeImage(&CvMatcvmat, 1);	//解码，这一步将数据转换为IplImage格式
	cvNamedWindow("CV", CV_WINDOW_AUTOSIZE);
	cvShowImage("CV", cvImg);
	cvWaitKey(10);
	cvReleaseImage(&cvImg);
}
