/*------------------------------------------------------------------------------
 * USB driver for reds developpment tools (ConsoleUSB, ServoUSB)
 * Nom          : redsusb.c
 *
 * Fonction     : Contient les fonctions qui nous sont nécessaires pour
 *                communiquer avec le microcontroleur via l'API Cypress
 *
 * Auteur       : Christian Muller
 * Date         : 02 mai 2014
 *
 * Version      : 1.2
 *
 * Note			: Inspiré de la version windows de Sébastien Masle
 *
 *--| Modifications |------------------------------------------------------------
 * Version  Auteur 	Date               Description
 * 1.0		CMR		02.05.2014			Original ConsoleUSB driver
 * 1.1		CMR		26.06.204			Extend driver to ServoUSB -> generic
 * 1.2		RMA		18.04.2016			Extended driver to MaqTrain 2.0
 *-----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/types.h>
#include "pthread.h"

#include <libusb.h>
#include "redsusb.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#define verbose_print(...) \
            do { if (VERBOSE) fprintf(stderr, __VA_ARGS__); } while (0)


#ifndef ARRAYSIZE
#define ARRAYSIZE(A) (sizeof(A)/sizeof((A)[0]))
#endif

#define USB_INTERFACE 0 // Interface number

#define USB_ENDPOINT_READ_ADDRESS 0x86
#define USB_ENDPOINT_WRITE_ADDRESS 0x02

#define USB_TIMEOUT 3000 // timeout (in millseconds) that this function should wait before giving up due to no response being received. For an unlimited timeout, use value 0.

#define ConfigBuffSize 257

/* Configuration des FPGAs */
#define InitConfig			0x00
#define Config				0x01

int opcode = 0x000;
libusb_context *ctx = NULL;


int usbdev_vendor_id = 0;
int usbdev_product_id = 0;

pthread_mutex_t mutex_usb = PTHREAD_MUTEX_INITIALIZER;

__attribute__((constructor)) void init(void) {
	verbose_print("INFO: USB communication library loaded!\n");
}

/**
 * @brief Return `1` if bit \p pos of \p bitmap is set.
 */
int testbit(uint8_t bitmap, size_t pos) {
    return bitmap & (1 << pos) ? 1 : 0;
}

/**
 * @brief Print a byte array \p buf of size \p sz in hexa, by 4 columns.
 *
 * Uses \p verbose_print macro
 */
void verbose_print_array(UCHAR *buf, size_t sz) {
    for (size_t i=0; i<sz; i++) {
        if ( (i != 0) && (i%4 == 0)) {
            verbose_print("\n");
        }
        verbose_print("0x%02x ", buf[i]);
    }
    verbose_print("\n");
}

/* Check For Our VID & PID */
int is_usbdevblock(libusb_device *dev) {
	struct libusb_device_descriptor desc;

	libusb_get_device_descriptor(dev, &desc);

	if (desc.idVendor == usbdev_vendor_id && desc.idProduct == usbdev_product_id) {
		return 1;
	}

	return 0;
}

int usb_set_device(int vendor_id, int product_id) {
	verbose_print("INFO: device set as VID=0x%x PID=0x%x\n", vendor_id, product_id);
	usbdev_vendor_id = vendor_id;
	usbdev_product_id = product_id;

	return 0;
}

int usb_open(libusb_device_handle **device) {

	verbose_print("INFO: opening USB device\n");

	/* discover devices */
	libusb_device **list;
	libusb_device *found = NULL;

	libusb_init(&ctx);
	libusb_set_debug(ctx, 3);

	ssize_t cnt = libusb_get_device_list(ctx, &list);
	ssize_t i = 0;
	int err = 0;

	if (cnt < 0) {
		printf("ERROR: no USB devices found, please connect the USB device to the computer\n");
		goto err;
	}

	/* Find our device */
	for (i = 0; i < cnt; i++) {
		libusb_device *dev = list[i];
		if (is_usbdevblock(dev)) {
			found = dev;
			verbose_print("INFO: USB device found\n");
			break;
		}
	}

	if (found) {
		err = libusb_open(found, device);
		if (err) {
			printf("ERROR: unable to open USB device\n");
			goto err;
		}

		if (libusb_kernel_driver_active(*device, 0)) {
			printf("ERROR: device busy... detaching...\n");
			libusb_detach_kernel_driver(*device, 0);
		} else
			verbose_print("INFO: device free from kernel\n");

		err = libusb_claim_interface(*device, 0);
		if (err) {
			printf("ERROR: failed to claim interface : ");
			switch (err) {
			case LIBUSB_ERROR_NOT_FOUND:
				printf("not found\n");
				break;
			case LIBUSB_ERROR_BUSY:
				printf("busy\n");
				break;
			case LIBUSB_ERROR_NO_DEVICE:
				printf("no device\n");
				break;
			default:
				printf("other\n");
				break;
			}
			goto err;
		}

		libusb_free_device_list(list, 1);
		return 0;

	} else {
		printf("ERROR: none of the connected USB devices corresponds to the selected one's ID (0x%x:0x%x)\n", usbdev_vendor_id, usbdev_product_id);
		goto err;
	}

	err:
        libusb_exit(NULL); //FIXME: segfault if called twice
		return -1;
}

