ifeq ($(CROSS),MINGW64)
  CC = x86_64-w64-mingw32-gcc
  LD = x86_64-w64-mingw32-gcc
#else
#CC = gcc
#LD = gcc
CC = clang
LD = clang
endif

# for Linux (Ubuntu 12.10 64-bit) or Apple OS/X 10.8
#CFLAGS  = -g -O0
CFLAGS  = -g -O3

# Our Cygwin users are using gcc.
ifeq ($(OS),Windows_NT)
    CC = gcc
    LD = gcc
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),FreeBSD)
      CFLAGS += -I /usr/local/include
      LDFLAGS += -L/usr/local/lib
    endif
endif

#CFLAGS = -m32
#CFLAGS = -m64

CFLAGS += -I../decNumber -I../simhv40-beta -I ../include 

CFLAGS += -std=c99 -U__STRICT_ANSI__  
#CFLAGS += -std=c99 -U__STRICT_ANSI__  -Wconversion

# CFLAGS += -finline-functions -fgcse-after-reload -fpredictive-commoning -fipa-cp-clone -fno-unsafe-loop-optimizations -fno-strict-overflow -Wno-unused-result 

CFLAGS += -D_GNU_SOURCE -DUSE_READER_THREAD
ifeq ($(CROSS),MINGW64)
CFLAGS += -D__USE_MINGW_ANSI_STDIO
#else
CFLAGS += -DHAVE_DLOPEN=so 
endif
CFLAGS += -DUSE_INT64
#CFLAGS += -DMULTIPASS
# Clang generates warning messages for code it generates itself...
CFLAGS += -Wno-array-bounds
LDFLAGS += -g
#CFLAGS += -pg
#LDFLAGS += -pg

MAKEFLAGS += --no-print-directory

%.o : %.c
	@echo CC $<
	@$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

# This file is included as '../Makefile.mk', so it's local include needs the ../
ifneq (,$(wildcard ../Makefile.local))
$(warning ####)
$(warning #### Using ../Makefile.local)
$(warning ####)
include ../Makefile.local
endif

