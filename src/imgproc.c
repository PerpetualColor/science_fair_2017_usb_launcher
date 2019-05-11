#include "../include/imgproc.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

struct outputInfo {
    double locX;
    double locY;
    double totalsum;
};
struct processInfo {
    unsigned char* bufferYUV;
    unsigned char* bufferRGB;
    double* colorDif;
    int width;
    int height;
    int heightOffset;
    unsigned char colorIn[3];
    double threshold;
    struct outputInfo *output;
};


void clamp(int *number) {
    if (*number > 255) {
        *number = 255;
    }
    if (*number < 0) {
        *number = 0;
    }
}
int compare_function(const void *a, const void *b) {
    double *x = (double *) a;
    double *y = (double *) b;
    if (*x < *y) return -1;
    else if (*x > *y) return 1; return 0;
}
void *threadImgProc(void *info) {
    // copy info to a struct
    struct processInfo threadInfo;
    memcpy(&threadInfo, info, sizeof(struct processInfo));
    int framesize = (threadInfo.width * threadInfo.height << 1);
    unsigned char *bufptr = &threadInfo.bufferRGB[0];
    // *bufferYUV is equivalent to bufferYUV[width * height * 2]

    // step through every 4 bytes of bufferYUV, convert to bufferRGB
    for (int i = 0; i < framesize; i += 4) {
        int y, y2, u, v;
        int r, g, b, r2, g2, b2;
        y = threadInfo.bufferYUV[i];
        y2 = threadInfo.bufferYUV[i + 2];
        u = threadInfo.bufferYUV[i + 1];
        v = threadInfo.bufferYUV[i + 3];
        // first pixel
        u -= 128;
        y -= 16;
        v -= 128;
        y2 -= 16;
        r = 1.164F * y + 1.596F * v;
        g = 1.164F * y - 0.813F * v - 0.391F * v;
        b = 1.164F * y + 2.018F * u;
        clamp(&r);
        clamp(&g);
        clamp(&b);
        *bufptr = r;
        bufptr++;
        *bufptr = g;
        bufptr++;
        *bufptr = b;
        bufptr++;
        r2 = 1.164F * y2 + 1.596F * v;
        g2 = 1.164F * y2 - 0.813F * v - 0.391F * v;
        b2 = 1.164F * y2 + 2.018F * u;
        clamp(&r2);
        clamp(&g2);
        clamp(&b2);
        *bufptr = r2;
        bufptr++;
        *bufptr = g2;
        bufptr++;
        *bufptr = b2;
        bufptr++;
    }
    threadInfo.colorDif = malloc(threadInfo.width * threadInfo.height * sizeof(double));
    double *colorptr = threadInfo.colorDif;
    double locX = 0;
    double locY = 0;
    double maxdif;
    double maxR, maxG, maxB;
    if (threadInfo.colorIn[0] > 128) {
        maxR = threadInfo.colorIn[0];
    } else {
        maxR = 255 - threadInfo.colorIn[0];
    }
    if (threadInfo.colorIn[1] > 128) {
        maxG = threadInfo.colorIn[1];
    } else {
        maxG = 255 - threadInfo.colorIn[1];
    }
    if (threadInfo.colorIn[2] > 128) {
        maxB = threadInfo.colorIn[2];
    } else {
        maxB = 255 - threadInfo.colorIn[2];
    }
    bufptr = &threadInfo.bufferRGB[0];
    maxdif = sqrt(pow(maxR, 2) + pow(maxG, 2) + pow(maxB, 2));
    int count = 0;
    double colorMap[threadInfo.width * threadInfo.height];
    double colorAvg = 0;
    for (int j = 0; j < threadInfo.height; ++j) {
        for (int i = 0; i < threadInfo.width; ++i) {
            int r, g, b;
            r = *bufptr;
            bufptr++;
            g = *bufptr;
            bufptr++;
            b = *bufptr;
            bufptr++;

            *colorptr = sqrt(pow((double)r - (double)threadInfo.colorIn[0], 2) + pow((double)g - (double)threadInfo.colorIn[1], 2) + pow((double)b - (double)threadInfo.colorIn[2], 2));
            *colorptr = *colorptr * 255 / maxdif;
            // calculate weighted undivided X and Y
            // give lowest numbers most weight (invert color)
            *colorptr = 255 - *colorptr;
            colorMap[i*j] = *colorptr / 255.0f;
            if (colorMap[i*j] < threadInfo.threshold) {
                colorMap[i*j] = 3.0;
                *colorptr = 0x00;
            } else {
                colorAvg += colorMap[i*j];
                count++;
            }
            colorptr++;
        }
    }
    double totalsum = 0; // sum of mappings to 0-1
    int above1 = 0;
    count = 0;
    printf("colorAvg: %f\n", colorAvg);
    for (int j = 0; j < threadInfo.height; ++j) {
        for (int i = 0; i < threadInfo.width; ++i) {
            if (colorMap[i*j] != 3.0) {
                double currentMap = colorMap[i*j];
                currentMap = currentMap + 1.0f - threadInfo.threshold;
                locX = locX + (double) i * currentMap;
                locY = locY + (double) (j + threadInfo.heightOffset) * currentMap;
                totalsum = totalsum + currentMap;
                count++;
            }
        }
    }
    threadInfo.output->locX = locX;
    threadInfo.output->locY = locY;
    threadInfo.output->totalsum = totalsum;
    memcpy(info, &threadInfo, sizeof(struct processInfo));
}
unsigned char processImage(unsigned char *bufferYUV, int width, int height, unsigned char colorIn[3], double threshold) {
    printf("Beginning image process\n");
    double locX = 0;
    double locY = 0;
    double totalsum = 0;
    struct processInfo threadInfo[4];
    struct outputInfo outputInfo[4];
    int yuvIndex = 0;
    int rgbIndex = 0;
    pthread_t threadIDs[4];
    unsigned char bufferRGB[width * height * 3];
    double *colorDif = malloc(width * height * sizeof(double) * 2);
    printf("Size of colorDif: %d\n", sizeof(colorDif));
    double *colorptr = &colorDif[0];
    int colorDifIndex = 0;
    for (int i = 0; i < 4; i++) {
        memset(&threadInfo[i], 0, sizeof(struct processInfo));
        threadInfo[i].bufferYUV = malloc(width * height);
        threadInfo[i].bufferRGB = malloc(0.5 * width * height * 3);
        memcpy(threadInfo[i].bufferYUV, bufferYUV + yuvIndex, width * height * 0.5);
        yuvIndex += (width * height / 2);
        for (int j = 0; j < 3; j++) {
            threadInfo[i].colorIn[j] = colorIn[j];
        }
        threadInfo[i].height = height / 4;
        threadInfo[i].heightOffset = height * i / 4;
        threadInfo[i].width = width;
        threadInfo[i].threshold = threshold;
        threadInfo[i].output = &outputInfo[i];
        printf("Preparation %d successful\n", i);
    }
    printf("Launching threads\n");
    for (int i = 0; i < 4; i++) {
        pthread_create(&threadIDs[i], NULL, threadImgProc, &threadInfo[i]);
    }
    // close threads and free allocated memory
    for (int i = 0; i < 4; i++) {
        printf("Joining thread %d\n", i);
        pthread_join(threadIDs[i], NULL);
        locX = locX + outputInfo[i].locX;
        locY = locY + outputInfo[i].locY;
        totalsum = totalsum + outputInfo[i].totalsum;
        memcpy(bufferRGB + rgbIndex, threadInfo[i].bufferRGB, threadInfo[i].width * threadInfo[i].height * 3);
        memcpy(colorDif + colorDifIndex, threadInfo[i].colorDif, threadInfo[i].width * threadInfo[i].height * sizeof(double));
        colorDifIndex = colorDifIndex + threadInfo[i].width * threadInfo[i].height;
        free(threadInfo[i].colorDif);
        rgbIndex = rgbIndex + threadInfo[i].width * threadInfo[i].height * 3;
    }
    for (int i = 0; i < 4; i++) {
        free(threadInfo[i].bufferYUV);
        free(threadInfo[i].bufferRGB);
    }
    printf("Threads complete\n");
    locX /= totalsum;
    locY /= totalsum;
    printf("locX: %f\nlocY: %f\n", locX, locY);
    unsigned char *bufptr;
    FILE *fp = fopen("recv.ppm", "wb");
    printf("Writing to recv.ppm\n");
    bufptr = &bufferRGB[0];
    (void) fprintf(fp, "P6\n%d %d\n255\n", width, height);
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            static unsigned char color[3];
            color[0] = *bufptr;
            bufptr++;
            color[1] = *bufptr;
            bufptr++;
            color[2] = *bufptr;
            bufptr++;
            (void) fwrite(color, 1, 3, fp);
        }
    }
    (void) fclose(fp);

    fp = fopen("recv2.ppm", "wb");
    printf("Writing to recv2.ppm\n");
    (void) fprintf(fp, "P6\n%d %d\n255\n", width, height);
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            static unsigned char color[3];
            color[0] = (int)*colorptr;
            color[1] = (int)*colorptr;
            color[2] = (int)*colorptr;
            if (abs(i - (int)locX) <= 4) {
                color[0] = 0xFF;
                color[1] = 0;
                color[2] = 0;
            }
            if (abs(j - (int)locY) <= 4) {
                color[0] = 0xFF;
                color[1] = 0;
                color[2] = 0;
            }
            colorptr++;
            (void) fwrite(color, 1, 3, fp);
        }
    }
    (void) fclose(fp);

    // find correct value to move the launcher
    unsigned char returnValue = 0x00;
    int moveThreshold = 30;
    if (abs((width / 2) - locX) > moveThreshold || abs((height / 2) - locX) > moveThreshold) {
        if (abs((width / 2) - locX) > moveThreshold) {
            if (locX > (width / 2)) {
                returnValue += 0x08;
            } else if (locX < (width / 2)) {
                returnValue += 0x04;
            }
        }
        if (abs((height / 2) - locX) > moveThreshold) {
            if (locY < (height / 2)) {
                returnValue += 0x02;
            } else if (locY > (height / 2)) {
                returnValue += 0x01;
            }
        }
    } else if (fpclassify(locX) == FP_NAN || fpclassify(locY) == FP_NAN) {
        returnValue = 0x20;
    } else {
        returnValue = 0x20;
    }
    printf("Processed image\n");

    return returnValue;

}
