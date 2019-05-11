#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "../include/launcher.h"
#include "../include/camera.h"
#include "../include/server.h"
#include "../include/imgproc.h"


int main(int argc, char *argv[]) {
	// create structs and variables
	struct accessLauncher launcherDev;
	struct cameraAccess cameraDev;
	struct accessServer serverStr;
	struct accessServer clientStr;
	cameraDev.outputnum = 0;
	bool isclient = 0;
	memset(&cameraDev, 0, sizeof(struct cameraAccess));
	char loadCam;
	int imagecount;
	char* videodevice;
	double threshold;
	char *hostAddress = "127.0.0.1";
	int c;
	unsigned char color[3] = { 0xFF, 0xFF, 0xFF };
	bool foundv = false;
	while ((c = getopt(argc, argv, "v:ci:t:h:s:")) != -1) {
		switch (c) {
			case 'v':
				foundv = true;
				videodevice = optarg;
				break;
			case 'c':
				foundv = true;
				isclient = true;
				break;
			case 'i':
				imagecount = atoi(optarg);
				break;
			case 't':
				threshold = atof(optarg);
				break;
			case 'h':
				hostAddress = optarg;
				break;
			case 's':
				for (int i = 0; i < 3; i++) {
					char currentColor[2];
					memcpy(currentColor, &optarg[i*2], 2);
					color[i] = strtol(currentColor, NULL, 16);
				}
		}
	}
	if (!foundv) {
		fprintf(stderr, "-v is a required argument\n");
		exit(EXIT_FAILURE);
	}

	if (isclient == false) {
		initLauncher(&launcherDev);
		if (launcherDev.foundLauncher == 1) {
			printf("Loading camera and server\n");
			initCamera(videodevice, &cameraDev);
			initServer(5000, &serverStr);
			for (int i = 0; i < imagecount; i++) {
				getImage(&cameraDev);
				unsigned char moveDir = sendData(cameraDev.framebuffer, cameraDev.framesize, &serverStr);
				signalLaunch(moveDir, launcherDev.handle);
			}
			signalLaunch(0x20, launcherDev.handle);
		} else {
			printf("Load camera and server anyways?\n");
			loadCam = getchar();
			if (loadCam == '\n')
				loadCam = getchar();
			while ((loadCam != 'n') && (loadCam != 'N') && (loadCam != 'y') && (loadCam != 'Y')) {
				printf("Invalid input, enter (y/Y/n/N) again\n");
				loadCam = getchar();
			}
			if ((loadCam == 'y') || (loadCam == 'Y')) {
				printf("Loading camera and server\n");
				initCamera(videodevice, &cameraDev);
				initServer(5000, &serverStr);
				for (int i = 0; i < imagecount; i++) {
					getImage(&cameraDev);
					unsigned char moveDir = sendData(cameraDev.framebuffer, cameraDev.framesize, &serverStr);
				}
			}
		}

	} else {
		initClient(&clientStr, 5000, hostAddress);
		for (int i = 0; i < imagecount; i++) {
			recvData(&clientStr);
			int valueReturn = processImage(clientStr.framebuffer, 640, 480, color, threshold);
			returnNet(valueReturn, &clientStr);

		}

	}


	// terminate launcher
exitProgram:
	if (!isclient) {
		termLauncher(&launcherDev);
	}
	if ((loadCam == 'y') || (loadCam == 'Y')) {
		disableDevice(&cameraDev);
	}
	termServer(&serverStr);
	termClient(&clientStr);
	return EXIT_SUCCESS;
}
