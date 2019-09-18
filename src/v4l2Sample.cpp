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
#include "showImage.h"
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

int grabAndRender(V4l2Camera cam, int renderTime, unsigned int fmt)
{
	int elapseT = 0;
	long long lastT = 0;
	v4l2_frame_t *frame = NULL;
	while (1)
	{
		long long nowT = getSecOfTime();
		v4l2_frame_t *bgr = NULL;
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
			if (fmt == V4L2_PIX_FMT_MJPEG)
			{
				showImageMJPEG(frame->width, frame->height, frame->data);
			}
			else
			{
				bgr = cam.allocateFrame(frame->width * frame->height * 3);
				if (!bgr)
				{
					printf("unable to allocate bgr frame!");
					return 0;
				}

				/* Do the BGR conversion */
				int ret = uvc_any2bgr(frame, bgr);
				if (ret)
				{
					cam.freeFrame(bgr);
					return -1;
				}
				showImage(bgr->width, bgr->height, bgr->data);
			}

		}

		if (frame != NULL)
		{
			cam.freeFrame(frame);
			frame = NULL;
		}
		if (bgr != NULL)
		{
			cam.freeFrame(bgr);
			bgr = NULL;
		}
	}
	return 0;
}
void formatTest()
{
	V4l2Camera webcam("/dev/video0");
	int index = 0;
	while (1)
	{
		int w = resols[index].width;
		int h = resols[index].height;
		unsigned fmt = resols[index].format;
		webcam.openCameraDevice(w, h, fmt);
		grabAndRender(webcam, 5, fmt);
		webcam.closeCameraDevice();
		index++;
		if (index >= 6)
		{
			index = 0;
		}
	}
}

void cb(v4l2_frame_t *frame, void *ptr)
{
	//DBG("callback seq:%d\n",frame->sequence);
	//DBG("callback start\n");
	v4l2_frame_t *bgr = NULL;
	V4l2Camera* pCam = (V4l2Camera*) ptr;
	if (frame->frame_format == V4L2_PIX_FMT_MJPEG)
	{
		showImageMJPEG(frame->width, frame->height, frame->data);
	}
	else
	{
		bgr = pCam->allocateFrame(frame->width * frame->height * 3);
		if (!bgr)
		{
			printf("unable to allocate bgr frame!");
			return;
		}

		/* Do the BGR conversion */
		int ret = uvc_any2bgr(frame, bgr);
		if (ret)
		{
			pCam->freeFrame(bgr);
			return;
		}
		showImage(bgr->width, bgr->height, bgr->data);
	}

	if (bgr != NULL)
	{
		pCam->freeFrame(bgr);
		bgr = NULL;
	}
	//DBG("callback end seq:%d\n",frame->sequence);
}
void formatTest2(int strOnTime, int strOffTime)
{
	V4l2Camera webcam("/dev/video0");
	int startIndex = 0;
	int index = 0;
	while (1)
	{
		int w = resols[index + startIndex].width;
		int h = resols[index + startIndex].height;
		unsigned fmt = resols[index + startIndex].format;
		webcam.openCameraDevice(w, h, fmt);
		webcam.startStreaming(cb, &webcam);
		sleep(strOnTime);
		webcam.stopStreaming();
		webcam.closeCameraDevice();
		sleep(strOffTime);
		index++;
		if (index >= 6)
			index = 0;
	}
}
int main()
{
#if 0
	formatTest2(5, 0);
#else
	formatTest();
#endif
	return 0;
}
