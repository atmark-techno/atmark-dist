/*
 * UVC gadget test application
 *
 * Copyright (C) 2010 Ideas on board SPRL <laurent.pinchart@ideasonboard.com>
 * Copyright (C) 2012 Atmark techno, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 */

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <linux/usb/ch9.h>
#include <linux/usb/video.h>
#include <linux/videodev2.h>
#include <linux/usbdevice_fs.h>
#include "uvc.h"

#define VERSION_STR "v0.1 (2012.12.12)"

#define DEFAULT_CAPTURE_DEVICE	"/dev/video1"
#define DEFAULT_OUTPUT_DEVICE	"/dev/video0"

enum {
	xLOGERR = 0,
	xLOGINFO,
	xLOGWARN,
	xLOGDBG,
};

static int log_level = xLOGWARN;
#define xprint(level, args...) \
	do { if ((level) <= log_level) { fprintf((level) <= xLOGERR ? stderr : stdout, args); } } while (0);

#define CLAMP(val, min, max) ({			\
	typeof(val) __val = (val);		\
	typeof(min) __min = (min);		\
	typeof(max) __max = (max);		\
	(void) (&__val == &__min);		\
	(void) (&__val == &__max);		\
	__val = __val < __min ? __min: __val;	\
	__val > __max ? __max: __val; })

#define ARRAY_SIZE(a) ((sizeof(a) / sizeof(a[0])))

#define NR_VIDEO_BUF (4)

enum {
	VIDEO_STREAM_OFF = 0,
	VIDEO_STREAM_ON,
};

struct uvc_gadget_device {
	enum v4l2_buf_type type;
	int fd;

	unsigned int fcc;
	unsigned int width;
	unsigned int height;

	void **buf;
	unsigned int nbufs;
	unsigned int bufsize;
};

struct uvc_gadget {
	struct uvc_gadget_device *out;

	struct uvc_streaming_control probe;
	struct uvc_streaming_control commit;

	int control;

	/* YUYV source */
	struct uvc_gadget_device *in;
};

/*
 * lock
 */
#define APP_LOCK_FILE	"/var/lock/uvc-gadget"
static int app_lock(void)
{
	return mkdir(APP_LOCK_FILE, 0644);
}

static void app_unlock(void)
{
	remove(APP_LOCK_FILE);
}

/*
 * systemcall wrapper
 */
static int
xioctl(int fd, int request, void *arg)
{
	int ret;

	do {
		ret = ioctl(fd, request, arg);
	} while ((ret < 0) && (errno == EINTR));

	return ret;
}

/*
 * ioctrl wrapper
 */
static inline int
v4l2_dequeue_buf(int fd, struct v4l2_buffer *buf)
{
	int ret;

	ret = xioctl(fd, VIDIOC_DQBUF, buf);
	if (ret < 0)
		xprint(xLOGERR, "Unable to dequeue buffer: %s\n", strerror(errno));

	return ret;
}

static inline int
v4l2_queue_buf(int fd, struct v4l2_buffer *buf)
{
	int ret;

	ret = xioctl(fd, VIDIOC_QBUF, buf);
	if (ret < 0)
		xprint(xLOGERR, "Unable to queue buffer: %s\n", strerror(errno));

	return ret;
}

static int
v4l2_dequeue_latest_buf(int fd, struct v4l2_buffer *buf)
{
	struct v4l2_buffer tmp[NR_VIDEO_BUF];
	int count;
	int ret;

	for (count = 0; count < NR_VIDEO_BUF; count++) {
		memcpy(&tmp[count], buf, sizeof(struct v4l2_buffer));
		ret = xioctl(fd, VIDIOC_DQBUF, &tmp[count]);
		if (ret < 0)
			break;
		if (count > 0) {
			ret = v4l2_queue_buf(fd, &tmp[count - 1]);
			if (ret < 0)
				return ret;
		}
	}

	if (count) {
		memcpy(buf, &tmp[count - 1], sizeof(struct v4l2_buffer));
		return 0;
	}

	return ret;
}

static inline int
v4l2_dequeue_event(int fd, struct v4l2_event *event)
{
	int ret;

	ret = xioctl(fd, VIDIOC_DQEVENT, event);
	if (ret < 0)
		xprint(xLOGERR, "Unable to dequeue event: %s\n", strerror(errno));

	return ret;
}