int usb_close(libusb_device_handle *device) {
	verbose_print("INFO: closing USB device\n");
	libusb_release_interface(device, USB_INTERFACE);
	libusb_close(device);

	verbose_print("INFO: USB device closed, leaving libusb\n");
	libusb_exit(ctx);

	return 0;
}

int usb_write(unsigned char *buffer, size_t bufferSize) {

	libusb_device_handle *device = NULL;
	int transfered = 0;
	int status = -1;

	pthread_mutex_lock(&mutex_usb);
	
	verbose_print("------------------ WRITE --------------------\n");

	if(usb_open(&device) < 0)
		return -1;

	if ((status = libusb_bulk_transfer(device, USB_ENDPOINT_WRITE_ADDRESS,
			buffer, bufferSize, &transfered, USB_TIMEOUT)) != 0
			|| transfered != bufferSize) {
		verbose_print("ERROR: unable to write to USB device\n");
	}

    verbose_print("INFO: %d/%d bytes sent:\n", transfered, (int ) bufferSize);
    verbose_print_array(buffer, bufferSize);

	usb_close(device);
	
	pthread_mutex_unlock(&mutex_usb);

	return 0;
}

int usb_read(UCHAR *buffer, size_t bufferSize) {

	libusb_device_handle *device = NULL;
	int transfered = 0;
	int status = -1;

	pthread_mutex_lock(&mutex_usb);

	verbose_print("------------------ READ --------------------\n");

	if(usb_open(&device) < 0)
		return -1;

	if ((status = libusb_bulk_transfer(device, USB_ENDPOINT_READ_ADDRESS,
            buffer, bufferSize, &transfered, USB_TIMEOUT)) != 0) {
		verbose_print("ERROR: unable to read from USB device, error %d\n", status);
	}

    verbose_print("INFO: %d bytes received:\n", transfered);
    verbose_print_array(buffer, bufferSize);

	usb_close(device);
	
	pthread_mutex_unlock(&mutex_usb);

    return 0;
}

void usb_write_value(int address, int value) {
	unsigned char buf[4];
	buf[0] = 0xFD;
	buf[1] = value;
	buf[2] = address >> 8;
	buf[3] = address;

    usb_write(buf, 4);
}

UCHAR usb_read_value(int address) {
	unsigned char buf[4];
	buf[0] = 0xFE;
	buf[2] = address >> 8;
	buf[3] = address;

    // send request and get 1-byte response
    usb_write(buf, sizeof(buf));
    usb_read(buf, 1);

    return buf[0];
}

char usb_read_version(void) {
	unsigned char buf[1];
	buf[0] = 0xFF;

    // send request and get 1-byte response
    usb_write(buf, sizeof(buf));
    usb_read(buf, 1);

    return buf[0];
}

char usb_read_fpga_version(void) {
	unsigned char buf[4];
	buf[0] = 0xFE;
	buf[2] = 0x60;
	buf[3] = 0x00;

    // send request and get 1-byte response
    usb_write(buf, sizeof(buf));
    usb_read(buf, 1);

    return buf[0];
}

char usb_read_fpga_nb_pin(void) {
	unsigned char buf[4];
	buf[0] = 0xFE;
	buf[2] = 0x60;
	buf[3] = 0x01;

    // send request and get 1-byte response
    usb_write(buf, sizeof(buf));
    usb_read(buf, 1);

    return buf[0];
}

/*
 * MaqTrain stuff
 */

/**
 * @brief MaqTrain packet structure
 */
struct maqtrain_packet_t {
    uint16_t cmd;
    uint8_t data[6];
};

int maqtrain_read_sensors(uint8_t *sensors) {

    // forge and send sensors read request
    struct maqtrain_packet_t req = { 
        .cmd = 0x018A,
        .data = {0},
    };
    if (usb_write((UCHAR *) &req, sizeof(req)) < 0)
        return -1;

    // read the response
    struct maqtrain_packet_t resp;
    if (usb_read((UCHAR *) &resp, sizeof(resp)) < 0)
        return -1;

    // check that we indeed received the response to our request
    if (req.cmd != resp.cmd)
        return -1;

    // for each byte MSB to LSB
    for (size_t i=0; i<sizeof(resp.data); i++) {
        // for each bit LSB to MSB
        for (size_t j=0; j<8; j++) {
            sensors[i*8 + j] = testbit(resp.data[i], j);
        }
    }

    return 0;
}

int maqtrain_send_command(uint8_t addr, uint8_t data) {

    struct maqtrain_packet_t req = {
        .cmd = addr ? 0x0026 : 0x0016, // no address = general command
        .data = {data, 0x00, addr, 0x00, 0x00, 0x00},
    };

    if (usb_write((UCHAR *) &req, sizeof(req)) < 0)
        return -1;

    return 0;
}
