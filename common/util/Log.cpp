#include "Log.h"
#include "uvlogplus.h"
#include <sstream>
#include <stdarg.h>

using namespace std;
using namespace uvLogPlus;

namespace Log {

const int LOG_SIZE = 4 * 1024;
static CLog *log = NULL;

bool open(Print print, Level level, const char *pathFileName) {
    stringstream ss;
    ss << "\"configuration\":{"
        "\"appenders\":{";
    bool apd = false;
    if ((int)print & (int)Print::file) {
        ss << "\"RollingFile\":{\"name\":\"RollingFileAppd\",\"fileName\":\"" << pathFileName << "\", \"Policies\":{\"size\":\"10MB\",\"max\":100}}";
        apd = true;
    }

    if ((int)print & (int)Print::console) {
        if(apd)
            ss << ",";
        ss << "\"console\":{\"name\":\"ConsoleAppd\",\"target\":\"SYSTEM_OUT\"}";
    }

    ss << "},\"loggers\":{"
           "\"root\":{\"level\":\"DEBUG\",\"appender-ref\":{\"ref\":\"ConsoleAppd\"}"
           "},\"commonLog\":{\"level\":\"DEBUG\",";
    apd = false;
    if((int)print & (int)Print::console) {
        ss << "\"appender-ref\":{\"ref\":\"ConsoleAppd\"}";
        apd = true;
    }
    if((int)print & (int)Print::file) {
        if(apd)
            ss << ",";
        ss << "\"appender-ref\":{\"ref\":\"RollingFileAppd\"}";
    }
    ss <<"}"   
        "}";

    log = CLog::Create(ss.str().c_str());
    return log!=NULL;
}

void close()
{
    if(log != NULL) {
        delete log;
        log = NULL;
    }
}

void print(Level level, const char *file, int line, const char *func, const char *fmt, ...)
{
    if(log != NULL) {
        uvLogPlus::Level lv = uvLogPlus::Level::Debug;
        if(level == Level::info)
            lv == uvLogPlus::Level::Info;
        else if(level == Level::warning)
            lv == uvLogPlus::Level::Warn;
        else if(level == Level::error)
            lv == uvLogPlus::Level::Error;

        va_list arg;
        va_start(arg, fmt);
        log->Write("commonLog", lv, file, line, file, fmt, arg);
        va_end(arg);
    }
}

}