static inline int
v4l2_request_buffer(int fd, struct v4l2_requestbuffers *rbuf)
{
	int ret;
	unsigned int nbufs;

	nbufs = rbuf->count;

	ret = xioctl(fd, VIDIOC_REQBUFS, rbuf);
	if (ret < 0)
		xprint(xLOGERR, "Unable to send response: %s\n", strerror(errno));

	if (rbuf->count != nbufs) {
		xprint(xLOGERR, "rbuf->count: %d nbufs: %d\n",
		       rbuf->count, nbufs);
		ret = -1;
	}

	return ret;
}

static inline int
v4l2_query_buffer(int fd, struct v4l2_buffer *buf)
{
	int ret;

	ret = xioctl(fd, VIDIOC_QUERYBUF, buf);
	if (ret < 0)
		xprint(xLOGERR, "Unable to query buffer: %s\n", strerror(errno));

	return ret;
}

static inline int
v4l2_query_capture(int fd, struct v4l2_capability *cap)
{
	int ret;

	ret = xioctl(fd, VIDIOC_QUERYCAP, cap);
	if (ret < 0)
		xprint(xLOGERR, "Unable to query capture: %s\n", strerror(errno));

	return ret;
}

static inline int
v4l2_subscribe_event(int fd, struct v4l2_event_subscription *sub)
{
	int ret;

	ret = xioctl(fd, VIDIOC_SUBSCRIBE_EVENT, sub);
	if (ret < 0)
		xprint(xLOGERR, "Unable to subscribe event: %s\n", strerror(errno));

	return ret;
}

static inline int
v4l2_stream_on(int fd, int type)
{
	int ret;

	ret = xioctl(fd, VIDIOC_STREAMON, &type);
	if (ret < 0)
		xprint(xLOGERR, "Unable to start video stream: %s\n", strerror(errno));

	return ret;
}

static inline int
v4l2_stream_off(int fd, int type)
{
	int ret;

	ret = xioctl(fd, VIDIOC_STREAMOFF, &type);
	if (ret < 0)
		xprint(xLOGERR, "Unable to stop video stream: %s\n", strerror(errno));

	return ret;
}

static inline int
v4l2_set_format(int fd, struct v4l2_format *fmt)
{
	int ret;

	ret = xioctl(fd, VIDIOC_S_FMT, fmt);
	if (ret < 0)
		xprint(xLOGERR, "Unable to set format: %s\n", strerror(errno));

	return ret;
}

static inline int
uvc_send_response(int fd, struct uvc_request_data *resp)
{
	int ret;

	ret = xioctl(fd, UVCIOC_SEND_RESPONSE, resp);
	if (ret < 0)
		xprint(xLOGERR, "Unable to send response: %s\n", strerror(errno));

	return ret;
}

/*
 * Video streaming
 */
static int
uvc_gadget_fill_buffer(struct uvc_gadget *gadget, struct v4l2_buffer *outbuf)
{
	struct v4l2_buffer inbuf;
	uint8_t *out = gadget->out->buf[outbuf->index];
	uint8_t *in;
	unsigned int width, height;
	unsigned int size;
	int ret;

	width = gadget->out->width;
	height = gadget->out->height;

	switch (gadget->out->fcc) {
	case V4L2_PIX_FMT_YUYV:
		size = width * height * 2;

		memset(&inbuf, 0, sizeof(struct v4l2_buffer));
		inbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		inbuf.memory = V4L2_MEMORY_MMAP;
		ret = v4l2_dequeue_latest_buf(gadget->in->fd, &inbuf);
		if (ret < 0)
			return ret;

		in = gadget->in->buf[inbuf.index];
		memcpy(out, in, size);

		ret = v4l2_queue_buf(gadget->in->fd, &inbuf);
		if (ret < 0)
			return ret;
		break;
	default:
		size = 0;
		break;
	}

	outbuf->bytesused = size;

	return 0;
}

static int
uvc_gadget_video_process(struct uvc_gadget *gadget)
{
	struct v4l2_buffer outbuf;
	int ret;

	memset(&outbuf, 0, sizeof(struct v4l2_buffer));
	outbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	outbuf.memory = V4L2_MEMORY_MMAP;
	ret = v4l2_dequeue_buf(gadget->out->fd, &outbuf);
	if (ret < 0)
		return ret;

	uvc_gadget_fill_buffer(gadget, &outbuf);

	ret = v4l2_queue_buf(gadget->out->fd, &outbuf);
	if (ret < 0)
		return ret;

	return 0;
}

