#配置编译参数
DEBUG = 1
SHARED = 0
BITS64 = 1

CC = gcc
GG = g++
AR = ar rc
CFLAGS = -fPIC -Wall -std=gnu11
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
	GFLAGS += -O -DNDEBUG
else
	CFLAGS += -g
	GFLAGS += -g
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
$(shell mkdir -p $(TMP_DIR)util/)
$(shell mkdir -p $(TMP_DIR)ssl/)
$(shell mkdir -p $(TMP_DIR)cjson/)
$(shell mkdir -p $(TMP_DIR)pugixml/)
$(shell mkdir -p $(TMP_DIR)libuv/)
$(shell mkdir -p $(TMP_DIR)uvipc/)
$(shell mkdir -p $(TMP_DIR)uvlogplus/)
$(shell mkdir -p $(TMP_DIR)uvnetplus/)
$(shell mkdir -p $(TMP_DIR)demo/)

all:thirdparty uvmoudles common demo

thirdparty:libuv cjson pugixml
uvmoudles:uvipc uvlogplus uvnetplus
common:ssl utilc util
demo:uvlogplustest uvnetplustest
	#
################################################

#thirdparty/libuv
LIBUV_SRC_DIR = thirdparty/libuv/src/
LIBUV_SOURCES =   fs-poll.c \
                   inet.c \
                   threadpool.c \
                   uv-data-getter-setters.c \
                   uv-common.c \
                   version.c
LIBUV_OBJS = $(patsubst %.c,%.o,$(LIBUV_SOURCES))
LIBUV_OBJSD = $(addprefix $(TMP_DIR)libuv/,$(LIBUV_OBJS))

LIBUV_SRC_UNIX_DIR = thirdparty/libuv/src/unix/
LIBUV_UNIX_SOURCES	= async.c \
                   core.c \
                   dl.c \
                   fs.c \
                   getaddrinfo.c \
                   getnameinfo.c \
                   loop-watcher.c \
                   loop.c \
                   pipe.c \
                   poll.c \
                   process.c \
                   signal.c \
                   stream.c \
                   tcp.c \
                   thread.c \
                   timer.c \
                   tty.c \
                   udp.c \
				   linux-core.c \
                   linux-inotify.c \
                   linux-syscalls.c \
                   procfs-exepath.c \
                   proctitle.c \
                   sysinfo-loadavg.c \
                   sysinfo-memory.c
LIBUV_UNIX_OBJS = $(patsubst %.c,%.o,$(LIBUV_UNIX_SOURCES))
LIBUV_UNIX_OBJSD = $(addprefix $(TMP_DIR)libuv/,$(LIBUV_UNIX_OBJS))

LIBUV_INCLUDE = -I ./thirdparty/libuv/include -I ./thirdparty/libuv/src -I ./thirdparty/libuv/src/unix

libuv:libuv$(BITS)
	#
libuv_static:$(LIBUV_OBJS) $(LIBUV_UNIX_OBJS)
	$(AR) $(OUT_DIR)libuv.a $(LIBUV_OBJSD) $(LIBUV_UNIX_OBJSD)

libuv_shared:$(LIBUV_OBJS) $(LIBUV_UNIX_OBJS)
	$(CC) -shared -fPIC $(LIBUV_OBJSD) $(LIBUV_UNIX_OBJSD) -o $(OUT_DIR)libuv.so

$(LIBUV_OBJS):%.o:$(LIBUV_SRC_DIR)%.c
	$(CC) $(LIBUV_INCLUDE) -fPIC -Wall -c $< -o $(TMP_DIR)libuv/$@
$(LIBUV_UNIX_OBJS):%.o:$(LIBUV_SRC_UNIX_DIR)%.c
	$(CC) $(LIBUV_INCLUDE) -fPIC -Wall -c $< -o $(TMP_DIR)libuv/$@


