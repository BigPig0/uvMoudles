#include "st.h"
#include "util.h"
#include "cstl_easy.h"

filter_t* create_filter(){
    SAFE_MALLOC(filter_t, ret);
    ret->on_match = ACCEPT;
    ret->mis_match = DENY;
    return ret;
}

consol_t* create_appender_consol(){
    SAFE_MALLOC(consol_t, ret);
    ret->filter = create_list(void*); //filter*
    list_init(ret->filter);
    return ret;
}

void destory_appender_consol(consol_t* appender) {
}

file_t* create_appender_file(){
    SAFE_MALLOC(file_t, ret);
    ret->filter = create_list(void*); //filter*
    list_init(ret->filter);
    return ret;
}

void destory_appender_file(file_t* appender) {
    if(appender->file_name)
        string_destroy(appender->file_name);
}

rolling_file_t* create_appender_rolling_file(){
    SAFE_MALLOC(rolling_file_t, ret);
    ret->filter = create_list(void*); //filter*
    list_init(ret->filter);
    ret->policies.time_policy.interval = 24;
    ret->policies.time_policy.modulate = false;
    ret->policies.size_policy.size = 0;
    ret->drs.max = 7;
    return ret;
}

void destory_appender_rolling_file(rolling_file_t* appender) {
    if(appender->filePattern)
        string_destroy(appender->filePattern);
}

void destory_appender(appender_t* appender) {
    CHECK_POINT_VOID(appender);
    if (appender->name)
        string_destroy(appender->name);
    if (appender->pattern_layout)
        string_destroy(appender->pattern_layout);
    LIST_DESTORY(appender->filter, filter_t*, free);

    if(appender->type == consol)
        destory_appender_consol((consol_t*)appender);
    else if(appender->type == file)
        destory_appender_file((file_t*)appender);
    else if(appender->type == rolling_file)
        destory_appender_rolling_file((rolling_file_t*)appender);

    free(appender);
}

logger_t* create_logger() {
    SAFE_MALLOC(logger_t,ret);
    ret->level = Info;
    ret->name = NULL;   //外面创建
    ret->appender_ref = create_list(void*);   //appender_t*
    list_init(ret->appender_ref);
    ret->additivity = true;
    return ret;
}

void destory_logger(logger_t* logger) {
    CHECK_POINT_VOID(logger);
    if(logger->name)
        string_destroy(logger->name);
    LIST_DESTORY(logger->appender_ref, appender_t*, destory_appender);
    SAFE_FREE(logger);
}

configuration_t* create_config() {
    SAFE_MALLOC(configuration_t, ret);
    ret->status = Error;
    ret->monitorinterval = 5;
    ret->appenders = create_hash_map(void*, void*); //string_t* : appender_t*
    hash_map_init_ex(ret->appenders, 0, string_map_hash, string_map_compare);
    ret->root = create_logger();
    ret->loggers = create_hash_map(void*, void*); //string_t* : logger_t*
    hash_map_init_ex(ret->loggers, 0, string_map_hash, string_map_compare);
    return ret;
}

void destory_config(configuration_t* conf) {
    CHECK_POINT_VOID(conf);
    HASH_MAP_DESTORY(conf->appenders, string_t*, appender_t*, string_destroy, destory_appender);
    destory_logger(conf->root);
    HASH_MAP_DESTORY(conf->loggers, string_t*, logger_t*, string_destroy, destory_logger);
    free(conf);
}