static int
uvc_gadget_reqbufs(struct uvc_gadget_device *dev, unsigned int nbufs)
{
	struct v4l2_requestbuffers rbuf;
	struct v4l2_buffer buf;
	unsigned int i;
	int ret;

	for (i = 0; i < dev->nbufs; ++i)
		munmap(dev->buf[i], dev->bufsize);

	free(dev->buf);
	dev->buf = NULL;
	dev->nbufs = 0;

	memset(&rbuf, 0, sizeof(struct v4l2_requestbuffers));
	rbuf.count = nbufs;
	rbuf.type = dev->type;
	rbuf.memory = V4L2_MEMORY_MMAP;
	ret = v4l2_request_buffer(dev->fd, &rbuf);
	if (ret < 0)
		return ret;

	xprint(xLOGDBG, "%u buffers allocated\n", rbuf.count);

	/* Map the buffers. */
	dev->buf = malloc(rbuf.count * sizeof(void *));

	for (i = 0; i < rbuf.count; ++i) {
		memset(&buf, 0, sizeof(struct v4l2_buffer));
		buf.index = i;
		buf.type = dev->type;
		buf.memory = V4L2_MEMORY_MMAP;

		ret = v4l2_query_buffer(dev->fd, &buf);
		if (ret < 0)
			return -1;
		xprint(xLOGDBG, "length: %u offset: %u\n",
		       buf.length, buf.m.offset);

		if (dev->fcc == V4L2_PIX_FMT_YUYV) {
			if (buf.length != (dev->width * dev->height * 2)) {
				xprint(xLOGERR, "unsupported format\n");
				return -1;
			}
		}

		dev->buf[i] = mmap(0, buf.length,PROT_READ | PROT_WRITE,
				   MAP_SHARED, dev->fd, buf.m.offset);
		if (dev->buf[i] == MAP_FAILED) {
			xprint(xLOGERR, "Unable to maped buffer: %s\n", strerror(errno));
			return -1;
		}
		xprint(xLOGDBG, "Buffer %u mapped at address %p\n",
		       i, dev->buf[i]);

		buf.index = i;
		buf.type = dev->type;
		buf.memory = V4L2_MEMORY_MMAP;

		ret = v4l2_queue_buf(dev->fd, &buf);
		if (ret < 0)
			return ret;
	}

	dev->bufsize = buf.length;
	dev->nbufs = rbuf.count;

	return 0;
}

static int
uvc_gadget_stream(struct uvc_gadget_device *dev, int enable)
{
	int ret;

	if (enable) {
		xprint(xLOGDBG, "Starting video stream\n");
		ret = v4l2_stream_on(dev->fd, dev->type);
	} else {
		xprint(xLOGDBG, "Stopping video stream\n");
		ret = v4l2_stream_off(dev->fd, dev->type);
	}

	return ret;
}

static int
uvc_gadget_set_format(struct uvc_gadget_device *dev, unsigned int imgsize)
{
	struct v4l2_format fmt;

	memset(&fmt, 0, sizeof(struct v4l2_format));
	fmt.type = dev->type;
	fmt.fmt.pix.width = dev->width;
	fmt.fmt.pix.height = dev->height;
	fmt.fmt.pix.pixelformat = dev->fcc;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	if (imgsize)
		fmt.fmt.pix.sizeimage = imgsize;

	return v4l2_set_format(dev->fd, &fmt);
}

/*
 * Request processing
 */

struct uvc_gadget_frame_info
{
	unsigned int width;
	unsigned int height;
	unsigned int intervals[8];
};

struct uvc_gadget_format_info
{
	unsigned int fcc;
	const struct uvc_gadget_frame_info *frames;
};

static const struct uvc_gadget_frame_info uvc_gadget_frames_yuyv[] = {
	{  320,  240, { 166666, 0 }, },
	{  640,  480, { 666666, 0 }, },
	{ 1920, 1080, { 2000000, 0 }, },
	{ 0, 0, { 0, }, },
};

