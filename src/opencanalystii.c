#include <libusb/libusb.h>
#include <opencanalystii.h>
#include <stdint.h>
#include <time.h>

#define mod(x) ((x) < 0 ? -(x) : (x))

#define static_assert(expr)                                                    \
    do {                                                                       \
        switch (expr) {                                                        \
        case 0:                                                                \
        case expr:                                                             \
            break;                                                             \
        }                                                                      \
    } while (0)

#define sizeof_arr(arr) (sizeof(arr) / sizeof(arr[0]))

static libusb_device_handle *dev_handle;

extern int ocii_open_device(void) {
    libusb_context *ctx = NULL;
    int config, error_code;

    /**
     * The standard size of a USB packet is 64 bytes
     */
    static_assert(sizeof(ocii_packet_t) == 64UL);

    if (libusb_init(&ctx) < 0) {
        error_code = OCII_ERROR_USB_INIT;
        goto ocii_leave;
    }

    dev_handle = libusb_open_device_with_vid_pid(ctx, OCII_USB_ID_VENDOR,
                                                 OCII_USB_ID_PRODUCT);
    if (dev_handle == NULL) {
        error_code = OCII_ERROR_USB_OPEN;
        goto ocii_exit;
    }

    if (libusb_get_configuration(dev_handle, &config) < 0) {
        error_code = OCII_ERROR_USB_GET_CONF;
        goto ocii_exit;
    }

    if (config != 1)
        if (libusb_set_configuration(dev_handle, config = 1) < 0) {
            error_code = OCII_ERROR_USB_SET_CONF;
            goto ocii_exit;
        }

    if (libusb_kernel_driver_active(dev_handle, 0) == 1)
        if (libusb_detach_kernel_driver(dev_handle, 0) != 0) {
            error_code = OCII_ERROR_USB_DRV_DETACH;
            goto ocii_exit;
        }

    if (libusb_claim_interface(dev_handle, 0) < 0) {
        error_code = OCII_ERROR_USB_CLAIM;
        goto ocii_close;
    }

    return OCII_ERROR_NO_ERROR;
ocii_close:
    libusb_close(dev_handle);
ocii_exit:
    libusb_exit(ctx);
ocii_leave:
    return error_code;
}

extern int ocii_close_device(void) {
    if (dev_handle == NULL)
        return OCII_ERROR_NULL_PTR;

    if (libusb_release_interface(dev_handle, 0) != 0)
        return OCII_ERROR_USB_RELEASE;

    libusb_close(dev_handle);
    libusb_exit(NULL);

    return OCII_ERROR_NO_ERROR;
}

static int ocii_transaction(uint8_t endpoint, ocii_packet_t *request,
                            ocii_packet_t *response) {
    int32_t length;

    if ((request == NULL && response == NULL) || dev_handle == NULL)
        return OCII_ERROR_NULL_PTR;

    if (request != NULL) {
        if (libusb_bulk_transfer(dev_handle, endpoint | OCII_USB_ENDPOINT_OUT,
                                 (unsigned char *)request,
                                 sizeof(ocii_packet_t), &length,
                                 ocii_timeout) != 0)
            return OCII_ERROR_BULK_TRANSFER;
        if (length != sizeof(ocii_packet_t))
            return OCII_ERROR_BULK_TRANSFER;
    }

    if (response != NULL) {
        if (libusb_bulk_transfer(dev_handle, endpoint | OCII_USB_ENDPOINT_IN,
                                 (unsigned char *)response,
                                 sizeof(ocii_packet_t), &length,
                                 ocii_timeout) != 0)
            return OCII_ERROR_BULK_TRANSFER;
        if (length != sizeof(ocii_packet_t))
            return OCII_ERROR_BULK_TRANSFER;
    }

    return OCII_ERROR_NO_ERROR;
}

extern int ocii_flush_tx_buffer(ocii_channel_t channel, int64_t timeout) {
    uint8_t endpoint =
        OCII_CHANNEL_TO_COMMAND_EP[mod(channel) % ocii_channel_sizeof];
    int flush_done = 0;
    int64_t deadline = 0;

    ocii_packet_t req = {.command = OCII_COMMAND_MESSAGE_STATUS};
    ocii_packet_t rsp = {.command = OCII_COMMAND_MESSAGE_STATUS};

    while (deadline == 0 || time(NULL) < deadline) {
        if (deadline == 0 && timeout != 0)
            deadline = time(NULL) + timeout;

        if ((flush_done = ocii_transaction(endpoint, &req, &rsp)) !=
            OCII_ERROR_NO_ERROR)
            break;

        if ((flush_done = rsp.tx_pending) == 0)
            break;
    }

    return flush_done == 0 ? OCII_ERROR_NO_ERROR : OCII_ERROR_FLUSH;
}

extern int ocii_clear_rx_buffer(ocii_channel_t channel) {
    uint8_t endpoint =
        OCII_CHANNEL_TO_COMMAND_EP[mod(channel) % ocii_channel_sizeof];
    ocii_packet_t req = {.command = OCII_COMMAND_CLEAR_RX_BUFFER};

    if (ocii_transaction(endpoint, &req, NULL) != OCII_ERROR_NO_ERROR)
        return OCII_ERROR_CLEAR;

    return OCII_ERROR_NO_ERROR;
}

extern int ocii_init(ocii_channel_t channel, ocii_packet_t *command) {
    uint8_t endpoint =
        OCII_CHANNEL_TO_COMMAND_EP[mod(channel) % ocii_channel_sizeof];

    if (command == NULL)
        return OCII_ERROR_NULL_PTR;

    command->command = OCII_COMMAND_INIT;
    command->padding[&command->mode - &command->padding[0] + 1] = 0x01;

    return ocii_transaction(endpoint, command, NULL);
}

