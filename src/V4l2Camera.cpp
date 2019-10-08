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

#define CLEAR(x) memset(&(x), 0, sizeof(x))
static int xioctl(int fh, int request, void *arg)
{
	int r;
	do
	{
		r = ioctl(fh, request, arg);
	} while (-1 == r && EINTR == errno);
	return r;
}
V4l2Camera::V4l2Camera(const char* device, int width, int height) :
		m_width(width), m_height(height)
{
	m_user_buf = NULL;
	m_user_cb = NULL;
	m_user_ptr = NULL;
	//m_fps = 30;
	m_format = V4L2_PIX_FMT_YUYV;
	m_fd = -1;
	m_isStreaming = false;
	m_worker = -1;
	m_buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
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
	type = m_buf_type;
	if (m_isStreaming == false)
		return;
	m_isStreaming = false;
	if (-1 == xioctl(m_fd, VIDIOC_STREAMOFF, &type))
	{
		perror("Fail to ioctl 'VIDIOC_STREAMOFF'");
		return;
	}
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
	return 0;
}
int V4l2Camera::stopStreaming()
{
	//DBG("stop stream\n");
	stopCapturing();
	//pthread_cancel(m_worker);
	void *status = NULL;
	pthread_join(m_worker, &status);
	m_user_cb = NULL;
	m_user_ptr = NULL;
	return 0;
}

int V4l2Camera::startCapturing()
{
	DBG("startCapturing \n");
	unsigned int i;
	enum v4l2_buf_type type;
	for (i = 0; i < NB_BUFFER; i++)
	{
		struct v4l2_buffer buf;
		CLEAR(buf);
		buf.type = m_buf_type;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == m_buf_type)
		{
			struct v4l2_plane planes[FMT_NUM_PLANES];
			buf.m.planes = planes;
			buf.length = FMT_NUM_PLANES;
		}
		if (-1 == xioctl(m_fd, VIDIOC_QBUF, &buf))
		{
			printf("VIDIOC_QBUF\n");
			return -1;
		}

	}
	type = m_buf_type;
	if (-1 == xioctl(m_fd, VIDIOC_STREAMON, &type))
	{
		printf("VIDIOC_STREAMON \n");
		return -1;
	}

	m_isStreaming = true;
	return 0;
}
int V4l2Camera::initMmap()
{
	struct v4l2_requestbuffers req;
	CLEAR(req);

	req.count = NB_BUFFER;
	req.type = m_buf_type;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(m_fd, VIDIOC_REQBUFS, &req))
	{
		printf("%s does not support "
				"memory mapping\n", m_device);
		return -1;
	}

	if (req.count < 2)
	{
		printf("Insufficient buffer memory on %s\n", m_device);
		return -1;
	}

	m_user_buf = (user_buffer_t*) calloc(req.count, sizeof(*m_user_buf));

	if (!m_user_buf)
	{
		printf("Out of memory\n");
		return -1;
	}
	int n_buffers = 0;
	for (n_buffers = 0; n_buffers < req.count;n_buffers++)
	{
		struct v4l2_buffer buf;
		struct v4l2_plane planes[FMT_NUM_PLANES];
		CLEAR(buf);
		CLEAR(planes);

		buf.type = m_buf_type;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == m_buf_type)
		{
			buf.m.planes = planes;
			buf.length = FMT_NUM_PLANES;
		}

		if (-1 == xioctl(m_fd, VIDIOC_QUERYBUF, &buf))
		{
			printf("VIDIOC_QUERYBUF");
			return -1;
		}

		if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == m_buf_type)
		{
			m_user_buf[n_buffers].length = buf.m.planes[0].length;
			m_user_buf[n_buffers].start = mmap(NULL /* start anywhere */,
					buf.m.planes[0].length,
					PROT_READ | PROT_WRITE /* required */,
					MAP_SHARED /* recommended */, m_fd,
					buf.m.planes[0].m.mem_offset);
		}
		else
		{
			m_user_buf[n_buffers].length = buf.length;
			m_user_buf[n_buffers].start = mmap(NULL /* start anywhere */,
					buf.length, PROT_READ | PROT_WRITE /* required */,
					MAP_SHARED /* recommended */, m_fd, buf.m.offset);
		}

		if (MAP_FAILED == m_user_buf[n_buffers].start)
			return -1;
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
	DBG("init_device\n");
	struct v4l2_capability cap;
	struct v4l2_format fmt;

	if (-1 == xioctl(m_fd, VIDIOC_QUERYCAP, &cap))
	{
		perror("FAIL to ioctl VIDIOC_QUERYCAP");
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
			&& !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE))
	{
		fprintf(stderr, "%s is not a video capture device, capabilities: %x\n",
				m_device, cap.capabilities);
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		printf("The Current device does not support streaming i/o\n");
		return -1;
	}

	if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
		m_buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	else if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
		m_buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

	CLEAR(fmt);
	fmt.type = m_buf_type;
	fmt.fmt.pix.width = m_width;
	fmt.fmt.pix.height = m_height;
	fmt.fmt.pix.pixelformat = m_format;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (-1 == xioctl(m_fd, VIDIOC_S_FMT, &fmt))
	{
		perror("Fail to ioctl set format");
		return -1;
	}

	DBG("buf type %d\n", m_buf_type);
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
	if ((m_fd = open(m_device, O_RDWR | O_NONBLOCK)) < 0)
	{
		DBG("Fail to open\n");
		return -1;
	}
	memset(&cap, 0, sizeof cap);
	if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) < 0)
	{
		printf("Error opening device %s : unable to query device.\n", m_device);
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
	f->data_bytes = len;
	*frame = f;
}
#if 0
/* return >= 0 ok otherwhise -1 */
int V4l2Camera::isv4l2Control(int control, struct v4l2_queryctrl *queryctrl)
{
	int err = 0;
	queryctrl->id = control;
	if ((err = ioctl(m_fd, VIDIOC_QUERYCTRL, queryctrl)) < 0)
	{
		DBG("ioctl querycontrol error %d \n", errno);
	}
	else if (queryctrl->flags & V4L2_CTRL_FLAG_DISABLED)
	{
		DBG("control %s disabled \n", (char * ) queryctrl->name);
	}
	else if (queryctrl->flags & V4L2_CTRL_TYPE_BOOLEAN)
	{
		return 1;
	}
	else if (queryctrl->type & V4L2_CTRL_TYPE_INTEGER)
	{
		return 0;
	}
	else
	{
		DBG("contol %s unsupported  \n", (char * ) queryctrl->name);
	}
	return -1;
}