static const struct uvc_gadget_format_info uvc_gadget_formats[] = {
	{ V4L2_PIX_FMT_YUYV,  uvc_gadget_frames_yuyv },
};

static void
uvc_gadget_fill_streaming_control(struct uvc_streaming_control *ctrl,
				  int iframe, int iformat)
{
	const struct uvc_gadget_format_info *format;
	const struct uvc_gadget_frame_info *frame;
	unsigned int nframes;

	if (iformat < 0)
		iformat = ARRAY_SIZE(uvc_gadget_formats) + iformat;
	if (iformat < 0 || iformat >= (int)ARRAY_SIZE(uvc_gadget_formats))
		return;
	format = &uvc_gadget_formats[iformat];

	nframes = 0;
	while (format->frames[nframes].width != 0)
		++nframes;

	if (iframe < 0)
		iframe = nframes + iframe;
	if (iframe < 0 || iframe >= (int)nframes)
		return;
	frame = &format->frames[iframe];

	memset(ctrl, 0, sizeof(struct uvc_streaming_control));

	ctrl->bmHint = 1; /* dwFrameInterval */
	ctrl->bFormatIndex = iformat + 1;
	ctrl->bFrameIndex = iframe + 1;
	ctrl->dwFrameInterval = frame->intervals[0];
	if (format->fcc == V4L2_PIX_FMT_YUYV)
		ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
	ctrl->dwMaxPayloadTransferSize = 1024;

	ctrl->bmFramingInfo = 3; /* Support FID/EOF bit of the payload headers */
	ctrl->bPreferedVersion = 1; /* Payload header version 1.1 */
	ctrl->bMaxVersion = 1;
}

static void
uvc_gadget_events_process_standard(struct uvc_gadget *gadget,
				   struct usb_ctrlrequest *ctrl,
				   struct uvc_request_data *resp)
{
	xprint(xLOGDBG, "standard request\n");
	(void)gadget;
	(void)ctrl;
	(void)resp;
}

static void
uvc_gadget_events_process_control(struct uvc_gadget *gadget, uint8_t req,
				  uint8_t cs, struct uvc_request_data *resp)
{
	xprint(xLOGDBG, "control request (req %02x cs %02x)\n", req, cs);
	(void)gadget;
	(void)req;
	(void)cs;
	(void)resp;
}

static void
uvc_gadget_events_process_streaming(struct uvc_gadget *gadget, uint8_t req,
				    uint8_t cs, struct uvc_request_data *resp)
{
	struct uvc_streaming_control *ctrl;

	xprint(xLOGDBG, "streaming request (req %02x cs %02x)\n", req, cs);

	if ((cs != UVC_VS_PROBE_CONTROL) && (cs != UVC_VS_COMMIT_CONTROL))
		return;

	ctrl = (struct uvc_streaming_control *)&resp->data;
	resp->length = sizeof(struct uvc_streaming_control);

	switch (req) {
	case UVC_SET_CUR:
		gadget->control = cs;
		break;
	case UVC_GET_CUR:
		if (cs == UVC_VS_PROBE_CONTROL)
			memcpy(ctrl, &gadget->probe,
			       sizeof(struct uvc_streaming_control));
		else
			memcpy(ctrl, &gadget->commit,
			       sizeof(struct uvc_streaming_control));
		break;
	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_DEF:
		uvc_gadget_fill_streaming_control(ctrl,
					   (req == UVC_GET_MAX) ? -1 : 0,
					   (req == UVC_GET_MAX) ? -1 : 0);
		break;
	case UVC_GET_RES:
		memset(ctrl, 0, sizeof(struct uvc_streaming_control));
		break;
	case UVC_GET_LEN:
		resp->data[0] = 0x00;
		resp->data[1] = sizeof(struct uvc_streaming_control);
		resp->length = 2;
		break;
	case UVC_GET_INFO:
		resp->data[0] = 0x03; /* Supports GET/SET value request */
		resp->length = 1;
		break;
	}
}

static void
uvc_gadget_events_process_class(struct uvc_gadget *gadget,
				struct usb_ctrlrequest *ctrl,
				struct uvc_request_data *resp)
{
	if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
		return;

