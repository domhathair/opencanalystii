/**
 * OpenCanalystII - Unofficial userspace C driver for the Canalyst-II USB-CAN
 * analyzer hardware
 *
 * Copyright (C) 2021 Angus Gratton <projectgus@aus.social>
 * Copyright (C) 2024 Alexander Chepkov <domhathair@pm.me>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef opencanalystii_h
#define opencanalystii_h

#include <stdint.h>

/**
 * Open Canalyst II error codes
 */
/* No error has occurred */
#define OCII_ERROR_NO_ERROR 0
/* Error occurred during USB initialization */
#define OCII_ERROR_USB_INIT -1
/* Error occurred while trying to open the USB device */
#define OCII_ERROR_USB_OPEN -2
/* Error occurred while trying to get the USB device configuration */
#define OCII_ERROR_USB_GET_CONF -3
/* Error occurred while trying to set the USB device configuration */
#define OCII_ERROR_USB_SET_CONF -4
/* Error occurred while trying to detach the USB device driver */
#define OCII_ERROR_USB_DRV_DETACH -5
/* Error occurred while trying to claim the USB device */
#define OCII_ERROR_USB_CLAIM -6
/* Error occurred while trying to release the USB device */
#define OCII_ERROR_USB_RELEASE -7
/* Null pointer was encountered, which is not allowed */
#define OCII_ERROR_NULL_PTR -8
/* Error occurred while performing a bulk USB transfer */
#define OCII_ERROR_BULK_TRANSFER -9
/* Error occurred while trying to flush TX data buffer */
#define OCII_ERROR_FLUSH -10
/* Error occurred while trying to clear RX data buffer */
#define OCII_ERROR_CLEAR -11
/* TX buffer has overflowed */
#define OCII_ERROR_BUFFER_OVERFLOW -12
/* RX buffer is empty */
#define OCII_ERROR_BUFFER_EMPTY -13

#define OCII_USB_ENDPOINT_IN 0x80
#define OCII_USB_ENDPOINT_OUT 0x00

/**
 * This is Microchip's vendor ID, as Canalyst-II uses a PIC32
 */
#define OCII_USB_ID_VENDOR 0x04D8
#define OCII_USB_ID_PRODUCT 0x0053

/**
 * According to https://www.waveshare.com/wiki/USB-CAN-B
 */
#define OCII_WRITE_BUFFER 1000
#define OCII_READ_BUFFER 2000

/**
 * Drop the message if transmission fails first time
 * Echo the message back as RX (note: the TX message is echoed even if sending
 * fails)
 */
#define OCII_SEND_TYPE_NORETRY 1
#define OCII_SEND_TYPE_ECHO 2

#define OCII_COMMAND_INIT 0x1
#define OCII_COMMAND_START 0x2
#define OCII_COMMAND_STOP 0x3
#define OCII_COMMAND_CLEAR_RX_BUFFER 0x5
#define OCII_COMMAND_MESSAGE_STATUS 0x0A
#define OCII_COMMAND_CAN_STATUS 0x0B
#define OCII_COMMAND_PREINIT 0x13

/**
 * Timeout (in milliseconds) that transaction function should wait before giving
 * up due to no response being received. For an unlimited timeout, use value 0
 */
extern uint32_t ocii_timeout;

typedef struct __attribute__((packed)) {
    uint32_t can_id;     /* CAN ID */
    uint32_t time_stamp; /* Time stamp in units of 100 us */
    uint8_t time_flag;   /* Set as 0 or 1U */
    uint8_t send_type;   /* Logical OR of flag bits from SEND_TYPE defines */
    uint8_t remote;      /* Set if message is remote */
    uint8_t extended;    /* Set if CAN ID is an extended address */
    uint8_t data_len;    /* Data lenght */
    uint8_t data[8];     /* Data */
} ocii_message_t;

