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


UVMODULES:ssl$(BITS) utilc$(BITS)

#common/utilc
SRC_DIR = common/utilc/
SOURCES = $(wildcard common/utilc/*.c)
OBJS = $(patsubst %.c,%.o,$(notdir $(SOURCES)))
OBJSD = $(addprefix $(TMP_DIR)utilc/,$(OBJS))

utilc_static:$(OBJS)
	$(AR) $(OUT_DIR)utilc.a $(OBJSD)

utilc_shared:$(OBJS)
	$(CC) -shared -fPIC $(OBJSD) -o $(OUT_DIR)utilc.so

$(OBJS):%.o:$(SRC_DIR)%.c
	$(CC) $(CFLAGS) -c $< -o $(TMP_DIR)utilc/$@

#common/util
SRC_DIR = common/util/
SOURCES = $(wildcard common/util/*.cpp)
OBJS = $(patsubst %.cpp,%.o,$(notdir $(SOURCES)))
OBJSD = $(addprefix $(TMP_DIR)util/,$(OBJS))

util_static:$(OBJS)
	$(AR) $(OUT_DIR)util.a $(OBJSD)

util_shared:$(OBJS)
	$(GG) -shared -fPIC $(OBJSD) -o $(OUT_DIR)util.so

$(OBJS):%.o:$(SRC_DIR)%.cpp
	$(GG) $(GFLAGS) -c $< -o $(TMP_DIR)util/$@

#common/ssl
SRC_DIR = common/ssl/
SOURCES = $(wildcard common/ssl/*.cpp)
OBJS = $(patsubst %.cpp,%.o,$(notdir $(SOURCES)))
OBJSD = $(addprefix $(TMP_DIR)ssl/,$(OBJS))

ssl_static:$(OBJS)
	$(AR) $(OUT_DIR)ssl.a $(OBJSD)

ssl_shared:$(OBJS)
	$(GG) -shared -fPIC $(OBJSD) -o $(OUT_DIR)ssl.so

$(OBJS):%.o:$(SRC_DIR)%.cpp
	$(GG) $(GFLAGS) -c $< -o $(TMP_DIR)ssl/$@

clean:
	rm -rf $(TMP_DIR)utilc/*.o
	rm -rf $(TMP_DIR)util/*.o
	rm -rf $(TMP_DIR)ssl/*.o