	switch (ctrl->wIndex & 0xff) {
	case UVC_INTF_CONTROL:
	case (UVC_INTF_CONTROL + 4):
		uvc_gadget_events_process_control(gadget, ctrl->bRequest,
						  ctrl->wValue >> 8, resp);
		break;
	case UVC_INTF_STREAMING:
	case (UVC_INTF_STREAMING + 4):
		uvc_gadget_events_process_streaming(gadget, ctrl->bRequest,
						    ctrl->wValue >> 8, resp);
		break;
	default:
		break;
	}
}

static void
uvc_gadget_events_process_setup(struct uvc_gadget *gadget,
				struct usb_ctrlrequest *ctrl,
				struct uvc_request_data *resp)
{
	gadget->control = 0;

	xprint(xLOGDBG, "bRequestType %02x bRequest %02x "
	       "wValue %04x wIndex %04x wLength %04x\n",
	       ctrl->bRequestType, ctrl->bRequest,
	       ctrl->wValue, ctrl->wIndex, ctrl->wLength);

	switch (ctrl->bRequestType & USB_TYPE_MASK) {
	case USB_TYPE_STANDARD:
		uvc_gadget_events_process_standard(gadget, ctrl, resp);
		break;
	case USB_TYPE_CLASS:
		uvc_gadget_events_process_class(gadget, ctrl, resp);
		break;
	default:
		break;
	}
}

static void
uvc_gadget_events_process_data(struct uvc_gadget *gadget,
			       struct uvc_request_data *data)
{
	struct uvc_streaming_control *target;
	struct uvc_streaming_control *ctrl;
	const struct uvc_gadget_format_info *format;
	const struct uvc_gadget_frame_info *frame;
	const unsigned int *interval;
	unsigned int iformat, iframe;
	unsigned int nframes;

	switch (gadget->control) {
	case UVC_VS_PROBE_CONTROL:
		xprint(xLOGDBG, "setting probe control, length = %d\n",
		       data->length);
		target = &gadget->probe;
		break;
	case UVC_VS_COMMIT_CONTROL:
		xprint(xLOGDBG, "setting commit control, length = %d\n",
		       data->length);
		target = &gadget->commit;
		break;
	default:
		xprint(xLOGERR, "setting unknown control, length = %d\n",
			data->length);
		return;
	}

	ctrl = (struct uvc_streaming_control *)&data->data;

	iformat = CLAMP((unsigned int)ctrl->bFormatIndex, 1U,
			(unsigned int)ARRAY_SIZE(uvc_gadget_formats));
	format = &uvc_gadget_formats[iformat - 1];

	nframes = 0;
	while (format->frames[nframes].width != 0)
		nframes++;

	iframe = CLAMP((unsigned int)ctrl->bFrameIndex, 1U, nframes);
	frame = &format->frames[iframe - 1];

	interval = frame->intervals;
	while (interval[0] < ctrl->dwFrameInterval && interval[1])
		interval++;

	target->bFormatIndex = iformat;
	target->bFrameIndex = iframe;
	if (format->fcc == V4L2_PIX_FMT_YUYV)
		target->dwMaxVideoFrameSize = frame->width * frame->height * 2;
	target->dwFrameInterval = *interval;

	if (gadget->control == UVC_VS_COMMIT_CONTROL) {
		gadget->out->fcc = format->fcc;
		gadget->out->width = frame->width;
		gadget->out->height = frame->height;
		uvc_gadget_set_format(gadget->out, 0);

		gadget->in->fcc = format->fcc;
		gadget->in->width = frame->width;
		gadget->in->height = frame->height;
		uvc_gadget_set_format(gadget->in, 0);
	}
}