typedef struct __attribute__((packed)) {
    union {
        /**
         * Message buffer
         */
        struct {
            uint8_t count;
            ocii_message_t message[3];
        };
        /**
         * Command operation
         */
        struct {
            uint32_t command;
            union {
                /**
                 * Determine the package size by padding
                 */
                struct {
                    uint32_t padding[15];
                };
                /**
                 * Init command
                 */
                struct {
                    uint32_t acc_code; /* ACK behvaiour, set to 0 or 1U */
                    uint32_t acc_mask; /* Similar, set to 0xFFFFFFFF */
                    uint32_t : 32;     /* Reserved */
                    uint32_t filter; /* CANPro sets to 1U for "SingleFilter" */
                    uint32_t : 32;   /* Reserved */
                    uint32_t timing[2]; /* BTR */
                    uint32_t mode;      /* Set to 0 for normal mode */
                };
                /**
                 * Message status response
                 */
                struct {
                    uint32_t rx_pending; /* RX message pending */
                    uint32_t tx_pending; /* TX message pending */
                };
                /**
                 * CAN status response.
                 * This is guesswork, mapping the names in the DLL structure to
                 * the fields in the packet - as this pattern also applies to
                 * the Messages, but maybe it's not this simple.
                 */
                struct {
                    uint32_t err_interrupt;
                    uint32_t reg_mode;
                    uint32_t reg_status;
                    uint32_t reg_al_capture;
                    uint32_t reg_ec_capture;
                    uint32_t reg_ew_limit;
                    uint32_t reg_re_counter;
                    uint32_t reg_te_counter;
                };
            };
        };
    };
} ocii_packet_t;

/**
 * OCIIBR stands for Open Canalyst II Baud Rate
 */
#define OCIIBR5000 ((uint32_t[]){0xBF, 0xFF})
#define OCIIBR10000 ((uint32_t[]){0x31, 0x1C})
#define OCIIBR20000 ((uint32_t[]){0x18, 0x1C})
#define OCIIBR33330 ((uint32_t[]){0x09, 0x6F})
#define OCIIBR40000 ((uint32_t[]){0x87, 0xFF})
#define OCIIBR50000 ((uint32_t[]){0x09, 0x1C})
#define OCIIBR66660 ((uint32_t[]){0x04, 0x6F})
#define OCIIBR80000 ((uint32_t[]){0x83, 0xFF})
#define OCIIBR83330 ((uint32_t[]){0x03, 0x6F})
#define OCIIBR100000 ((uint32_t[]){0x04, 0x1C})
#define OCIIBR125000 ((uint32_t[]){0x03, 0x1C})
#define OCIIBR200000 ((uint32_t[]){0x81, 0xFA})
#define OCIIBR250000 ((uint32_t[]){0x01, 0x1C})
#define OCIIBR400000 ((uint32_t[]){0x80, 0xFA})
#define OCIIBR500000 ((uint32_t[]){0x00, 0x1C})
#define OCIIBR666000 ((uint32_t[]){0x80, 0xB6})
#define OCIIBR800000 ((uint32_t[]){0x00, 0x16})
#define OCIIBR1000000 ((uint32_t[]){0x00, 0x14})

typedef enum {
    ocii_channel0,
    ocii_channel1,
    ocii_channel_sizeof
} ocii_channel_t;

#define OCII_CHANNEL_TO_COMMAND_EP ((uint8_t[]){0x02, 0x04})
#define OCII_CHANNEL_TO_MESSAGE_EP ((uint8_t[]){0x01, 0x03})

extern int ocii_open_device(void);
extern int ocii_close_device(void);
extern int ocii_flush_tx_buffer(ocii_channel_t channel, int64_t timeout);
extern int ocii_clear_rx_buffer(ocii_channel_t channel);
extern int ocii_init(ocii_channel_t channel, ocii_packet_t *command);
extern int ocii_start(ocii_channel_t channel);
extern int ocii_stop(ocii_channel_t channel);
extern int ocii_write(ocii_channel_t channel, ocii_packet_t *message);
extern int ocii_read(ocii_channel_t channel, ocii_packet_t *message);
extern int ocii_get_status(ocii_channel_t channel, ocii_packet_t *status);
extern const char *ocii_error_code_to_string(int error_code);

#endif /* opencanalystii_h */