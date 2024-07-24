# Canalyst-II Userspace Driver for C

Unofficial C userspace driver for the low cost USB analyzer "Canalyst-II" by Chuangxin Technology.

Uses [libusb](https://libusb.info/) library for USB support on Windows and Linux.

This driver is based on black box reverse engineering made by [projectgus](https://github.com/projectgus/python-canalystii) of the USB behaviour of the proprietary software, and reading the basic data structure layouts in the original python-can canalystii source.

## Standalone Usage

```c
#include <opencanalystii.h>
#include <stdint.h>
#include <stdio.h>

uint32_t ocii_timeout = 1000U; /* It is necessary to define extern value */

int main() {
    int ret;

    if ((ret = ocii_open_device()) != OCII_ERROR_NO_ERROR)
        goto ocii_leave;

    ocii_channel_t channel = ocii_channel0;
    ocii_packet_t init = {
        .acc_code = 0x00,
        .acc_mask = 0xFFFFFFFF,
        .filter = 0x01, /* Receive all packages */
        .timing = {[0] = OCIIBR125000[0], [1] = OCIIBR125000[1]},
        .mode = 0x00 /* Normal mode */
    };

    if ((ret = ocii_init(channel, &init)) == OCII_ERROR_NO_ERROR) {
        if ((ret = ocii_start(channel)) == OCII_ERROR_NO_ERROR) {
            ocii_packet_t tx_buffer = {.count = 1};
            tx_buffer.message[0].can_id = 0x610;
            tx_buffer.message[0].data_len = 8;
            tx_buffer.message[0].data[0] = 0x40;
            tx_buffer.message[0].data[1] = 0x01;
            tx_buffer.message[0].data[2] = 0x00;
            tx_buffer.message[0].data[3] = 0x00;
            tx_buffer.message[0].data[4] = 0x00;
            tx_buffer.message[0].data[5] = 0x00;
            tx_buffer.message[0].data[6] = 0x00;
            tx_buffer.message[0].data[7] = 0x00;

            if ((ret = ocii_write(channel, &tx_buffer)) ==
                OCII_ERROR_NO_ERROR) {
                (void)fprintf(stdout, "CAN TX: ");
                for (unsigned long i = 0;
                     i < (sizeof(tx_buffer.message[0].data) /
                          sizeof(tx_buffer.message[0].data[0]));
                     i++)
                    (void)fprintf(stdout, "%02X ",
                                  tx_buffer.message[0].data[i]);
                (void)fprintf(stdout, "\n");

                ocii_packet_t rx_buffer;
                do {
                } while ((ret = ocii_read(channel, &rx_buffer)) !=
                         OCII_ERROR_NO_ERROR);
                (void)fprintf(stdout, "CAN RX: ");
                for (unsigned long i = 0;
                     i < (sizeof(rx_buffer.message[0].data) /
                          sizeof(rx_buffer.message[0].data[0]));
                     i++)
                    (void)fprintf(stdout, "%02X ",
                                  rx_buffer.message[0].data[i]);
                (void)fprintf(stdout, "\n");
            }

            if ((ret = ocii_stop(channel)) != OCII_ERROR_NO_ERROR)
                goto ocii_leave;
        }
    }

    if ((ret = ocii_close_device()) == OCII_ERROR_NO_ERROR)
        return 0;
ocii_leave:
    printf("%s\n", ocii_error_code_to_string(ret));
    return -1;
}
```

## Limitations

Currently, the following things are not supported and may not be possible based on the known USB protocol:

* CAN bus error conditions. There is a function `ocii_get_status()` that seems to provide access to some internal device state, not clear if this can be used to determine when errors occured or invalid messages seen;
* Receive buffer hardware overflow detection;
* ACK status of sent CAN messages;
* Failure status of sent CAN messages. If the device fails to get bus arbitration after some unknown amount of time, it will drop the message silently;
* Configuring whether messages are ACKed by Canalyst-II. This may be possible, see `ocii_init` `acc_code` and `acc_mask`.

## Performance

###TODO: Not yet tested
