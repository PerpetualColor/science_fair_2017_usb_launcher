#ifndef camera
#define camera

#define NMBUFS 16
#include <linux/videodev2.h>

struct cameraAccess {
    int devicefd;
    int type;
    void *mem[NMBUFS];
    struct v4l2_buffer buf;
    struct v4l2_format fmt;
    struct v4l2_capability cap;
    struct v4l2_requestbuffers rbufs;
    unsigned char* framebuffer;
    int framesize;
    int width;
    int height;
    char *outputfile;
    int outputnum;
    char *videodevice;
};
int disableDevice(struct cameraAccess *cameraDev);
int getImage(struct cameraAccess *cameraDev);
int initCamera(char *videodevice, struct cameraAccess *cameraDev);
#endif