static void
uvc_gadget_events_process(struct uvc_gadget *gadget)
{
	struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (struct uvc_event *)&v4l2_event.u.data;
	struct uvc_request_data resp;
	int ret;

	ret = v4l2_dequeue_event(gadget->out->fd, &v4l2_event);
	if (ret < 0)
		return;

	switch (v4l2_event.type) {
	case UVC_EVENT_CONNECT:
		break;
	case UVC_EVENT_SETUP:
		memset(&resp, 0, sizeof(struct uvc_request_data));
		resp.length = -EL2HLT;
		uvc_gadget_events_process_setup(gadget, &uvc_event->req, &resp);
		uvc_send_response(gadget->out->fd, &resp);
		break;
	case UVC_EVENT_DATA:
		uvc_gadget_events_process_data(gadget, &uvc_event->data);
		break;
	case UVC_EVENT_STREAMON:
		ret = uvc_gadget_reqbufs(gadget->in, NR_VIDEO_BUF);
		if (ret < 0)
			goto unreq_inbuf;
		ret = uvc_gadget_reqbufs(gadget->out, NR_VIDEO_BUF);
		if (ret < 0)
			goto unreq_buf;

		uvc_gadget_stream(gadget->in, VIDEO_STREAM_ON);
		uvc_gadget_stream(gadget->out, VIDEO_STREAM_ON);
		break;
	case UVC_EVENT_DISCONNECT:
	case UVC_EVENT_STREAMOFF:
		uvc_gadget_stream(gadget->out, VIDEO_STREAM_OFF);
		uvc_gadget_reqbufs(gadget->out, 0);

		uvc_gadget_stream(gadget->in, VIDEO_STREAM_OFF);
		uvc_gadget_reqbufs(gadget->in, 0);
		break;
	}

	return;

unreq_buf:
	uvc_gadget_reqbufs(gadget->out, 0);
unreq_inbuf:
	uvc_gadget_reqbufs(gadget->in, 0);
}

static int
uvc_gadget_events_init(struct uvc_gadget *gadget)
{
	struct v4l2_event_subscription sub;
	int ret;

	uvc_gadget_fill_streaming_control(&gadget->probe, 0, 0);
	uvc_gadget_fill_streaming_control(&gadget->commit, 0, 0);

	memset(&sub, 0, sizeof(struct v4l2_event_subscription));
	sub.type = UVC_EVENT_SETUP;
	ret = v4l2_subscribe_event(gadget->out->fd, &sub);
	if (ret < 0)
		return ret;

	sub.type = UVC_EVENT_DATA;
	ret = v4l2_subscribe_event(gadget->out->fd, &sub);
	if (ret < 0)
		return ret;

	sub.type = UVC_EVENT_STREAMON;
	ret = v4l2_subscribe_event(gadget->out->fd, &sub);
	if (ret < 0)
		return ret;

	sub.type = UVC_EVENT_STREAMOFF;
	ret = v4l2_subscribe_event(gadget->out->fd, &sub);
	if (ret < 0)
		return ret;

	sub.type = UVC_EVENT_CONNECT;
	ret = v4l2_subscribe_event(gadget->out->fd, &sub);
	if (ret < 0)
		return ret;

	sub.type = UVC_EVENT_DISCONNECT;
	ret = v4l2_subscribe_event(gadget->out->fd, &sub);
	if (ret < 0)
		return ret;


	return 0;
}

/*
 * main
 */
static int
uvc_gadget_outdev_reset(const char *dev)
{
	int fd;

	fd = open(dev, O_WRONLY);
	if (fd < 0) {
		xprint(xLOGERR, "Error opening output file: %s\n", strerror(errno));
		return -1;
	}

	close(fd);

	return 0;
}

static int
uvc_gadget_device_open(const char *dev)
{
	struct v4l2_capability cap;
	struct stat st;
	int fd;
	int ret;

	ret = stat(dev, &st);
	if (ret < 0) {
		xprint(xLOGERR, "stat: %s\n", strerror(errno));
		return ret;
	}

	if (!S_ISCHR(st.st_mode)) {
		xprint(xLOGERR, "%s is no device\n", dev);
		return -1;
	}

	fd = open(dev, O_RDWR | O_NONBLOCK);
	if (fd < -1) {
		xprint(xLOGERR, "open: %s\n", strerror(errno));
		return -1;
	}
	xprint(xLOGDBG, "'%s' open succeeded, file descriptor = %d\n", dev, fd);

	ret = v4l2_query_capture(fd, &cap);
	if (ret < 0)
		goto uvcdev_do_close;
	xprint(xLOGDBG, "'%s' is %s on bus %s\n", dev, cap.card, cap.bus_info);

	return fd;

uvcdev_do_close:
	close(fd);

	return -1;
}

