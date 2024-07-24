CC = gcc
CFLAGS = -O1 -Wall -Wextra -std=c23 -pedantic -static -Ilib/libusb-1.0.27 -Iinclude

LIBUSB_DIR = lib/libusb-1.0.27
LIBUSB_LIB_DIR = $(LIBUSB_DIR)/linux_x64
LIBUSB_LIB = libusb-1.0.a

ifeq ($(shell uname),Linux)
	LDFLAGS = -L$(LIBUSB_LIB_DIR) -lusb-1.0 -ludev
else
	LIBUSB_LIB_DIR = $(LIBUSB_DIR)/windows_x64
	LIBUSB_LIB = libusb-1.0.a
	LDFLAGS = -L$(LIBUSB_LIB_DIR) -lusb-1.0
endif

TARGET = opencanalystii
SRCS = src/opencanalystii.c
OBJS = $(SRCS:.c=.o)
HDRS = include/opencanalystii.h

all: $(TARGET).a

$(TARGET).a: $(OBJS)
	mkdir -p out
	ar rcs out/$(TARGET).a $(OBJS)

.PHONY: clean
clean:
	rm -frv $(OBJS) out

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@