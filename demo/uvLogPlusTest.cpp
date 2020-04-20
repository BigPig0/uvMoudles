// uvHttpClient.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <tchar.h>
#include "utilc_api.h"
#include "uvlogplus.h"
#include <stdarg.h>
#include <windows.h>


//class CLog
//{
//public:
//    CLog();
//    ~CLog();
//
//    void Debug(const char *file, int line, const char *function, char* msg, ...);
//    void Info(const char *file, int line, const char *function, char* msg, ...);
//    void Error(const char *file, int line, const char *function, char* msg, ...);
//private:
//    void LogPrint(uvLogPlus::Level lv, const char *file, int line, const char *function, char* msg, va_list va);
//private:
//    uvLogPlus::CLog *logger;
//    const static unsigned int LOG_SIZE;
//    const static char const *LOG_CONFIG;
//};
//
//const unsigned int CLog::LOG_SIZE = 2048;
/*
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
*/
const char* const LOG_CONFIG = "{"
"\"configuration\":{"
    "\"appenders\":{"
        "\"console\":{\"name\":\"Console\",\"target\":\"SYSTEM_OUT\"},"
        "\"RollingFile\":{\"name\":\"RollingFileInfo\",\"fileName\":\"./logs/info.log\"}"
    "},\"loggers\":{"
        "\"root\":{\"level\":\"DEBUG\",\"appender-ref\":{\"ref\":\"Console\"},\"appender-ref\":{\"ref\":\"RollingFileInfo\"}}"
    "}"
"}"
"}";

//inline CLog::CLog() {
//    logger = uvLogPlus::CLog::CreateBuff(LOG_CONFIG);
//    if(!logger) {
//        printf("init uvLog failed\r\n");
//        return;
//    }
//}
//
//inline CLog::~CLog() {
//    delete logger;
//}
//
//inline void CLog::Debug(const char *file, int line, const char *function, char* msg, ...){
//    va_list args;
//    va_start(args, msg);
//    LogPrint(uvLogPlus::Level::Debug, file, line, function, msg, args);
//    va_end(args);
//}
//inline void CLog::Info(const char *file, int line, const char *function, char* msg, ...){
//    va_list args;
//    va_start(args, msg);
//    LogPrint(uvLogPlus::Level::Info, file, line, function, msg, args);
//    va_end(args);
//}
//inline void CLog::Error(const char *file, int line, const char *function, char* msg, ...){
//    va_list args;
//    va_start(args, msg);
//    LogPrint(uvLogPlus::Level::Error, file, line, function, msg, args);
//    va_end(args);
//}
//
//inline void CLog::LogPrint(uvLogPlus::Level lv, const char *file, int line, const char *function, char* msg, va_list va) {
//    char text[LOG_SIZE];              //日志内容
//    memset(text,0,LOG_SIZE);
//    vsprintf_s(text, LOG_SIZE, msg, va);
//    logger->Write("CLog", lv, file, line, function, text);
//}

using namespace uvLogPlus;

#define MODULE_1_LOGDEBUG(msg, ...) log->Write("moudle1", Level::Debug, __FILE__, __LINE__, __FUNCTION__, msg, ##__VA_ARGS__)
#define MODULE_1_LOGINFO(msg, ...)  log->Write("moudle1", Level::Debug, __FILE__, __LINE__, __FUNCTION__, msg, ##__VA_ARGS__)
#define MODULE_1_LOGERROR(msg, ...) log->Write("moudle1", Level::Debug, __FILE__, __LINE__, __FUNCTION__, msg, ##__VA_ARGS__)

#define MODULE_2_LOGDEBUG(msg, ...) log->Write("moudle2", Level::Debug, __FILE__, __LINE__, __FUNCTION__, msg, ##__VA_ARGS__)
#define MODULE_2_LOGINFO(msg, ...)  log->Write("moudle2", Level::Debug, __FILE__, __LINE__, __FUNCTION__, msg, ##__VA_ARGS__)
#define MODULE_2_LOGERROR(msg, ...) log->Write("moudle2", Level::Debug, __FILE__, __LINE__, __FUNCTION__, msg, ##__VA_ARGS__)

int _tmain(int argc, _TCHAR* argv[])
{
    CLog *log = CLog::Create(LOG_CONFIG);
    for(;;){
        MODULE_1_LOGDEBUG("this is a test %d", 1);
        MODULE_2_LOGDEBUG("this is a test %d", 2);
        MODULE_1_LOGINFO("info 1");
        MODULE_2_LOGINFO("info2");
        MODULE_2_LOGERROR("error2");
        sleep(1000);
    }
    delete log;
	sleep(INFINITE);
	return 0;
}

