CC := gcc
CFLAGS := -Wall -Werror -Wextra -pedantic -Iinclude -std=gnu11 -O2
TARGET := hsh
DEBUGFLAGS := -g -O0 -DDEBUG

SRCDIR := src
BUILDDIR := build
INCDIR := include

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))

all: $(BUILDDIR) $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(TARGET)

re: clean all

debug: CFLAGS := $(CFLAGS) $(DEBUGFLAGS)
debug: re

.PHONY: all clean re debug
