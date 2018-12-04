#include "uv.h"
#include "st.h"
#include "uvLog.h"
#include "util.h"
#include "cptr/cptr.h"
#include "faster/fasterparse.h"

static void _parse_level(faster_attribute_t *attr, level_t *status) {
    if(attr->value_len == 3 && !strncasecmp(attr->value, "All", 3)) {
        *status = All;
    } else if(attr->value_len == 5 && !strncasecmp(attr->value, "Trace", 5)) {
        *status = Trace;
    } else if(attr->value_len == 5 && !strncasecmp(attr->value, "Debug", 5)) {
        *status = Debug;
    } else if(attr->value_len == 5 && !strncasecmp(attr->value, "Info", 5)) {
        *status = Info;
    } else if(attr->value_len == 4 && !strncasecmp(attr->value, "Warn", 4)) {
        *status = Warn;
    } else if(attr->value_len == 4 && !strncasecmp(attr->value, "Error", 4)) {
        *status = Error;
    } else if(attr->value_len == 5 && !strncasecmp(attr->value, "Fatal", 5)) {
        *status = Fatal;
    } else if(attr->value_len == 3 && !strncasecmp(attr->value, "OFF", 3)) {
        *status = OFF;
    } else {
        *status = Debug;
    }
}

static void _parse_match(faster_attribute_t *attr, filter_match_t *match) {
    if(attr->value_len == 3 && !strncasecmp(attr->value, "ACCEPT", 3)) {
        *match = ACCEPT;
    } else if(attr->value_len == 5 && !strncasecmp(attr->value, "NEUTRAL", 5)) {
        *match = NEUTRAL;
    } else if(attr->value_len == 5 && !strncasecmp(attr->value, "DENY", 5)) {
        *match = DENY;
    } else {
        *match = NEUTRAL;
    }
}

static int _parse_pattern_layout(faster_node_t *node, char** value) {
    faster_attribute_t *attr;
    if(node->name_len == 13 && strncasecmp(node->name,"PatternLayout", 13)) {
        for(attr = node->attr; attr; attr = attr->next) {
            if(attr->name_len == 7 && !strncasecmp(attr->name, "pattern", 7)) {
                *value = (char*)malloc(attr->value_len+1);
                strncpy(*value, attr->value, attr->value_len);
            }
        }
        return 0;
    }
    return -1;
}

static int _parse_threshold_filter(faster_node_t *node, filter_list_t **value) {
    faster_attribute_t *attr;
    if(node->name_len == 15 && strncasecmp(node->name,"ThresholdFilter", 15)) {
        *value = (filter_list_t*)malloc(sizeof(filter_list_t));
        for(attr = node->attr; attr; attr = attr->next) {
            if(attr->name_len == 5 && !strncasecmp(attr->name, "level", 5)) {
                _parse_level(attr, &(*value)->level);
            } else if(attr->name_len == 7 && !strncasecmp(attr->name, "onMatch", 7)) {
                _parse_match(attr, &(*value)->on_match);
            } else if(attr->name_len == 10 && !strncasecmp(attr->name, "onMismatch", 10)) {
                _parse_match(attr, &(*value)->mis_match);
            } 
        }
        return 0;
    }
    return -1;
}

