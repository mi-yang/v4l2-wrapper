#include "utils.h"


static inline unsigned char sat(int i)
{
	return (unsigned char) (i >= 255 ? 255 : (i < 0 ? 0 : i));
}
#define IYUYV2BGR_2(pyuv, pbgr) { \
    int r = (22987 * ((pyuv)[3] - 128)) >> 14; \
    int g = (-5636 * ((pyuv)[1] - 128) - 11698 * ((pyuv)[3] - 128)) >> 14; \
    int b = (29049 * ((pyuv)[1] - 128)) >> 14; \
    (pbgr)[0] = sat(*(pyuv) + b); \
    (pbgr)[1] = sat(*(pyuv) + g); \
    (pbgr)[2] = sat(*(pyuv) + r); \
    (pbgr)[3] = sat((pyuv)[2] + b); \
    (pbgr)[4] = sat((pyuv)[2] + g); \
    (pbgr)[5] = sat((pyuv)[2] + r); \
    }

#define IYUYV2BGR_16(pyuv, pbgr) IYUYV2BGR_8(pyuv, pbgr); IYUYV2BGR_8(pyuv + 16, pbgr + 24);
#define IYUYV2BGR_8(pyuv, pbgr) IYUYV2BGR_4(pyuv, pbgr); IYUYV2BGR_4(pyuv + 8, pbgr + 12);
#define IYUYV2BGR_4(pyuv, pbgr) IYUYV2BGR_2(pyuv, pbgr); IYUYV2BGR_2(pyuv + 4, pbgr + 6);

int uvc_yuyv2bgr(v4l2_frame_t *in, v4l2_frame_t *out)
{
	if (in->frame_format != V4L2_PIX_FMT_YUYV)
		return -1;

	//if (uvc_ensure_frame_size(out, in->width * in->height * 3) < 0)
	//	return -1;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = 0;
	//out->step = in->width * 3;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	//out->capture_time_finished = in->capture_time_finished;
	//out->source = in->source;

	unsigned char *pyuv = (unsigned char *) in->data;
	unsigned char *pbgr = (unsigned char *) out->data;
	unsigned char *pbgr_end = pbgr + out->data_bytes;

	while (pbgr < pbgr_end)
	{
		IYUYV2BGR_8(pyuv, pbgr);

		pbgr += 3 * 8;
		pyuv += 2 * 8;
	}

	return 0;
}
int uvc_any2bgr(v4l2_frame_t *in, v4l2_frame_t *out)
{
	switch (in->frame_format)
	{
	case V4L2_PIX_FMT_YUYV:
		return uvc_yuyv2bgr(in, out);
	case V4L2_PIX_FMT_MJPEG:
	default:
		return -1;
	}
}
long long getSecOfTime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec);
}