static struct uvc_gadget *
uvc_gadget_init(const char *outdev, const char *indev)
{
	struct uvc_gadget *gadget;

	gadget = malloc(sizeof(struct uvc_gadget));
	if (gadget == NULL)
		return NULL;
	memset(gadget, 0, sizeof(struct uvc_gadget));

	gadget->out = malloc(sizeof(struct uvc_gadget_device));
	if (gadget->out == NULL)
		goto uvc_do_free;
	memset(gadget->out, 0, sizeof(struct uvc_gadget_device));

	gadget->in = malloc(sizeof(struct uvc_gadget_device));
	if (gadget->in == NULL)
		goto uvc_do_free;
	memset(gadget->in, 0, sizeof(struct uvc_gadget_device));

	gadget->out->fd = uvc_gadget_device_open(outdev);
	if (gadget->out->fd < 0)
		goto uvc_do_free;
	gadget->out->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	gadget->in->fd = uvc_gadget_device_open(indev);
	if (gadget->in->fd < 0)
		goto uvc_do_close;
	gadget->in->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	return gadget;

uvc_do_close:
	close(gadget->out->fd);
uvc_do_free:
	free(gadget);
	free(gadget->out);
	free(gadget->in);

	return NULL;
}

static void
uvc_gadget_exit(struct uvc_gadget *gadget)
{
	close(gadget->in->fd);
	close(gadget->out->fd);
	free(gadget->in);
	free(gadget->out);
	free(gadget);
}

static void
uvc_gadget_usage(const char *name, FILE *fp)
{
	fprintf(fp, "%s version: %s\n", name, VERSION_STR);
	fprintf(fp, "Usage: %s [options]\n", name);
	fprintf(fp, "Available options are\n");
	fprintf(fp, " -o DEVICE   Output Video device [default: %s]\n", DEFAULT_OUTPUT_DEVICE);
	fprintf(fp, " -c DEVICE   Capture Video device [default: %s]\n", DEFAULT_CAPTURE_DEVICE);
	fprintf(fp, " -d          Prints debug information\n");
	fprintf(fp, " -h          Print this help screen and exit\n");
}

static int is_exit = 0;
static void
uvc_gadget_sighandler(int signum __attribute__((unused)))
{
	is_exit = 1;
}

int main(int argc, char *argv[])
{
	struct uvc_gadget *gadget;
	struct sigaction act;
	char *outdev = DEFAULT_OUTPUT_DEVICE;
	char *indev = DEFAULT_CAPTURE_DEVICE;
	fd_set fds;
	fd_set wfds;
	fd_set efds;
	int opt;
	int ret;

	atexit(app_unlock);
	ret = app_lock();
	if (ret) {
		fprintf(stderr, "%s is already running\n", argv[0]);
		return EXIT_FAILURE;
	}

	while ((opt = getopt(argc, argv, "o:c:dh")) != -1) {
		switch (opt) {
		case 'o':
			outdev = optarg;
			break;
		case 'c':
			indev = optarg;
			break;
		case 'd':
			log_level = xLOGDBG;
			break;
		case 'h':
			uvc_gadget_usage(argv[0], stdout);
			return EXIT_SUCCESS;
		default:
			fprintf(stderr, "Invalid option '-%c'\n", opt);
			uvc_gadget_usage(argv[0], stderr);
			return EXIT_FAILURE;
		}
	}

	ret = uvc_gadget_outdev_reset(outdev);
	if (ret < 0)
		return EXIT_FAILURE;

	gadget = uvc_gadget_init(outdev, indev);
	if (gadget == NULL)
		return EXIT_FAILURE;

	ret = uvc_gadget_events_init(gadget);
	if (ret < 0)
		return EXIT_FAILURE;

	daemon(0, 1);

	/* set signal handlers */
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = (void (*)(int))uvc_gadget_sighandler;
	sigemptyset(&act.sa_mask);
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGPIPE, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

	FD_ZERO(&fds);
	FD_SET(gadget->out->fd, &fds);
	while (!is_exit) {
		wfds = fds;
		efds = fds;

		ret = select(gadget->out->fd + 1, NULL, &wfds, &efds, NULL);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			else {
				xprint(xLOGERR, "select: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
		}

		if (FD_ISSET(gadget->out->fd, &efds))
			uvc_gadget_events_process(gadget);
		if (FD_ISSET(gadget->out->fd, &wfds))
			uvc_gadget_video_process(gadget);
	}

	uvc_gadget_exit(gadget);

	return EXIT_SUCCESS;
}