static int _uv_log_init_conf_buff(uv_log_handle_t *h, char* conf_buff) {
    faster_node_t *root = NULL, *tmp_node;
    faster_attribute_t *attr = NULL;
    int ret = parse_json(&root, conf_buff);
    if(ret < 0) {
        ret = parse_xml(&root, conf_buff);
    }

    if(root->name_len != 13 || strncasecmp(root->name, "configuration", 13))
        return -1;
    for(attr = root->attr; attr; attr = attr->next) {
        if(attr->name_len == 6 && !strncasecmp(attr->name, "status", 6)) {
            _parse_level(attr, &(h->config.status));
        } else if (attr->name_len == 15 && !strncasecmp(attr->name, "monitorInterval", 15)) {
            char buff[100]={0};
            strncpy(buff, attr->value, attr->value_len);
            h->config.monitorinterval = atoi(buff);
        }
    }

    for(tmp_node = root->first_child; tmp_node; tmp_node=tmp_node->next) {
        if (tmp_node->name_len == 9 && !strncasecmp(tmp_node->name, "appenders", 9)) {
            faster_node_t *appender_node = tmp_node->first_child;
            for(;appender_node; appender_node = appender_node->next) {
                if(appender_node->name_len == 7 && strncasecmp(appender_node->name, "console", 7)) {
                    consol_t* apd = (consol_t*)malloc(sizeof(consol_t));
                    apd->target = SYSTEM_OUT;
                    for(attr = root->attr; attr; attr = attr->next) {
                        if(attr->name_len == 4 && !strncasecmp(attr->name, "name", 4)) {
                            apd->name = (char*)malloc(attr->value_len+1);
                            strncpy(apd->name, attr->value, attr->value_len);
                        } else if (attr->name_len == 4 && !strncasecmp(attr->name, "target", 4)) {
                            if(attr->value_len == 10 && strncasecmp(attr->value, "SYSTEM_ERR", 10)){
                                apd->target = SYSTEM_ERR;
                            }
                        }
                    }
                    apd->pattern_layout = NULL;
                    if(appender_node->first_child) {
                        faster_node_t *partten_node;
                        int ret = 0;
                        for(partten_node= appender_node->first_child; partten_node; partten_node = partten_node->next){
                            ret = _parse_pattern_layout(appender_node->first_child, &apd->pattern_layout);
                            if(!ret) ret = _parse_threshold_filter(appender_node->first_child, &apd->filter);
                        }
                    }
                } else if(appender_node->name_len == 4 && strncasecmp(appender_node->name, "File", 4)) {
                    file_t* apd = (file_t*)malloc(sizeof(file_t));
                    apd->append = 0;
                    for(attr = root->attr; attr; attr = attr->next) {
                        if(attr->name_len == 4 && !strncasecmp(attr->name, "name", 4)) {
                            apd->name = (char*)malloc(attr->value_len+1);
                            strncpy(apd->name, attr->value, attr->value_len);
                        } else if (attr->name_len == 8 && !strncasecmp(attr->name, "fileName", 8)) {
                            apd->file_name = (char*)malloc(attr->value_len+1);
                            strncpy(apd->file_name, attr->value, attr->value_len);
                        } else if (attr->name_len == 8 && !strncasecmp(attr->name, "append", 8)){
                            if(attr->value_len == 10 && strncasecmp(attr->value, "true", 10)){
                                apd->append = 1;
                            }
                        }
                    }
                    apd->pattern_layout = NULL;
                    if(appender_node->first_child) {
                        faster_node_t *partten_node;
                        int ret = 0;
                        for(partten_node= appender_node->first_child; partten_node; partten_node = partten_node->next){
                            ret = _parse_pattern_layout(appender_node->first_child, &apd->pattern_layout);
                        }
                    }
                } else if(appender_node->name_len == 11 && strncasecmp(appender_node->name, "RollingFile", 11)) {
                    rolling_file_t* apd = (rolling_file_t*)malloc(sizeof(rolling_file_t));
                    for(attr = root->attr; attr; attr = attr->next) {
                        if(attr->name_len == 4 && !strncasecmp(attr->name, "name", 4)) {
                            apd->name = (char*)malloc(attr->value_len+1);
                            strncpy(apd->name, attr->value, attr->value_len);
                        } else if (attr->name_len == 8 && !strncasecmp(attr->name, "fileName", 8)) {
                            apd->file_name = (char*)malloc(attr->value_len+1);
                            strncpy(apd->file_name, attr->value, attr->value_len);
                        } else if (attr->name_len == 8 && !strncasecmp(attr->name, "filePattern", 8)){
                            apd->filePattern = (char*)malloc(attr->value_len+1);
                            strncpy(apd->filePattern, attr->value, attr->value_len);
                        }
                    }
                    apd->pattern_layout = NULL;
                    if(appender_node->first_child) {
                        faster_node_t *partten_node;
                        int ret = 0;
                        for(partten_node= appender_node->first_child; partten_node; partten_node = partten_node->next){
                            ret = _parse_pattern_layout(appender_node->first_child, &apd->pattern_layout);
                        }
                    }
                }
            }
        } else if (tmp_node->name_len == 7 && !strncasecmp(tmp_node->name, "loggers", 7)) {
            faster_node_t *logger_node = tmp_node->first_child;
            for(;logger_node; logger_node = logger_node->next) {}
        }
    }
    
    return ret;
}

static int _uv_log_init_conf(uv_log_handle_t *h, uv_file file) {
    char *base = (char*)calloc(1024,1024);
    uv_buf_t buffer = uv_buf_init(base, 1024*1024);
    uv_fs_t read_req;
    int ret = uv_fs_read(NULL, &read_req, file, &buffer, 1, -1, NULL);
    if(ret < 0) {
        printf("uv fs read failed:%s\n",uv_strerror(ret));
        return ret;
    }

    ret = _uv_log_init_conf_buff(h, base);
    return ret;
}

static uv_log_handle_t* _uv_log_init() {
    uv_log_handle_t* logger = (uv_log_handle_t*)malloc(sizeof(uv_log_handle_t));
    logger->config.appenders = create_hash_map(char*, void*);
    logger->config.loggers = create_hash_map(char*, void*);
    if(uv_loop_init(&logger->uv) < 0) {
        printf("uv loop init failed\n");
    }
    return logger;
}

int uv_log_init(uv_log_handle_t **h) {
    uv_fs_t open_req;
    int ret=-1,i=0;
    char *fname[] = {"uvLog_test.json","uvLog_test.jsn","uvLog_test.xml","uvLog.json","uvLog.jsn","uvLog.xml"};
    
    uv_log_handle_t* logger = _uv_log_init();

    while (ret < 0)
    {
        ret = uv_fs_open(NULL, &open_req, fname[i], O_RDONLY, 0, NULL);
        if(ret < 0) {
            printf("uv fs open %s failed:%d\n", fname[i], uv_strerror(ret));
        } else {
            ret = _uv_log_init_conf(logger, open_req.result);
            if(!ret) {
                break;
            }
        }
        i++;
    }
    if(!ret) *h = logger;
    else uv_log_close(logger);
    
    return ret;
}

int uv_log_init_conf(uv_log_handle_t **h, char* conf_path) {
    uv_fs_t open_req;
    int ret;
    uv_log_handle_t* logger = _uv_log_init();

    ret = uv_fs_open(&logger->uv, &open_req, conf_path, O_RDONLY, 0, NULL);
    if(ret < 0) {
        printf("uv fs open %s failed:%d\n", conf_path, uv_strerror(ret));
        return ret;
    }
    ret = _uv_log_init_conf(logger, open_req.result);
    if(!ret) *h = logger;
    else uv_log_close(logger);

    return ret;
}

int uv_log_init_conf_buff(uv_log_handle_t **h, char* conf_buff) {
    uv_log_handle_t* logger = _uv_log_init();

    int ret = _uv_log_init_conf_buff(logger, conf_buff);
    if(ret) *h = logger;
    else uv_log_close(logger);

    return ret;
}

int uv_log_close(uv_log_handle_t *h) {
    hash_map_destroy(h->config.appenders);
    hash_map_destroy(h->config.loggers);
    free(h);
    return 0;
}