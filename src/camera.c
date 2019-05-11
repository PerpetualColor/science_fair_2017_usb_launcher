#include "../include/camera.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

void refreshCam(struct cameraAccess *cameraDev) {
	int err;
	for (int i = 0; i < NMBUFS; i++) {
		munmap(cameraDev->mem[i], cameraDev->buf.length);
	}

	if ((err = ioctl(cameraDev->devicefd, VIDIOC_QUERYCAP, &cameraDev->cap)) < 0) {
		fprintf(stderr, "Error querying: %d\n", err);
	}
	printf("caps: %x\n", cameraDev->cap.capabilities);
	memset(&cameraDev->fmt, 0, sizeof(struct v4l2_format));
	cameraDev->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	cameraDev->fmt.fmt.pix.width = cameraDev->width;
	cameraDev->fmt.fmt.pix.height = cameraDev->height;
	cameraDev->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
	cameraDev->fmt.fmt.pix.field = V4L2_FIELD_ANY;
	if ((err = ioctl(cameraDev->devicefd, VIDIOC_S_FMT, &cameraDev->fmt)) < 0) {
		fprintf(stderr, "Unable to set format\n");
	}
	memset(&cameraDev->rbufs, 0, sizeof(struct v4l2_requestbuffers));
	cameraDev->rbufs.count = NMBUFS;
	cameraDev->rbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	cameraDev->rbufs.memory = V4L2_MEMORY_MMAP;
	if ((err = ioctl(cameraDev->devicefd, VIDIOC_REQBUFS, &cameraDev->rbufs)) < 0) {
		printf("Error requesting bufs\n");
	}
	// map buffers
	for (int i = 0; i < NMBUFS; i++) {
		memset(&cameraDev->buf, 0, sizeof(struct v4l2_buffer));
		cameraDev->buf.index = i;
		cameraDev->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		cameraDev->buf.memory = V4L2_MEMORY_MMAP;
		if ((err = ioctl(cameraDev->devicefd, VIDIOC_QUERYBUF, &cameraDev->buf)) < 0) {
			fprintf(stderr, "Failed to query: %d\n", strerror(err));
		}
		cameraDev->mem[i] = mmap(0,    // start anywhere
				cameraDev->buf.length, PROT_READ, MAP_SHARED,  // amount of data, protocols
				cameraDev->devicefd,   // device from
				cameraDev->buf.m.offset);  // offset
		printf("Mapped index %d to %p\n", i, cameraDev->mem[i]);
		if (cameraDev->mem[i] == MAP_FAILED) {
			fprintf(stderr, "Map %d failed\n", i);
		}
	}
	for (int i = 0; i < NMBUFS; ++i) {
		memset(&cameraDev->buf, 0, sizeof(struct v4l2_buffer));
		cameraDev->buf.index = i;
		cameraDev->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		cameraDev->buf.memory = V4L2_MEMORY_MMAP;
		if ((err = ioctl(cameraDev->devicefd, VIDIOC_QBUF, &cameraDev->buf)) < 0) {
			printf("Error queueing buffer: %d\n", err);
		}
	}
}
int getImage(struct cameraAccess *cameraDev) {
	refreshCam(cameraDev);
	char outputfile[5] = "0.jpg";
	cameraDev->outputfile = outputfile;
	outputfile[0] = cameraDev->outputnum + '0';
	cameraDev->outputnum++;
	int err;
	FILE *file;
	memset(&cameraDev->buf, 0, sizeof(struct v4l2_buffer));
	printf("grabbing frame\n");

	bool queued = false;
capture:
	if ((err = ioctl(cameraDev->devicefd, VIDIOC_STREAMON, &cameraDev->type)) < 0) {
		printf("unable to start capture: %d\n", err);
	}
	cameraDev->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	cameraDev->buf.memory = V4L2_MEMORY_MMAP;
	if ((err = ioctl(cameraDev->devicefd, VIDIOC_DQBUF, &cameraDev->buf)) < 0) {
		printf("Unable to dequeue frame\n");
	}
	queued = false;
	printf("bytesused: %d\nbuf.index: %d\n", cameraDev->buf.bytesused, cameraDev->buf.index);
	if (cameraDev->buf.index == NMBUFS - 1) {
		for (int i = 0; i < NMBUFS; ++i) {
			memset(&cameraDev->buf, 0, sizeof(struct v4l2_buffer));
			cameraDev->buf.index = i;
			cameraDev->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			cameraDev->buf.memory = V4L2_MEMORY_MMAP;
			if ((err = ioctl(cameraDev->devicefd, VIDIOC_QBUF, &cameraDev->buf)) < 0) {
				printf("Unable to qbuf: %d\n", err);
			} 

			queued = true;
		}
		printf("Resetting buffers\n");
	}

	if (cameraDev->buf.bytesused < cameraDev->framesize) {
		printf("%d < %d\n", cameraDev->buf.bytesused, cameraDev->framesize);
		goto capture;
	} else {
		if (cameraDev->buf.bytesused > cameraDev->framesize) {
			memcpy(cameraDev->framebuffer, cameraDev->mem[cameraDev->buf.index], (size_t) cameraDev->framesize);
		} else {
			memcpy(cameraDev->framebuffer, cameraDev->mem[cameraDev->buf.index], (size_t) cameraDev->buf.bytesused);
		}
	}

	printf("Completed number %d\n", cameraDev->outputnum);
	if ((err = ioctl(cameraDev->devicefd, VIDIOC_STREAMOFF, &cameraDev->type)) < 0) {
		printf("Error disabling stream\n");
	}
	for (int i = 0; i < 16; i++) {
		munmap(cameraDev->mem[i], cameraDev->buf.length);
	}

}
int initCamera(char *videodevice, struct cameraAccess *cameraDev) {
	cameraDev->outputfile = cameraDev->outputnum + ".jpg";
	int format = V4L2_PIX_FMT_YUYV;
	int grabmethod = 1;
	cameraDev->videodevice = videodevice;
	cameraDev->width = 640;
	cameraDev->height = 480;
	int err;
	cameraDev->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	cameraDev->framesize = (cameraDev->width * cameraDev->height << 1);
	printf("Framesize: %d\n", cameraDev->framesize);
	cameraDev->framebuffer = (unsigned char*) calloc(1, (size_t) cameraDev->framesize);

	memset(&cameraDev->cap, 0, sizeof(struct v4l2_capability));

	if ((cameraDev->devicefd = open(cameraDev->videodevice, O_RDWR)) == -1) {
		fprintf(stderr, "Error opening\n");
		exit(EXIT_FAILURE);
	} else {
		printf("Success opening\n");
	}


	return 0;
}
int disableDevice(struct cameraAccess *cameraDev) {
	int err;
	// disable and free memory
	if ((err = ioctl(cameraDev->devicefd, VIDIOC_STREAMOFF, &cameraDev->type)) < 0) {
		printf("Error disabling stream\n");
	}
	for (int i = 0; i < 16; i++) {
		munmap(cameraDev->mem[i], cameraDev->buf.length);
	}

	free(cameraDev->framebuffer);
	cameraDev->framebuffer = NULL;
	if (close(cameraDev->devicefd) < 0) {
		fprintf(stderr, "Closing failure\n");
	}

	return 0;
}
