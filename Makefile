#配置编译参数
DEBUG = 0
SHARED = 0
BITS64 = 1

CC = gcc
GG = g++
AR = ar rc
CFLAGS = -fPIC -Wall -std=c11
GFLAGS = -fPIC -Wall -std=c++11
LFLAGS = 
OUT_DIR = ./out/
TMP_DIR = ./out/tmp/

ifeq ($(BITS64),0)
	CFLAGS += -m32
	GFLAGS += -m32
	LFLAGS += -m32
	OUT_DIR = out/linux32/
	TMP_DIR = out/tmp/linux32/
else
	CFLAGS += -m64
	GFLAGS += -m64
	LFLAGS += -m64
	OUT_DIR = out/linux64/
	TMP_DIR = out/tmp/linux64/
endif

ifeq ($(DEBUG),0)
	#release
	CFLAGS += -O -DNDEBUG
else
	CFLAGS += -g
endif

ifeq ($(SHARED),0)
	#static
	CFLAGS += -static
	LFLAGS += -static
	BITS = _static
else
	BITS = _shared
endif

#创建输出目录
$(shell mkdir -p $(OUT_DIR))
$(shell mkdir -p $(TMP_DIR)utilc/)
$(shell mkdir -p $(TMP_DIR)ssl/)
$(shell mkdir -p $(TMP_DIR)cjson/)
$(shell mkdir -p $(TMP_DIR)pugixml/)
$(shell mkdir -p $(TMP_DIR)libuv/)
$(shell mkdir -p $(TMP_DIR)uvipc/)
$(shell mkdir -p $(TMP_DIR)uvlogplus/)
$(shell mkdir -p $(TMP_DIR)uvnetplus/)

thirdparty:libuv$(BITS) cjson$(BITS) pugixml$(BITS)
uvmoudles:uvipc$(BITS) uvlogplus$(BITS) uvnetplus$(BITS)
common:ssl$(BITS) utilc$(BITS) util$(BITS)

################################################

#thirdparty/libuv
LIBUV_SRC_DIR = thirdparty/libuv/
LIBUV_SRC_FILE =   src/fs-poll.c \
                   src/inet.c \
                   src/threadpool.c \
                   src/uv-data-getter-setters.c \
                   src/uv-common.c \
                   src/version.c \
				   src/unix/async.c \
                   src/unix/core.c \
                   src/unix/dl.c \
                   src/unix/fs.c \
                   src/unix/getaddrinfo.c \
                   src/unix/getnameinfo.c \
                   src/unix/loop-watcher.c \
                   src/unix/loop.c \
                   src/unix/pipe.c \
                   src/unix/poll.c \
                   src/unix/process.c \
                   src/unix/signal.c \
                   src/unix/stream.c \
                   src/unix/tcp.c \
                   src/unix/thread.c \
                   src/unix/timer.c \
                   src/unix/tty.c \
                   src/unix/udp.c \
				   src/unix/linux-core.c \
                   src/unix/linux-inotify.c \
                   src/unix/linux-syscalls.c \
                   src/unix/procfs-exepath.c \
                   src/unix/proctitle.c \
                   src/unix/sysinfo-loadavg.c \
                   src/unix/sysinfo-memory.c
LIBUV_SOURCES = $(addprefix $(LIBUV_SRC_DIR),$(LIBUV_SRC_FILE))
LIBUV_OBJS = $(patsubst %.c,%.o,$(notdir $(LIBUV_SOURCES)))
LIBUV_OBJSD = $(addprefix $(TMP_DIR)libuv/,$(LIBUV_OBJS))
LIBUV_INCLUDE = -I ./thirdparty/libuv/include -I ./thirdparty/libuv/src -I ./thirdparty/libuv/src/unix

libuv_static:$(LIBUV_OBJS)
	$(AR) $(OUT_DIR)libuv.a $(LIBUV_OBJSD)

libuv_shared:$(LIBUV_OBJS)
	$(CC) -shared -fPIC $(LIBUV_OBJSD) -o $(OUT_DIR)libuv.so

$(LIBUV_OBJS):%.o:$(LIBUV_SOURCES)
	$(CC) $(LIBUV_INCLUDE) -fPIC -Wall -c $< -o $(TMP_DIR)libuv/$@


