//============================================================================
// Name        : v4l2Sample.cpp
// Author      : yangmi
// Version     :
// Copyright   : Your copyright notice
// Description : v4l2 sample in C++, Ansi-style
//============================================================================

#include <iostream>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include "V4l2Camera.h"
#include "utils.h"

typedef struct resolution
{
	int width;
	int height;
	unsigned int format;
} resolution_t;
resolution_t resols[] =
{
{ 320, 240, V4L2_PIX_FMT_MJPEG },
{ 640, 480, V4L2_PIX_FMT_MJPEG },
{ 1024, 768, V4L2_PIX_FMT_MJPEG },
{ 320, 240, V4L2_PIX_FMT_YUYV },
{ 640, 480, V4L2_PIX_FMT_YUYV },
{ 1024, 768, V4L2_PIX_FMT_YUYV } };

static void saveImage(FILE *fp, const void *p, int size)
{
	printf("saveImage size: %d\n", size);
	fwrite(p, size, 1, fp);
	fflush(fp);
}
void onFrameGrab(v4l2_frame_t *frame, void *ptr)
{
	FILE* fp = (FILE*) ptr;
	if (frame && fp)
	{
		saveImage(fp, frame->data, frame->data_bytes);
	}
}

int grabAndSaveWithTimeoutByCallback(V4l2Camera cam, int renderTime,v4l2_frame_callback_t *cb)
{
	FILE* fp = NULL;
	if ((fp = fopen("/tmp/out.yuv", "w")) == NULL)
	{
		perror("Creat file failed");
		exit(0);
	}
	cam.startStreaming(cb,fp);
	sleep(renderTime);
	cam.stopStreaming();
	return 0;
}
int grabAndSaveWithTimeout(V4l2Camera cam, int renderTime)
{
	int elapseT = 0;
	long long lastT = 0;
	v4l2_frame_t *frame = NULL;
	FILE* fp = NULL;
	if ((fp = fopen("/tmp/out.yuv", "w")) == NULL)
	{
		perror("Creat file failed");
		exit(0);
	}
	while (1)
	{
		long long nowT = getSecOfTime();
		if (nowT - lastT >= 1 && lastT > 0)
		{
			elapseT += (nowT - lastT);
			if (elapseT > renderTime && renderTime > 0)
			{
				break;
			}
			printf("%d secs passed \n", elapseT);
		}
		lastT = nowT;

		int res = cam.grabFrame(&frame);

		if (res == 0)
		{
			saveImage(fp, frame->data, frame->data_bytes);
		}

		if (frame != NULL)
		{
			cam.freeFrame(frame);
			frame = NULL;
		}
	}
	return 0;
}

void rk3288Test()
{
	//DBG("create camera object with:/dev/video1\n");
	V4l2Camera webcam("/dev/video1");
	/*int index = 0;
	 int w = resols[index].width;
	 int h = resols[index].height;
	 unsigned fmt = resols[index].format;*/
	//DBG("fmt:%x\n", V4L2_PIX_FMT_NV12);
	if (webcam.openCameraDevice(640, 480, V4L2_PIX_FMT_NV12) == 0)
	{
		//grabAndSaveWithTimeout(webcam, 2);
		grabAndSaveWithTimeoutByCallback(webcam, 2,onFrameGrab);
	}
	webcam.closeCameraDevice();
}
int main()
{
	rk3288Test();
	return 0;
}
