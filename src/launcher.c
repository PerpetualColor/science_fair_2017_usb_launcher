#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include <stdlib.h>
#include "../include/launcher.h"
#include <string.h>

void signalLaunch(unsigned char byte, libusb_device_handle *handle) {
    unsigned char data[8] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    data[1] = byte;
    int err;
    err = libusb_control_transfer(handle, 0x21, 0x09, 0x0, 0x0, data, 8, 100);
    if (err < 0) {
        printf("transfer failed: %s\n", libusb_error_name(err));
    } else {
        printf("Signaled launcher: %d\n", byte);
    }
}

unsigned char usrSig(libusb_device_handle *handle) {
    unsigned char value;
    printf("Enter value\n");
    scanf("%hhx", &value);
    signalLaunch(value, handle);
    return value;
}

void sleepns(int ns) {
    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = ns;
    int err;
    if ((err = nanosleep(&tim, &tim2)) < 0) {
        printf("nanosleep failed: %d\n", err);
    }
}

void launcherStart(struct accessLauncher *launcherDev) {
    usrSig(launcherDev->handle);
    sleepns(300000000L);
    signalLaunch(0x20, launcherDev->handle);

}

void error() {
    exit(EXIT_FAILURE);
}

int is_launcher(libusb_device *device) {
    struct libusb_device_descriptor desc;
    int err;
    err = libusb_get_device_descriptor(device, &desc);
    if (desc.idVendor == 0x2123 && desc.idProduct == 0x1010) {
        return 1;
    }

    return 0;
}
int get_interface_number(libusb_device *device) {
    int err;
    struct libusb_config_descriptor *config;
    const struct libusb_interface *inter;
    const struct libusb_interface_descriptor *interdesc;

    err = libusb_get_config_descriptor(device, 0, &config);
    if (config->bNumInterfaces != 1) {
        printf("bNumInterfaces is not 1\n");
        error();
    } else {
        inter = &config->interface[0];
        if (inter->num_altsetting != 1) {
            printf("num_altsetting is not 1\n");
            error();
        } else {
            interdesc = &inter->altsetting[0];
            printf("Interface number: %x\n", interdesc->bInterfaceNumber);
            return interdesc->bInterfaceNumber;

        }
    }
    return 0;

}
int initLauncher(struct accessLauncher *launcherDev) {
    libusb_device **list;
    libusb_device *found = NULL;
    ssize_t cnt;
    ssize_t i = 0;
    int err = 0;
    launcherDev->foundLauncher = 0;
    err = libusb_init(&launcherDev->context);
    if (err < 0) {
        printf("init error\n");
        error();
    }
    cnt = libusb_get_device_list(NULL, &list);
    if (cnt < 0)
        error();
    for (i = 0; i < cnt; i++) {
        libusb_device *device = list[i];
        if (is_launcher(device)) {
            printf("Found launcher\n");
            launcherDev->interface_number = get_interface_number(device);
            launcherDev->foundLauncher = 1;
            launcherDev->device = device;
            break;
        }
    }
    if (launcherDev->foundLauncher) {
        printf("&launcherDev->handle: %p\n", &launcherDev->handle);
        err = libusb_open(launcherDev->device, &launcherDev->handle);
        printf("test\n");
        if (err) {
            printf("Error opening: %s\n", libusb_error_name(err));
            error();
        }
        else {
            err = libusb_detach_kernel_driver(launcherDev->handle, launcherDev->interface_number);
            if (err < 0) {
                printf("Error detaching: %s\n", libusb_error_name(err));
            }
            err = libusb_claim_interface(launcherDev->handle, launcherDev->interface_number);
            if (err < 0) {
                printf("Error claiming: %s\n", libusb_error_name(err));
            }
        }
    } else {
        printf("Launcher not found\n");
    }


    libusb_free_device_list(list, 1);

    return EXIT_SUCCESS;
}
int termLauncher(struct accessLauncher *launcherDev) {
    int err;
    if (launcherDev->foundLauncher) {
        err = libusb_attach_kernel_driver(launcherDev->handle, launcherDev->interface_number);
        if (err < 0) {
            printf("Error attaching: %s\n", libusb_error_name(err));
        }
    }
    libusb_exit(launcherDev->context);
    return EXIT_SUCCESS;

}
