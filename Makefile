


#### User configure  ###############
CONFIG_DEBUG = n
CONFIG_DEBUG_PKB = n
CONFIG_DEBUG_WAIT = n
CONFIG_DEBUG_SOCK = n
CONFIG_DEBUG_ARP_LOCK = n
CONFIG_DEBUG_ICMPEXCFRAGTIME = n
CONFIG_TOPLOGY = 2
#### End of User configure #########



# Use 'make V=1' to see the full commands
ifeq ("$(origin V)", "command line")
	Q =
else
	Q = @
endif
#export Q

MAKEFLAGS += --no-print-directory

IXY     := ../ixy.c2
IXY_INC := $(IXY)/src
IXY_LIB_DIR := $(IXY)/bin
IXY_LIBS := $(IXY_LIB_DIR)/libixy.o



CC = gcc
#LD = ld
CFLAGS =  -Wall -Isrc/include -I $(IXY_INC)
LDFLAGS = -pthread
#export LD CC CFLAGS

ifeq ($(CONFIG_DEBUG), y)
	CFLAGS += -g
endif

ifeq ($(CONFIG_DEBUG), y)
	CFLAGS += -DDEBUG_PKB
endif

ifeq ($(CONFIG_DEBUG_SOCK), y)
	CFLAGS += -DSOCK_DEBUG
endif

ifeq ($(CONFIG_DEBUG_ICMPEXCFRAGTIME), y)
	CFLAGS += -DICMP_EXC_FRAGTIME_TEST
endif

ifeq ($(CONFIG_DEBUG_WAIT), y)
	CFLAGS += -DWAIT_DEBUG
endif

ifeq ($(CONFIG_DEBUG_ARP_LOCK), y)
	CFLAGS += -DDEBUG_ARPCACHE_LOCK
endif

ifeq ($(CONFIG_TOPLOGY), 1)
	CFLAGS += -DCONFIG_TOP1
else
	CFLAGS += -DCONFIG_TOP2
endif


SRC_DIR := src/app src/arp src/ip src/lib src/net src/shell src/socket src/tcp src/udp
SRCS := $(wildcard $(addsuffix /*.c, $(SRC_DIR)))
OBJS := $(patsubst %.c, %.o, $(SRCS))

TAPIP_NAME := bin/tapip
TAPIP_SRCS := $(SRCS)
TAPIP_OBJS := $(patsubst %.c, %.o, $(TAPIP_SRCS))

CBUF_NAME := bin/cbuf
CBUF_SRCS := src/lib/cbuf.c src/lib/lib.c
#CBUF_OBJS := $(patsubst %.c, %.o, $(CBUF_SRCS))

ECHO_CLIENT_NAME := bin/echo-client
ECHO_CLIENT_SRCS := src/test/echo-client.c


.PHONY: all config build run clean tag lines

all: run

config:
	@echo CFLAGS: $(CFLAGS)
	@echo LDFLAGS: $(LDFLAGS)

build: config $(TAPIP_NAME) $(ECHO_CLIENT_NAME)

$(TAPIP_NAME): $(TAPIP_OBJS)
	@echo TAPIP_SRCS: $(TAPIP_SRCS)
	@echo TAPIP_OBJS: $(TAPIP_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(IXY_LIBS)

$(CBUF_NAME): $(CBUF_SRCS)
	@echo CBUF_SRCS:$(CBUF_SRCS)
#	@echo CBUF_OBJS:$(CBUF_OBJS)
	$(CC) -DCBUF_TEST -Isrc/include -o $@ $^ 

$(ECHO_CLIENT_NAME): $(ECHO_CLIENT_SRCS)
	@echo ECHO_CLIENT_SRCS: $(ECHO_CLIENT_SRCS)
	$(CC) -o $@ $^ -pthread

include Makefile.dep
Makefile.dep: $(SRCS)
	$(CC) $(CFLAGS) -MM $^ > $@

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<


run: build
	sudo $(TAPIP_NAME)

clean:
	rm $(OBJS) $(TAPIP_NAME) $(CBUF_NAME) $(ECHO_CLIENT_NAME)


tag:
	ctags -R src/*

lines:
	@echo "code lines:"
	@wc -l `find src/ -name \*.[ch]` | sort -n

