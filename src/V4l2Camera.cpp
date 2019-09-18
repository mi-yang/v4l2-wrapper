/*
 * V4l2Camera.cpp
 *
 *  Created on: Sep 15, 2019
 *      Author: yang mi
 */

#include "V4l2Camera.h"
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

V4l2Camera::V4l2Camera(const char* device, int width, int height) :
		m_width(width), m_height(height)
{
	m_user_buf = NULL;
	m_user_cb = NULL;
	m_user_ptr = NULL;
	m_fps = 30;
	m_format = V4L2_PIX_FMT_YUYV;
	m_fd = -1;
	m_isStreaming = false;
	m_worker = -1;
	memset(m_device, 0, 50);
	int len = strlen(device);
	if (len > 50)
		len = 50;
	memcpy(m_device, device, len);
}

V4l2Camera::~V4l2Camera()
{
	m_user_cb = NULL;
	m_user_ptr = NULL;
}
void V4l2Camera::freeFrame(v4l2_frame_t *frame)
{
	if (frame)
	{
		if (frame->data_bytes > 0)
			free(frame->data);
		free(frame);
	}
}
void V4l2Camera::stopCapturing()
{
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(m_isStreaming == false)
		return;
	m_isStreaming = false;
	if (-1 == ioctl(m_fd, VIDIOC_STREAMOFF, &type))
	{
		perror("Fail to ioctl 'VIDIOC_STREAMOFF'");
		return;
	}
	return;
}
void V4l2Camera::workerCleanup(void *arg)
{
	//DBG("workerCleanup \n");
	//V4l2Camera *pCam = (V4l2Camera *) arg;
}
int V4l2Camera::grabRoutine()
{
	int res = 0;
	v4l2_frame_t *frame = NULL;
	while (m_isStreaming)
	{
		//pthread_testcancel();
		//DBG("grab.....%d\n",m_isStreaming);
		if (m_user_cb != NULL)
		{
			res = grabFrame(&frame);
			if (res == 0)
			{
				m_user_cb(frame, m_user_ptr);
			}
			else
			{
				DBG("grab frame error \n");
			}
			if (frame != NULL)
			{
				freeFrame(frame);
				frame = NULL;
			}
		}
	}
	//DBG("grab routine end \n");
	return res;
}
void* V4l2Camera::workerThread(void *arg)
{
	V4l2Camera* pCam = (V4l2Camera *) arg;
	pthread_cleanup_push(workerCleanup, pCam); //注册线程处理函数
	pCam->grabRoutine();
	pthread_cleanup_pop(1); //线程资源释放，参数为非0
	return NULL;
}
int V4l2Camera::startStreaming(v4l2_frame_callback_t *cb, void *user_ptr)
{
	m_user_cb = cb;
	m_user_ptr = user_ptr;
	startCapturing();
	pthread_create(&m_worker, 0, workerThread, this);
	//pthread_detach(m_worker);
	return m_worker;
}
int V4l2Camera::stopStreaming()
{
	 //DBG("stop stream\n");
	 stopCapturing();
	 //pthread_cancel(m_worker);
	 void *status = NULL;
	 pthread_join(m_worker,&status);
	 m_user_cb = NULL;
	 m_user_ptr = NULL;
	 return 0;
}

