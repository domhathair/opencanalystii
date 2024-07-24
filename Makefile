CC = gcc
CFLAGS = -O1 -Wall -Wextra -std=c23 -pedantic -static -Ilib/libusb-1.0.27 -Iinclude

ifeq ($(shell uname),Linux)
	LDFLAGS = -ludev
endif

TARGET = opencanalystii
SRCS = src/opencanalystii.c
OBJS = $(SRCS:.c=.o)
HDRS = include/opencanalystii.h

all: $(TARGET).a

$(TARGET).a: $(OBJS)
	mkdir -p out
	ar rcs out/lib$(TARGET).a $(OBJS)

.PHONY: clean
clean:
	rm -frv $(OBJS) out

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@