/*==================================================================
 * Function  : v4l2GetControl
 * Description : get v4l2 control value
 * Input Para  : para1:control id,para2 control value
 * Output Para :
 * Return Value: -1 error -2 not support 0 success
 ==================================================================*/
int V4l2Camera::v4l2GetControl(int control,control_value_t* ctr)
{
	struct v4l2_queryctrl queryctrl;
	struct v4l2_control control_s;
	int err;
	if (isv4l2Control(control, &queryctrl) < 0)
	return -2;
	control_s.id = control;
	if ((err = ioctl(m_fd, VIDIOC_G_CTRL, &control_s)) < 0)
	{
		DBG("ioctl get control error\n");
		return -1;
	}
	(*ctr).value = control_s.value;
	(*ctr).def = queryctrl.default_value;
	(*ctr).max = queryctrl.maximum;
	(*ctr).min = queryctrl.minimum;
	(*ctr).step = queryctrl.step;
	return 0;
}
/*==================================================================
 * Function  : getFps
 * Description : get stream fps
 * Input Para  : fps
 * Output Para :
 * Return Value: -1 error 0 success
 ==================================================================*/
int V4l2Camera::getFps(int* fps)
{
	int ret = 0;
	struct v4l2_streamparm *setfps = NULL;
	setfps = (struct v4l2_streamparm *) calloc(1,
			sizeof(struct v4l2_streamparm));
	memset(setfps, 0, sizeof(struct v4l2_streamparm));
	setfps->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(m_fd, VIDIOC_G_PARM, setfps);
	if (ret == 0)
	{
		*fps = setfps->parm.capture.timeperframe.denominator;
	}
	else
	{
		perror("Unable to query that the FPS change is supported\n");
		ret = -1;
	}
	if (setfps != NULL)
	{
		free(setfps);
		setfps = NULL;
	}
	return ret;
}
/*==================================================================
 * Function  : setFps
 * Description : set stream fps
 * Input Para  : fps
 * Output Para :
 * Return Value: -1 error 0 success
 ==================================================================*/
