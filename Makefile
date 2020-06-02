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


UVMODULES:cjson$(BITS) ssl$(BITS) utilc$(BITS)

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
	$(GG) $(GFLAGS) -c $< -o $(TMP_DIR)pugixml/$@

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

