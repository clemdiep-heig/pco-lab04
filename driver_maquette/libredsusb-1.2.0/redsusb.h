/**
 * @file redsusb.h
 * @brief eZ USB FX2 driver for REDS SUB devices
 *
 * @author christain.mueller@heig-vd.ch
 * @bug No known bugs
 */


#ifndef REDSUSB_H
#define REDSUSB_H

#include "stdlib.h"
#include "stdint.h"

#define UCHAR unsigned char

#define BUF_SIZE 1024

#define MAQTRAIN_NB_SENSORS 48

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @brief Set device VID and PID
 *
 * You have to call this function after the init and before
 * you start read or write. Default VID/PID are 0, so the
 * device won't be found if you forget this call.
 *
 * @param vendor_id the USB device vendor id
 * @param product_id the USB device product id
 * @return Always 0
 */
int usb_set_device(int vendor_id, int product_id);

/**
 * @brief Write a value to the USB device
 *
 * @param address the FX2 address to write
 * @param value the value to write
 * @return void
 */
void usb_write_value(int address, int value);

/**
 * @brief Read a value from the USB device
 *
 * @param address the FX2 address to read from
 * @return the value at address
 */
UCHAR usb_read_value(int address);

/**
 * @brief Get all current sensors state from the MaqTrain. (thread-safe)
 *
 * @param sensors an output array containing each sensor value
 *      - must be already allocated
 *      - size of `MAQTRAIN_NB_SENSORS`
 * @return 0 if success, -1 if communication error
 *
 *
 */
int maqtrain_read_sensors(uint8_t *sensors);

/**
 * @brief Send a command to the Marklin console. (thread-safe)
 *
 * @param addr the target element (switch, locomotive, etc.)
 * @param data the order to execute (forward, stop, etc.)
 * @return 0 if success, -1 if communication error
 *
 * Refer to the Marklin interface documentation for the list of valid commands.
 */
int maqtrain_send_command(uint8_t addr, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif /* REDSUSB_H */