int V4l2Camera::setFps(int fps)
{
	struct v4l2_streamparm *setfps = NULL;
	int ret = 0;
	setfps = (struct v4l2_streamparm *) calloc(1,
			sizeof(struct v4l2_streamparm));
	memset(setfps, 0, sizeof(struct v4l2_streamparm));
	setfps->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	/*
	 * first query streaming parameters to determine that the FPS selection is supported
	 */
	do
	{
		ret = ioctl(m_fd, VIDIOC_G_PARM, setfps);
		if (ret == 0)
		{
			if (setfps->parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
			{
				memset(setfps, 0, sizeof(struct v4l2_streamparm));
				setfps->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				setfps->parm.capture.timeperframe.numerator = 1;
				setfps->parm.capture.timeperframe.denominator = fps;
				ret = ioctl(m_fd, VIDIOC_S_PARM, setfps);
				if (ret)
				{
					perror("Unable to set the FPS\n");
					ret = -1;
					break;
				}
				else
				{
					if (fps != setfps->parm.capture.timeperframe.denominator)
					{
						printf("FPS coerced ......: from %d to %d\n", fps,
								setfps->parm.capture.timeperframe.denominator);
						ret = -1;
						break;
					}
					ret = 0;
					break;
				}
			}
			else
			{
				perror(
						"Setting FPS on the capture device is not supported, fallback to software framedropping\n");
				return -1;
			}
		}
		else
		{
			perror("Unable to query that the FPS change is supported\n");
			ret= -1;
			break;
		}
	}while (0);

	if(setfps != NULL)
	{
		free(setfps);
		setfps = NULL;
	}
	return ret;
}
/*==================================================================
 * Function  : v4l2SetControl
 * Description : set v4l2 control value
 * Input Para  : para1:control id para2:contorl value
 * Output Para :
 * Return Value: -1 error 0 success -2 not support
 ==================================================================*/
int V4l2Camera::v4l2SetControl(int control, int value)
{
	struct v4l2_control control_s;
	struct v4l2_queryctrl queryctrl;
	int min, max, step, val_def;
	int err;
	if (isv4l2Control(control, &queryctrl) < 0)
	return -2;
	min = queryctrl.minimum;
	max = queryctrl.maximum;
	step = queryctrl.step;
	val_def = queryctrl.default_value;
	if ((value >= min) && (value <= max))
	{
		control_s.id = control;
		control_s.value = value;
		if ((err = ioctl(m_fd, VIDIOC_S_CTRL, &control_s)) < 0)
		{
			DBG("ioctl set control error\n");
			return -1;
		}
	}
	return 0;
}
#endif
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
	buf.type = m_buf_type;
	buf.memory = V4L2_MEMORY_MMAP;

	if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == m_buf_type)
	{
		struct v4l2_plane planes[FMT_NUM_PLANES];
		buf.m.planes = planes;
		buf.length = FMT_NUM_PLANES;
	}

	//从队列中取缓冲区
	if (-1 == ioctl(m_fd, VIDIOC_DQBUF, &buf))
	{
		DBG("Fail to ioctl 'VIDIOC_DQBUF'\n");
		return -1;
	}
	seq = buf.sequence;
	timestamp = buf.timestamp;

	if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == m_buf_type)
	{
		//DBG("multi-planes bytesused %d\n", buf.m.planes[0].bytesused);
		createFrame((char*) m_user_buf[buf.index].start,
				buf.m.planes[0].bytesused, &f);
	}
	else
	{
		//DBG("bytesused %d\n", buf.bytesused);
		createFrame((char*) m_user_buf[buf.index].start, buf.bytesused, &f);
	}

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
