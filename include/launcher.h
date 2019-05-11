#ifndef launcher
#define launcher
#include <libusb-1.0/libusb.h>

struct accessLauncher {
    libusb_device_handle *handle;
    int interface_number;
    libusb_context *context;
    libusb_device *device;
    int foundLauncher;
    int active;
};
int initLauncher(struct accessLauncher *launcherDev);
void launcherStart(struct accessLauncher *launcherDev);
int termLauncher(struct accessLauncher *launcherDev);
void signalLaunch(unsigned char byte, libusb_device_handle *handle);
void sleepns(int ns);

#endif