extern int ocii_start(ocii_channel_t channel) {
    uint8_t endpoint =
        OCII_CHANNEL_TO_COMMAND_EP[mod(channel) % ocii_channel_sizeof];
    ocii_packet_t req = {.command = OCII_COMMAND_START};

    return ocii_transaction(endpoint, &req, NULL);
}

extern int ocii_stop(ocii_channel_t channel) {
    uint8_t endpoint =
        OCII_CHANNEL_TO_COMMAND_EP[mod(channel) % ocii_channel_sizeof];
    ocii_packet_t req = {.command = OCII_COMMAND_STOP};

    return ocii_transaction(endpoint, &req, NULL);
}

extern int ocii_write(ocii_channel_t channel, ocii_packet_t *message) {
    uint8_t endpoint;
    ocii_packet_t req = {.command = OCII_COMMAND_MESSAGE_STATUS};
    ocii_packet_t rsp = {.command = OCII_COMMAND_MESSAGE_STATUS};
    int error_code;

    if (message == NULL)
        return OCII_ERROR_NULL_PTR;

    endpoint = OCII_CHANNEL_TO_COMMAND_EP[mod(channel) % ocii_channel_sizeof];
    if ((error_code = ocii_transaction(endpoint, &req, &rsp)) !=
        OCII_ERROR_NO_ERROR)
        return error_code;

    if (rsp.tx_pending > OCII_WRITE_BUFFER)
        return OCII_ERROR_BUFFER_OVERFLOW;

    endpoint = OCII_CHANNEL_TO_MESSAGE_EP[mod(channel) % ocii_channel_sizeof];
    if ((error_code = ocii_transaction(endpoint, message, NULL)) !=
        OCII_ERROR_NO_ERROR)
        return error_code;

    return OCII_ERROR_NO_ERROR;
}

extern int ocii_read(ocii_channel_t channel, ocii_packet_t *message) {
    uint8_t endpoint;
    ocii_packet_t req = {.command = OCII_COMMAND_MESSAGE_STATUS};
    ocii_packet_t rsp = {.command = OCII_COMMAND_MESSAGE_STATUS};
    int error_code;

    if (message == NULL)
        return OCII_ERROR_NULL_PTR;

    endpoint = OCII_CHANNEL_TO_COMMAND_EP[mod(channel) % ocii_channel_sizeof];
    if ((error_code = ocii_transaction(endpoint, &req, &rsp)) !=
        OCII_ERROR_NO_ERROR)
        return error_code;

    if (rsp.rx_pending == 0)
        return OCII_ERROR_BUFFER_EMPTY;

    endpoint = OCII_CHANNEL_TO_MESSAGE_EP[mod(channel) % ocii_channel_sizeof];
    if ((error_code = ocii_transaction(endpoint, NULL, message)) !=
        OCII_ERROR_NO_ERROR)
        return error_code;

    return OCII_ERROR_NO_ERROR;
}

extern int ocii_get_status(ocii_channel_t channel, ocii_packet_t *status) {
    uint8_t endpoint =
        OCII_CHANNEL_TO_COMMAND_EP[mod(channel) % ocii_channel_sizeof];
    ocii_packet_t req = {.command = OCII_COMMAND_CAN_STATUS};

    if (status == NULL)
        return OCII_ERROR_NULL_PTR;

    status->command = OCII_COMMAND_CAN_STATUS;

    return ocii_transaction(endpoint, &req, status);
}

extern const char *ocii_error_code_to_string(int error_code) {
    static const char *error_message[] = {
        [mod(OCII_ERROR_NO_ERROR)] = /* */
        "No error has occurred",
        [mod(OCII_ERROR_USB_INIT)] = /* */
        "Error occurred during USB initialization",
        [mod(OCII_ERROR_USB_OPEN)] = /* */
        "Error occurred while trying to open the USB device",
        [mod(OCII_ERROR_USB_GET_CONF)] = /* */
        "Error occurred while trying to get the USB device configuration",
        [mod(OCII_ERROR_USB_SET_CONF)] = /* */
        "Error occurred while trying to set the USB device configuration",
        [mod(OCII_ERROR_USB_DRV_DETACH)] = /* */
        "Error occurred while trying to detach the USB device driver",
        [mod(OCII_ERROR_USB_CLAIM)] = /* */
        "Error occurred while trying to claim the USB device",
        [mod(OCII_ERROR_USB_RELEASE)] = /* */
        "Error occurred while trying to release the USB device",
        [mod(OCII_ERROR_NULL_PTR)] = /* */
        "Null pointer was encountered, which is not allowed",
        [mod(OCII_ERROR_BULK_TRANSFER)] = /* */
        "Error occurred while performing a bulk USB transfer",
        [mod(OCII_ERROR_FLUSH)] = /* */
        "Error occurred while trying to flush TX data buffer",
        [mod(OCII_ERROR_CLEAR)] = /* */
        "Error occurred while trying to clear RX data buffer",
        [mod(OCII_ERROR_BUFFER_OVERFLOW)] = /* */
        "TX buffer has overflowed",
        [mod(OCII_ERROR_BUFFER_EMPTY)] = /* */
        "RX buffer is empty"};

    if ((error_code = mod(error_code)) < sizeof_arr(error_message))
        return error_message[error_code];
    else
        return NULL;
}
