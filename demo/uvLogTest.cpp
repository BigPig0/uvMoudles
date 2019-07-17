// uvHttpClient.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <tchar.h>
#include "utilc_api.h"
#include "uvLog.h"

uv_log_handle_t *logger = NULL;
#define MODULE_1_LOGDEBUG(msg, ...) UV_LOG_DEBUG(logger, "moudle1", msg, ##__VA_ARGS__)
#define MODULE_1_LOGINFO(msg, ...)  UV_LOG_INFO (logger, "moudle1", msg, ##__VA_ARGS__)
#define MODULE_1_LOGERROR(msg, ...) UV_LOG_ERROR(logger, "moudle1", msg, ##__VA_ARGS__)

#define MODULE_2_LOGDEBUG(msg, ...) UV_LOG_DEBUG(logger, "moudle2", msg, ##__VA_ARGS__)
#define MODULE_2_LOGINFO(msg, ...)  UV_LOG_INFO (logger, "moudle2", msg, ##__VA_ARGS__)
#define MODULE_2_LOGERROR(msg, ...) UV_LOG_ERROR(logger, "moudle2", msg, ##__VA_ARGS__)

class CLog
{
public:
    CLog();
    ~CLog();

    void Debug(char* msg, ...);
    void Info(char* msg, ...);
    void Error(char* msg, ...);
private:
    void LogPrint(level_t lv, char* msg, va_list va);
private:
    uv_log_handle_t *logger;
    const static unsigned int LOG_SIZE;
    const static char const *LOG_CONFIG;
};

const unsigned int CLog::LOG_SIZE = 2048;
const char const * CLog::LOG_CONFIG = ""
"<configuration status=\"WARN\" monitorInterval=\"30\">"
  "<appenders>"
    "<console name=\"Console\" target=\"SYSTEM_OUT\">"
      "<PatternLayout pattern=\"[%d{HH:mm:ss:SSS}] [%p] - %l - %m%n\"/>"
    "</console>"
    "<RollingFile name=\"RollingFileInfo\" fileName=\"./logs/info.log\" filePattern=\"./logs/$${date:yyyy-MM}/info-%d{yyyy-MM-dd}-%i.log\">"
      "<ThresholdFilter level=\"info\" onMatch=\"ACCEPT\" onMismatch=\"DENY\"/>"
      "<PatternLayout pattern=\"[%d{HH:mm:ss:SSS}] [%p] - %l - %m%n\"/>"
      "<Policies>"
        "<TimeBasedTriggeringPolicy/>"
        "<SizeBasedTriggeringPolicy size=\"100 MB\"/>"
      "</Policies>"
    "</RollingFile>"
  "</appenders>"
  "<loggers>"
    "<root level=\"all\">"
      "<appender-ref ref=\"Console\"/>"
      "<appender-ref ref=\"RollingFileInfo\"/>"
    "</root>"
  "</loggers>"
"</configuration>";

inline CLog::CLog() {
    int ret = uv_log_init_conf_buff(&logger, (char*)LOG_CONFIG);
    if(ret) {
        printf("init uvLog failed %d\r\n", ret);
        return;
    }
}

inline CLog::~CLog() {
    int ret = uv_log_close(logger);
}

inline void CLog::Debug(char* msg, ...){
    va_list args;
    va_start(args, msg);
    LogPrint(level_t::Debug, msg, args);
    va_end(args);
}
inline void CLog::Info(char* msg, ...){
    va_list args;
    va_start(args, msg);
    LogPrint(level_t::Info, msg, args);
    va_end(args);
}
inline void CLog::Error(char* msg, ...){
    va_list args;
    va_start(args, msg);
    LogPrint(level_t::Error, msg, args);
    va_end(args);
}

inline void CLog::LogPrint(level_t lv, char* msg, va_list va) {
    char text[LOG_SIZE];              //日志内容
    memset(text,0,LOG_SIZE);
    vsprintf_s(text, LOG_SIZE, msg, va);
    UV_LOG_WRITE(logger, "CLog", lv, text);
}

int _tmain(int argc, _TCHAR* argv[])
{
    int ret = uv_log_init(&logger);

    CLog log3;

    for(;;){
        MODULE_1_LOGDEBUG("this is a test %d", 1);
        MODULE_2_LOGDEBUG("this is a test %d", 2);
        log3.Debug("this is a test %d", 3);
        MODULE_1_LOGINFO("info 1");
        MODULE_2_LOGINFO("info2");
        log3.Info("info3");
        MODULE_1_LOGERROR("error:%d", 1);
        MODULE_2_LOGERROR("error2");
        log3.Info("error3");
        //break;
        sleep(1000);
    }

    //uv_log_close(logger);
	sleep(INFINITE);
	return 0;
}

