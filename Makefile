#
# Author: Wirote Chukeaw (chin6700x)
# Copyright: (c) 2026 Wirote Chukeaw. All rights reserved.
# License: MIT License
#

CC ?= clang
CFLAGS = -O3 -Wall -Wextra -pthread -fomit-frame-pointer
ASFLAGS = -O3 -Wall

ARCH ?= $(shell uname -m)

SRCS_CORE = src/main.c \
            src/core/mmap_reader.c \
            src/core/gitignore.c \
            src/core/pool.c \
            src/core/walker.c \
            src/output/printer.c \
            src/asm/dispatcher.c \
            src/asm/simd_portable.c

ifeq ($(ARCH),arm64)
    SRCS_ASM = src/asm/simd_arm64.s src/asm/binary_probe.s src/asm/newline_count.s
    OBJS = $(SRCS_CORE:.c=.o) $(SRCS_ASM:.s=.o)
else ifeq ($(ARCH),aarch64)
    SRCS_ASM = src/asm/simd_arm64.s src/asm/binary_probe.s src/asm/newline_count.s
    OBJS = $(SRCS_CORE:.c=.o) $(SRCS_ASM:.s=.o)
else ifeq ($(ARCH),x86_64)
    CFLAGS += -mavx2 -mfma
    SRCS_X86 = src/asm/simd_x86_avx2.c
    OBJS = $(SRCS_CORE:.c=.o) $(SRCS_X86:.c=.o)
else
    # Universal portable fallback
    OBJS = $(SRCS_CORE:.c=.o)
endif

TARGET = chingrep
ALIAS = cg

all: $(TARGET) $(ALIAS)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(ALIAS): $(TARGET)
	ln -sf $(TARGET) $(ALIAS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(ASFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) src/asm/*.o src/core/*.o src/output/*.o src/*.o $(TARGET) $(ALIAS) asmgrep

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	ln -sf /usr/local/bin/$(TARGET) /usr/local/bin/$(ALIAS)

test: $(TARGET)
	bash test/run-tests.sh

bench: $(TARGET)
	bash test/benchmark.sh

arena: $(TARGET)
	bash arena/run-arena.sh

.PHONY: all clean install test bench arena