#thirdparty/cjson
CJSON_SRC_DIR = thirdparty/cjson/
CJSON_SOURCES = $(wildcard thirdparty/cjson/*.c)
CJSON_OBJS = $(patsubst %.c,%.o,$(notdir $(CJSON_SOURCES)))
CJSON_OBJSD = $(addprefix $(TMP_DIR)cjson/,$(CJSON_OBJS))

cjson_static:$(CJSON_OBJS)
	$(AR) $(OUT_DIR)cjson.a $(CJSON_OBJSD)

cjson_shared:$(CJSON_OBJS)
	$(CC) -shared -fPIC $(CJSON_OBJSD) -o $(OUT_DIR)cjson.so

$(CJSON_OBJS):%.o:$(CJSON_SRC_DIR)%.c
	$(CC) $(CFLAGS) -c $< -o $(TMP_DIR)cjson/$@

UVMODULES:cjson$(BITS) ssl$(BITS) utilc$(BITS)

#thirdparty/pugixml
PUGIXML_SRC_DIR = thirdparty/pugixml/
PUGIXML_SOURCES = $(wildcard thirdparty/pugixml/*.cpp)
PUGIXML_OBJS = $(patsubst %.cpp,%.o,$(notdir $(PUGIXML_SOURCES)))
PUGIXML_OBJSD = $(addprefix $(TMP_DIR)pugixml/,$(PUGIXML_OBJS))

pugixml_static:$(PUGIXML_OBJS)
	$(AR) $(OUT_DIR)pugixml.a $(PUGIXML_OBJSD)

pugixml_shared:$(PUGIXML_OBJS)
	$(GG) -shared -fPIC $(PUGIXML_OBJSD) -o $(OUT_DIR)pugixml.so

$(PUGIXML_OBJS):%.o:$(PUGIXML_SRC_DIR)%.cpp
	$(GG) $(GFLAGS) -c $< -o ccpugixml/$@

#################################################################

#uvmoudles/uvipc
UVIPC_SRC_DIR = uvmoudles/uvipc/
UVIPC_INCLUDE = -I ./thirdparty/libuv/include -I ./common/utilc

uvipc_static:uvipc.o
	$(AR) $(OUT_DIR)uvipc.a $(TMP_DIR)uvipc/uvipc.o

uvipc_shared:uvipc.o
	$(CC) -shared -fPIC $(TMP_DIR)uvipc/uvipc.o -o $(OUT_DIR)uvipc.so

uvipc.o:$(UVIPC_SRC_DIR)uvIpc.c $(UVIPC_SRC_DIR)uvIpc.h
	$(CC) $(UVIPC_INCLUDE) -fPIC -Wall -c $(UVIPC_SRC_DIR)uvIpc.c -o $(TMP_DIR)uvipc/uvipc.o

#uvmoudles/uvlogplus
UVLOGPLUS_SRC_DIR = uvmoudles/uvlogplus/
UVLOGPLUS_SOURCES = $(wildcard uvmoudles/uvlogplus/*.cpp)
UVLOGPLUS_OBJS = $(patsubst %.cpp,%.o,$(notdir $(UVLOGPLUS_SOURCES)))
UVLOGPLUS_OBJSD = $(addprefix $(TMP_DIR)uvlogplus/,$(UVLOGPLUS_OBJS))
UVLOGPLUS_INCLUDE = -I ./thirdparty/libuv/include
UVLOGPLUS_INCLUDE += -I ./thirdparty/cjson
UVLOGPLUS_INCLUDE += -I ./thirdparty/pugixml
UVLOGPLUS_INCLUDE += -I ./common/utilc
UVLOGPLUS_INCLUDE += -I ./common/util
UVLOGPLUS_INCLUDE += -I ./common/util/lock_free

uvlogplus_static:$(UVLOGPLUS_OBJS)
	$(AR) $(OUT_DIR)uvlogplus.a $(UVLOGPLUS_OBJSD)

uvlogplus_shared:$(UVLOGPLUS_OBJS)
	$(GG) -shared -fPIC $(UVLOGPLUS_OBJSD) -o $(OUT_DIR)uvlogplus.so

$(UVLOGPLUS_OBJS):%.o:$(UVLOGPLUS_SRC_DIR)%.cpp
	$(GG) $(UVLOGPLUS_INCLUDE) $(GFLAGS) -c $< -o $(TMP_DIR)uvlogplus/$@

#uvmoudles/uvnetplus
UVNETPLUS_SRC_DIR = uvmoudles/uvnetplus/
UVNETPLUS_SOURCES = $(wildcard uvmoudles/uvnetplus/*.cpp)
UVNETPLUS_OBJS = $(patsubst %.cpp,%.o,$(notdir $(UVNETPLUS_SOURCES)))
UVNETPLUS_OBJSD = $(addprefix $(TMP_DIR)uvnetplus/,$(UVNETPLUS_OBJS))
UVNETPLUS_INCLUDE = -I ./thirdparty/libuv/include
UVNETPLUS_INCLUDE += -I ./thirdparty/cjson
UVNETPLUS_INCLUDE += -I ./thirdparty/pugixml
UVNETPLUS_INCLUDE += -I ./common/utilc
UVNETPLUS_INCLUDE += -I ./common/util

uvnetplus_static:$(UVNETPLUS_OBJS)
	$(AR) $(OUT_DIR)uvnetplus.a $(UVNETPLUS_OBJSD)

uvnetplus_shared:$(UVNETPLUS_OBJS)
	$(GG) -shared -fPIC $(UVNETPLUS_OBJSD) -o $(OUT_DIR)uvnetplus.so

$(UVNETPLUS_OBJS):%.o:$(UVNETPLUS_SRC_DIR)%.cpp
	$(GG) $(UVNETPLUS_INCLUDE) $(GFLAGS) -c $< -o $(TMP_DIR)uvnetplus/$@

######################################################################

#common/utilc
UTILC_SRC_DIR = common/utilc/
UTILC_SOURCES = $(wildcard common/utilc/*.c)
UTILC_OBJS = $(patsubst %.c,%.o,$(notdir $(UTILC_SOURCES)))
UTILC_OBJSD = $(addprefix $(TMP_DIR)utilc/,$(UTILC_OBJS))

utilc_static:$(UTILC_OBJS)
	$(AR) $(OUT_DIR)utilc.a $(UTILC_OBJSD)

utilc_shared:$(UTILC_OBJS)
	$(CC) -shared -fPIC $(UTILC_OBJSD) -o $(OUT_DIR)utilc.so

$(UTILC_OBJS):%.o:$(UTILC_SRC_DIR)%.c
	$(CC) $(CFLAGS) -c $< -o $(TMP_DIR)utilc/$@

#common/util
UTIL_SRC_DIR = common/util/
UTIL_SOURCES = $(wildcard common/util/*.cpp)
UTIL_OBJS = $(patsubst %.cpp,%.o,$(notdir $(UTIL_SOURCES)))
UTIL_OBJSD = $(addprefix $(TMP_DIR)util/,$(UTIL_OBJS))

util_static:$(UTIL_OBJS)
	$(AR) $(OUT_DIR)util.a $(UTIL_OBJSD)

util_shared:$(UTIL_OBJS)
	$(GG) -shared -fPIC $(UTIL_OBJSD) -o $(OUT_DIR)util.so

$(UTIL_OBJS):%.o:$(UTIL_SRC_DIR)%.cpp
	$(GG) $(GFLAGS) -c $< -o $(TMP_DIR)util/$@

#common/ssl
SSL_SRC_DIR = common/ssl/
SSL_SOURCES = $(wildcard common/ssl/*.cpp)
SSL_OBJS = $(patsubst %.cpp,%.o,$(notdir $(SSL_SOURCES)))
SSL_OBJSD = $(addprefix $(TMP_DIR)ssl/,$(SSL_OBJS))

ssl_static:$(SSL_OBJS)
	$(AR) $(OUT_DIR)ssl.a $(SSL_OBJSD)

ssl_shared:$(SSL_OBJS)
	$(GG) -shared -fPIC $(SSL_OBJSD) -o $(OUT_DIR)ssl.so

$(SSL_OBJS):%.o:$(SSL_SRC_DIR)%.cpp
	$(GG) $(GFLAGS) -c $< -o $(TMP_DIR)ssl/$@

clean:
	rm -rf $(TMP_DIR)utilc/*.o
	rm -rf $(TMP_DIR)util/*.o
	rm -rf $(TMP_DIR)ssl/*.o
	rm -rf $(TMP_DIR)cjson/*.o
	rm -rf $(TMP_DIR)pugixml/*.o
	rm -rf $(TMP_DIR)libuv/*.o
	rm -rf $(TMP_DIR)uvipc/*.o
	rm -rf $(TMP_DIR)uvlogplus/*.o