#thirdparty/cjson
CJSON_SRC_DIR = thirdparty/cjson/
CJSON_SOURCES = $(wildcard thirdparty/cjson/*.c)
CJSON_OBJS = $(patsubst %.c,%.o,$(notdir $(CJSON_SOURCES)))
CJSON_OBJSD = $(addprefix $(TMP_DIR)cjson/,$(CJSON_OBJS))

cjson:cjson$(BITS)
	#
cjson_static:$(CJSON_OBJS)
	$(AR) $(OUT_DIR)cjson.a $(CJSON_OBJSD)

cjson_shared:$(CJSON_OBJS)
	$(CC) -shared -fPIC $(CJSON_OBJSD) -o $(OUT_DIR)cjson.so

$(CJSON_OBJS):%.o:$(CJSON_SRC_DIR)%.c
	$(CC) $(CFLAGS) -c $< -o $(TMP_DIR)cjson/$@

#thirdparty/pugixml
PUGIXML_SRC_DIR = thirdparty/pugixml/
PUGIXML_SOURCES = $(wildcard thirdparty/pugixml/*.cpp)
PUGIXML_OBJS = $(patsubst %.cpp,%.o,$(notdir $(PUGIXML_SOURCES)))
PUGIXML_OBJSD = $(addprefix $(TMP_DIR)pugixml/,$(PUGIXML_OBJS))

pugixml:pugixml$(BITS)
	#
pugixml_static:$(PUGIXML_OBJS)
	$(AR) $(OUT_DIR)pugixml.a $(PUGIXML_OBJSD)

pugixml_shared:$(PUGIXML_OBJS)
	$(GG) -shared -fPIC $(PUGIXML_OBJSD) -o $(OUT_DIR)pugixml.so

$(PUGIXML_OBJS):%.o:$(PUGIXML_SRC_DIR)%.cpp
	$(GG) $(GFLAGS) -c $< -o $(TMP_DIR)pugixml/$@

#################################################################

#uvmoudles/uvipc
UVIPC_SRC_DIR = uvmoudles/uvipc/
UVIPC_INCLUDE = -I ./thirdparty/libuv/include -I ./common/utilc

uvipc:uvipc$(BITS)
	#
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

uvlogplus:uvlogplus$(BITS)
	#
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
UVNETPLUS_INCLUDE += -I ./uvmoudles/uvlogplus
UVNETPLUS_INCLUDE += -I ./common/utilc
UVNETPLUS_INCLUDE += -I ./common/util

uvnetplus:uvnetplus$(BITS)
	#
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

utilc:utilc$(BITS)
	#
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

util:util$(BITS)
	#
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

ssl:ssl$(BITS)
	#
ssl_static:$(SSL_OBJS)
	$(AR) $(OUT_DIR)ssl.a $(SSL_OBJSD)

ssl_shared:$(SSL_OBJS)
	$(GG) -shared -fPIC $(SSL_OBJSD) -o $(OUT_DIR)ssl.so

$(SSL_OBJS):%.o:$(SSL_SRC_DIR)%.cpp
	$(GG) $(GFLAGS) -c $< -o $(TMP_DIR)ssl/$@

##############################################################
DEMO_INC = -I ./thirdparty/libuv/include
DEMO_INC += -I ./uvmoudles/uvipc
DEMO_INC += -I ./uvmoudles/uvlogplus
DEMO_INC += -I ./uvmoudles/uvnetplus
DEMO_INC += -I ./common/utilc
DEMO_INC += -I ./common/util

#demo/uvipctest
uvipctest:uvipctest$(BITS)
	#
uvipctest_static:uvipctest.o
	$(CC) $(TMP_DIR)demo/uvipctest.o -o $(OUT_DIR)uvipctests -L $(OUT_DIR) -l:uvipc.a -l:libuv.a -l:utilc.a -pthread -ldl

uvipctest_shared:uvipctest.o
	$(CC) $(TMP_DIR)demo/uvipctest.o -o $(OUT_DIR)uvipctest -L $(OUT_DIR) -l:uvipc.so -l:libuv.so -l:utilc.so -pthread -ldl

uvipctest.o:demo/uvIpcTest.c
	$(CC) $(CFLAGS) $(DEMO_INC) -c demo/uvIpcTest.c -o $(TMP_DIR)demo/uvipctest.o

#demo/uvlogplustest
uvlogplustest:uvlogplustest$(BITS)
	#
uvlogplustest_static:uvlogplustest.o
	$(GG) $(TMP_DIR)demo/uvlogplustest.o -o $(OUT_DIR)uvlogplustests -L $(OUT_DIR) -l:uvlogplus.a -l:libuv.a -l:utilc.a -l:util.a -l:cjson.a -l:pugixml.a -pthread -ldl

uvlogplustest_shared:uvlogplustest.o
	$(GG) $(TMP_DIR)demo/uvlogplustest.o -o $(OUT_DIR)uvlogplustests -L $(OUT_DIR) -l:uvlogplus.so -l:libuv.so -l:utilc.so -l:util.so -l:cjson.so -l:pugixml.so -pthread -ldl

uvlogplustest.o:demo/uvLogPlusTest.cpp
	$(GG) $(GFLAGS) $(DEMO_INC) -c demo/uvLogPlusTest.cpp -o $(TMP_DIR)demo/uvlogplustest.o

#demo/uvnetplustest
uvnetplustest:uvnetplustest$(BITS)
	#
uvnetplustest_static:uvnetplustest.o
	$(GG) $(TMP_DIR)demo/uvnetplustest.o -o $(OUT_DIR)uvnetplustests -L $(OUT_DIR) -l:uvnetplus.a -l:uvlogplus.a -l:libuv.a -l:utilc.a -l:util.a -l:cjson.a -l:pugixml.a -pthread -ldl

uvnetplustest_shared:uvnetplustest.o
	$(GG) $(TMP_DIR)demo/uvnetplustest.o -o $(OUT_DIR)uvnetplustest -L $(OUT_DIR) -l:uvnetplus.so -l:uvlogplus.so -l:libuv.so -l:utilc.so -l:util.so -l:cjson.so -l:pugixml.so -pthread -ldl

uvnetplustest.o:demo/uvNetPlusTest.cpp
	$(GG) $(GFLAGS) $(DEMO_INC) -c demo/uvNetPlusTest.cpp -o $(TMP_DIR)demo/uvnetplustest.o

#############################################################

clean:
	rm -rf $(TMP_DIR)utilc/*.o
	rm -rf $(TMP_DIR)util/*.o
	rm -rf $(TMP_DIR)ssl/*.o
	rm -rf $(TMP_DIR)cjson/*.o
	rm -rf $(TMP_DIR)pugixml/*.o
	rm -rf $(TMP_DIR)libuv/*.o
	rm -rf $(TMP_DIR)uvipc/*.o
	rm -rf $(TMP_DIR)uvlogplus/*.o
	rm -rf $(TMP_DIR)uvnetplus/*.o
	rm -rf $(TMP_DIR)demo/*.o