int V4l2Camera::startCapturing()
{
	unsigned int i;
	enum v4l2_buf_type type;
	if(m_isStreaming == true)
		return 0;
	m_isStreaming = true;
	//将申请的内核缓冲区放入一个队列中
	for (i = 0; i < NB_BUFFER; i++)
	{
		struct v4l2_buffer buf;

		bzero(&buf, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (-1 == ioctl(m_fd, VIDIOC_QBUF, &buf))
		{
			perror("Fail to ioctl 'VIDIOC_QBUF'");
			return -1;
		}
	}

	//开始采集数据
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl(m_fd, VIDIOC_STREAMON, &type))
	{
		printf("i = %d.\n", i);
		perror("Fail to ioctl 'VIDIOC_STREAMON'");
		return -1;
	}
	return 0;
}
int V4l2Camera::initMmap()
{
	int i = 0;
	struct v4l2_requestbuffers reqbuf;

	bzero(&reqbuf, sizeof(reqbuf));
	reqbuf.count = NB_BUFFER;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	//申请视频缓冲区(这个缓冲区位于内核空间，需要通过mmap映射)
	//这一步操作可能会修改reqbuf.count的值，修改为实际成功申请缓冲区个数
	if (-1 == ioctl(m_fd, VIDIOC_REQBUFS, &reqbuf))
	{
		perror("Fail to ioctl 'VIDIOC_REQBUFS'");
		return -1;
	}

	//n_buffer = reqbuf.count;
	m_user_buf = (user_buffer_t*) calloc(reqbuf.count, sizeof(*m_user_buf));
	if (m_user_buf == NULL)
	{
		fprintf(stderr, "Out of memory\n");
		return -1;
	}

	//将内核缓冲区映射到用户进程空间
	for (i = 0; i < (int)reqbuf.count; i++)
	{
		struct v4l2_buffer buf;
		bzero(&buf, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		//查询申请到内核缓冲区的信息
		if (-1 == ioctl(m_fd, VIDIOC_QUERYBUF, &buf))
		{
			perror("Fail to ioctl : VIDIOC_QUERYBUF");
			return -1;
			//exit(EXIT_FAILURE);
		}

		m_user_buf[i].length = buf.length;
		m_user_buf[i].start = mmap(
		NULL,/*start anywhere*/
		buf.length,
		PROT_READ | PROT_WRITE,
		MAP_SHARED, m_fd, buf.m.offset);

		if (MAP_FAILED == m_user_buf[i].start)
		{
			perror("Fail to mmap");
			return -1;
		}
	}

	return 0;
}
void V4l2Camera::uninitDevice()
{
	unsigned int i;

	for (i = 0; i < NB_BUFFER; i++)
	{
		if (-1 == munmap(m_user_buf[i].start, m_user_buf[i].length))
		{
			return;
		}
	}
	free(m_user_buf);
	return;
}

void V4l2Camera::closeCameraDevice()
{
	stopCapturing();
	uninitDevice();
	if (-1 == close(m_fd))
	{
		perror("Fail to close fd");
		return;
	}
	return;
}
int V4l2Camera::initDevice()
{
	struct v4l2_fmtdesc fmt;
	struct v4l2_capability cap;
	struct v4l2_format stream_fmt;
	int ret;

	//当前视频设备支持的视频格式
	memset(&fmt, 0, sizeof(fmt));
	fmt.index = 0;
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	//查询视频设备驱动的功能
	ret = ioctl(m_fd, VIDIOC_QUERYCAP, &cap);
	if (ret < 0)
	{
		perror("FAIL to ioctl VIDIOC_QUERYCAP");
		return -1;
		//exit(EXIT_FAILURE);
	}

	//判断是否是一个视频捕捉设备
	if (!(cap.capabilities & V4L2_BUF_TYPE_VIDEO_CAPTURE))
	{
		printf("The Current device is not a video capture device\n");
		return -1;
		//exit(EXIT_FAILURE);
	}

	//判断是否支持视频流形式
	if (!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		printf("The Current device does not support streaming i/o\n");
		return -1;
		//exit(EXIT_FAILURE);
	}

	//设置摄像头采集数据格式，如设置采集数据的
	//长,宽，图像格式(JPEG,YUYV,MJPEG等格式)
	stream_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	stream_fmt.fmt.pix.width = m_width;
	stream_fmt.fmt.pix.height = m_height;
	stream_fmt.fmt.pix.pixelformat = m_format;
	stream_fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (-1 == ioctl(m_fd, VIDIOC_S_FMT, &stream_fmt))
	{
		perror("Fail to ioctl set format");
		return -1;
	}

	//初始化视频采集方式(mmap)
	initMmap();

	return 0;
}
int V4l2Camera::openCameraDevice(int width, int height, uint32_t format)
{
	struct v4l2_capability cap;
	if (width != m_width && height != m_height)
	{
		m_width = width;
		m_height = height;
	}

	m_format = format;
	if (m_format == V4L2_PIX_FMT_YUYV)
	{
		DBG("resolution:%dx%d YUYV\n", m_width, m_height);
	}
	else
	{
		DBG("resolution:%dx%d MJPEG\n", m_width, m_height);
	}
	if ((m_fd = open(m_device, O_RDWR | O_NONBLOCK)) < 0)
	{
		DBG("Fail to open\n");
		return -1;
	}
	memset(&cap, 0, sizeof cap);
	if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) < 0)
	{
		printf("Error opening device /dev/video0 : unable to query device.\n");
		close(m_fd);
		return -1;
	}
	if (initDevice() < 0)
	{
		return -1;
	}

	if (startCapturing() < 0)
		return -1;

	return 0;
}
v4l2_frame_t* V4l2Camera::allocateFrame(int data_bytes)
{
	v4l2_frame_t *frame = (v4l2_frame_t *) malloc(sizeof(*frame));
	if (!frame)
		return NULL;
	memset(frame, 0, sizeof(*frame));
	//frame->library_owns_data = 1;

	if (data_bytes > 0)
	{
		frame->data_bytes = data_bytes;
		frame->data = malloc(data_bytes);

		if (!frame->data)
		{
			free(frame);
			return NULL;
		}
	}
	return frame;
}
void V4l2Camera::createFrame(char* data, int len, v4l2_frame_t** frame)
{
	v4l2_frame_t * f = NULL;
	f = allocateFrame(m_width * m_height * 3);
	memcpy(f->data, data, len);
	f->width = m_width;
	f->height = m_height;
	//f->frame_format = V4L2_PIX_FMT_YUYV;
	f->data_bytes = len;
	*frame = f;
}
int V4l2Camera::grabFrame(v4l2_frame_t** frame)
{
	fd_set fds;
	struct timeval tv;
	struct v4l2_buffer buf;
	v4l2_frame_t *f = NULL;
	unsigned int seq = 0;
	struct timeval timestamp;
	FD_ZERO(&fds);
	FD_SET(m_fd, &fds);
	/* Timeout. */
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	int r = select(m_fd + 1, &fds, NULL, NULL, &tv);
	if (-1 == r)
	{
		printf("select error \n");
		if (EINTR == errno)
		{
			return -1;
		}
	}
	bzero(&buf, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	//从队列中取缓冲区
	if (-1 == ioctl(m_fd, VIDIOC_DQBUF, &buf))
	{
		DBG("Fail to ioctl 'VIDIOC_DQBUF'\n");
		return -1;
	}
	seq = buf.sequence;
	timestamp = buf.timestamp;
	createFrame((char*) m_user_buf[buf.index].start, buf.bytesused, &f);
	if (-1 == ioctl(m_fd, VIDIOC_QBUF, &buf))
	{
		perror("Fail to ioctl 'VIDIOC_QBUF'");
		return -1;
	}
	f->sequence = seq;
	f->frame_format = m_format;
	f->capture_time = timestamp;
	*frame = f;
	return 0;
}
