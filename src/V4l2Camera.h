/*
 * V4l2Camera.h
 *
 *  Created on: Sep 15, 2019
 *      Author: yang mi
 */

#ifndef V4L2CAMERA_H_
#define V4L2CAMERA_H_

#include <stdint.h>
#include <pthread.h>
#include <linux/videodev2.h>

#define NB_BUFFER 4
#define DEBUG
#ifdef DEBUG
#define DBG(...) fprintf(stderr, " DBG(%s, %s(), %d): ", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__)
#else
#define DBG(...)
#endif


typedef struct v4l2_frame
{
	/** Image data for this frame */
	void *data;
	/** Size of image data buffer */
	int data_bytes;
	/** Width of image in pixels */
	uint32_t width;
	/** Height of image in pixels */
	uint32_t height;
	/** Pixel data format */
	uint32_t frame_format;
	/** Number of bytes per horizontal line (undefined for compressed format) */
	int step;
	/** Frame number (may skip, but is strictly monotonically increasing) */
	uint32_t sequence;
	/** Estimate of system time when the device started capturing the image */
	struct timeval capture_time;
	/** Estimate of system time when the device finished receiving the image */
} v4l2_frame_t;


typedef void(v4l2_frame_callback_t)(struct v4l2_frame *frame, void *user_ptr);

typedef struct user_buffer
{
	void *start;
	int length;
} user_buffer_t;
#ifdef __cplusplus
class V4l2Camera
{
public:
	V4l2Camera(const char* device = "/dev/video0", int width = 640, int height = 480);
	int openCameraDevice(int width, int height, uint32_t format);
	int grabFrame(v4l2_frame_t** frame);
	void closeCameraDevice();
	void freeFrame(v4l2_frame_t *frame);
	v4l2_frame_t * allocateFrame(int data_bytes);
	int startStreaming(v4l2_frame_callback_t *cb, void *user_ptr);
	int stopStreaming();
	virtual ~V4l2Camera();

private:
	int initDevice();
	int initMmap();
	int startCapturing();
	void stopCapturing();
	void uninitDevice();
	int grabRoutine();
	void createFrame(char* data, int len, v4l2_frame_t** frame);
	user_buffer_t* m_user_buf;
	static void workerCleanup(void *arg);//线程清理函数
	static void* workerThread(void *arg);//线程回调函数
private:
	char m_device[50];
	int m_fd;
	int m_width;
	int m_height;
	int m_fps;
	bool m_isStreaming;
	v4l2_frame_callback_t *m_user_cb;
	void *m_user_ptr;
	uint32_t m_format;
	pthread_t m_worker;
};
#endif
#endif /* V4L2CAMERA_H_ */
