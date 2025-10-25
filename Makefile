CC := gcc
CFLAGS := -Wall -Werror -Wextra -pedantic -Iinclude -std=gnu11 -O2
TARGET := hsh
DEBUGFLAGS := -g -O0

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

re: clean all

debug: CFLAGS := $(CFLAGS) $(DEBUGFLAGS)
debug: re

.PHONY: all clean re debug
