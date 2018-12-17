#include "uv.h"
#include "st.h"
#include "uvLog.h"
#include "utilc.h"
#include "cptr/cptr.h"
#include "faster/fasterparse.h"
#include "cstl_easy.h"

static void _parse_level(fxml_attr_t *attr, level_t *status) {
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

static void _parse_match(fxml_attr_t *attr, filter_match_t *match) {
    if(attr->value_len == 3 && !strncasecmp(attr->value, "ACCEPT", 3)) {
        *match = ACCEPT;
    } else if(attr->value_len == 5 && !strncasecmp(attr->value, "NEUTRAL", 5)) {
        *match = NEUTRAL;
    } else if(attr->value_len == 5 && !strncasecmp(attr->value, "DENY", 5)) {
        *match = DENY;
    }
}

static int _parse_pattern_layout(fxml_node_t *node, string_t** value) {
    fxml_attr_t *attr;
    if(node->name_len == 13 && !strncasecmp(node->name,"PatternLayout", 13)) {
        for(attr = node->attr; attr; attr = attr->next) {
            if(attr->name_len == 7 && !strncasecmp(attr->name, "pattern", 7)) {
                *value = create_string();
                string_init_subcstr(*value, attr->value, attr->value_len);
            }
        }
        return 0;
    }
    return -1;
}

static int _parse_threshold_filter(fxml_node_t *node, list_t *fliter_list) {
    fxml_attr_t *attr;
    if(!parse_strcasecmp(node->name, node->name_len, "ThresholdFilter")) {
        filter_t* value = create_filter();
        for(attr = node->attr; attr; attr = attr->next) {
            if(!parse_strcasecmp(attr->name, attr->name_len, "level")) {
                _parse_level(attr, &value->level);
            } else if(!parse_strcasecmp(attr->name, attr->name_len, "onMatch")) {
                _parse_match(attr, &value->on_match);
            } else if(attr->name_len == 10 && !strncasecmp(attr->name, "onMismatch", 10)) {
                _parse_match(attr, &value->mis_match);
            } 
        }
        list_push_back(fliter_list, value);
        return 0;
    }
    return -1;
}

static int _parse_filters(fxml_node_t *node, list_t *fliter_list) {
    fxml_node_t *filters_child_node;

    if(parse_xmlnode_namecmp(node, "Filters"))
        return -1;

    for(filters_child_node= node->first_child; filters_child_node; filters_child_node = filters_child_node->next){
        _parse_threshold_filter(filters_child_node, fliter_list);
    }
    return 0;
}

static int _parse_policies(fxml_node_t *node, policies_t *policies) {
    fxml_node_t *policies_child_node;

    if(parse_xmlnode_namecmp(node, "Policies"))
        return -1;

    for (policies_child_node = node->first_child; policies_child_node; policies_child_node = policies_child_node->next) {
        if (!parse_xmlnode_namecmp(policies_child_node, "TimeBasedTriggeringPolicy")) {
            fxml_attr_t *attr;
            for(attr = policies_child_node->attr; attr; attr = attr->next) {
                if(!parse_strcasecmp(attr->name, attr->name_len, "interval")) {
                    char value[100]={0};
                    strncpy(value, attr->value, attr->value_len);
                    policies->time_policy.interval = atoi(value);
                } else if(!parse_strcasecmp(attr->name, attr->name_len, "modulate")) {
                    if(!parse_xmlattr_valuecmp(attr, "true"))
                        policies->time_policy.modulate = true;
                }
            }
        } else if (!parse_xmlnode_namecmp(policies_child_node, "SizeBasedTriggeringPolicy")) {
            fxml_attr_t *attr;
            for(attr = policies_child_node->attr; attr; attr = attr->next) {
                if(!parse_strcasecmp(attr->name, attr->name_len, "size")) {
                    char value[100]={0};
                    strncpy(value, attr->value, attr->value_len);
                    policies->size_policy.size = atoi(value);
                    break;
                }
            }
        }
    }
    return 0;
}

static int _parse_default_rollover_strategy(fxml_node_t *node, int *max) {
    fxml_attr_t *attr;

    if(parse_xmlnode_namecmp(node, "DefaultRolloverStrategy"))
        return -1;

    for(attr = node->attr; attr; attr = attr->next) {
        if(!parse_strcasecmp(attr->name, attr->name_len, "max")) {
            char value[100]={0};
            strncpy(value, attr->value, attr->value_len);
            *max = atoi(value);
            break;
        }
    }
    return 0;
}

static int _parse_xml_config(fxml_node_t *root, configuration_t *config) {
    fxml_node_t *tmp_node;
    fxml_attr_t *attr = NULL;
    if(parse_strcasecmp(root->name, root->name_len, "configuration"))
        return -1;

    //根节点的属性
    for(attr = root->attr; attr; attr = attr->next) {
        if(!parse_strcasecmp(attr->name, attr->name_len, "status")) {
            _parse_level(attr, &(config->status));
        } else if(!parse_strcasecmp(attr->name, attr->name_len, "monitorInterval")) {
            char buff[100]={0};
            strncpy(buff, attr->value, attr->value_len);
            config->monitorinterval = atoi(buff);
        }
    }

    for(tmp_node = root->first_child; tmp_node; tmp_node=tmp_node->next) {
        if(!parse_xmlnode_namecmp(tmp_node, "appenders")) {
            fxml_node_t *appender_node = tmp_node->first_child;
            for(;appender_node; appender_node = appender_node->next) {
                // 控制台输出的appender
                if(!parse_xmlnode_namecmp(appender_node, "console")) {
                    consol_t* apd = create_appender_consol();
                    // 属性
                    for(attr = appender_node->attr; attr; attr = attr->next) {
                        if(!parse_xmlattr_namecmp(attr, "name")) {
                            apd->name = create_string();
                            string_init_subcstr(apd->name, attr->value, attr->value_len);
                        } else if (!parse_xmlattr_namecmp(attr, "target")) {
                            if(!parse_xmlattr_valuecmp(attr, "SYSTEM_ERR")) {
                                apd->target = SYSTEM_ERR;
                            }
                        }
                    }
                    // 子节点
                    if(appender_node->first_child) {
                        fxml_node_t *appender_child_node;
                        int ret = 0;
                        for(appender_child_node= appender_node->first_child; appender_child_node; appender_child_node = appender_child_node->next){
                            ret = _parse_pattern_layout(appender_child_node, &apd->pattern_layout);
                            if(!ret) ret = _parse_threshold_filter(appender_child_node, apd->filter);
                            if(!ret) ret = _parse_filters(appender_child_node, apd->filter);
                        }
                    }
                } else if(!parse_xmlnode_namecmp(appender_node, "File")) {
                    file_t* apd = create_appender_file();
                    //属性
                    for(attr = appender_node->attr; attr; attr = attr->next) {
                        if(!parse_xmlattr_namecmp(attr, "name")) {
                            apd->name = create_string();
                            string_init_subcstr(apd->name, attr->value, attr->value_len);
                        } else if (!parse_xmlattr_namecmp(attr, "fileName")) {
                            apd->file_name = create_string();
                            string_init_subcstr(apd->file_name, attr->value, attr->value_len);
                        } else if (!parse_xmlattr_namecmp(attr, "append")){
                            if(!parse_xmlattr_valuecmp(attr, "true")){
                                apd->append = true;
                            }
                        }
                    }
                    //子节点
                    if(appender_node->first_child) {
                        fxml_node_t *appender_child_node;
                        int ret = 0;
                        for(appender_child_node= appender_node->first_child; appender_child_node; appender_child_node = appender_child_node->next){
                            ret = _parse_pattern_layout(appender_child_node, &apd->pattern_layout);
                            if(!ret) ret = _parse_threshold_filter(appender_child_node, apd->filter);
                            if(!ret) ret = _parse_filters(appender_child_node, apd->filter);
                        }
                    }
                } else if(appender_node->name_len == 11 && !strncasecmp(appender_node->name, "RollingFile", 11)) {
                    rolling_file_t* apd = create_appender_rolling_file();
                    //属性
                    for(attr = appender_node->attr; attr; attr = attr->next) {
                        if(!parse_xmlattr_namecmp(attr, "name")) {
                            apd->name = create_string();
                            string_init_subcstr(apd->name, attr->value, attr->value_len);
                        } else if (!parse_xmlattr_namecmp(attr, "filePattern")){
                            apd->filePattern = create_string();
                            string_init_subcstr(apd->filePattern, attr->value, attr->value_len);
                        }
                    }
                    //子节点
                    if(appender_node->first_child) {
                        fxml_node_t *appender_child_node;
                        int ret = 0;
                        for(appender_child_node= appender_node->first_child; appender_child_node; appender_child_node = appender_child_node->next){
                            ret = _parse_pattern_layout(appender_child_node, &apd->pattern_layout);
                            if(!ret) ret = _parse_threshold_filter(appender_child_node, apd->filter);
                            if(!ret) ret = _parse_filters(appender_child_node, apd->filter);
                            if(!ret) ret = _parse_policies(appender_child_node, &apd->policies);
                            if(!ret) ret = _parse_default_rollover_strategy(appender_child_node, &apd->drs.max);
                        }
                    }
                }
            }
        } else if (!parse_xmlnode_namecmp(tmp_node, "loggers")) {
            fxml_node_t *logger_node = tmp_node->first_child;
            for(;logger_node; logger_node = logger_node->next) {
                if (!parse_strcasecmp(logger_node->name, logger_node->name_len, "logger")) {
                    logger_t *log = create_logger();
                    fxml_node_t *ref_node;
                    //属性
                    for(attr = logger_node->attr; attr; attr = attr->next) {
                        if(!parse_strcasecmp(attr->name, attr->name_len, "name")) {
                            log->name = create_string();
                            string_init_subcstr(log->name, attr->value, attr->value_len);
                        } else if(!parse_strcasecmp(attr->name, attr->name_len, "level")) {
                            _parse_level(attr, &log->level);
                        } else if(!parse_strcasecmp(attr->name, attr->name_len, "additivity")) {
                            if(!parse_xmlattr_valuecmp(attr,"false")) {
                                log->additivity = false;
                            }
                        }
                    }
                    //ref
                    for(ref_node = logger_node->first_child; ref_node; ref_node = tmp_node->next) {
                        if (!parse_strcasecmp(ref_node->name, ref_node->name_len, "appender-ref")) {
                            for(attr = logger_node->attr; attr; attr = attr->next) {
                                if(!parse_strcasecmp(attr->name, attr->name_len, "name")) {
                                    string_t *ref = create_string();
                                    string_init_subcstr(ref, attr->value, attr->value_len);
                                    list_push_back(log->appender_ref, ref);
                                    break;
                                }
                            }
                        }
                    }
                    hash_map_insert_easy(config->loggers, log->name, log);
                } else if(!parse_strcasecmp(logger_node->name, logger_node->name_len, "root")) {
                    fxml_node_t *ref_node;
                    //属性
                    for(attr = logger_node->attr; attr; attr = attr->next) {
                        if(!parse_strcasecmp(attr->name, attr->name_len, "level")) {
                            _parse_level(attr, &config->root->level);
                            break;
                        }
                    }
                    //ref
                    for(ref_node = logger_node->first_child; ref_node; ref_node = tmp_node->next) {
                        if (!parse_strcasecmp(ref_node->name, ref_node->name_len, "appender-ref")) {
                            for(attr = logger_node->attr; attr; attr = attr->next) {
                                if(!parse_strcasecmp(attr->name, attr->name_len, "name")) {
                                    string_t *ref = create_string();
                                    string_init_subcstr(ref, attr->value, attr->value_len);
                                    list_push_back(config->root->appender_ref, ref);
                                    break;
                                }
                            }
                        }
                    }
                }//end root
            }//end logger node
        }//end loggers
    }//end root

    return 0;
}

static int _uv_log_init_conf_buff(uv_log_handle_t *h, char* conf_buff) {
    fxml_node_t *root = NULL;
    int ret = parse_json(&root, conf_buff);
    if(ret < 0) {
        ret = parse_xml(&root, conf_buff);
    }

    if (!ret) {
        ret = _parse_xml_config(root, h->config);
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

static void uv_loop_thread_cb(void* arg) {
    uv_log_handle_t* log = (uv_log_handle_t*)arg;
    while (true) {
        uv_run(&log->uv, UV_RUN_DEFAULT);
        Sleep(1000);
    }
}

static void uv_task_timer_cb(uv_timer_t* handle);
static uv_log_handle_t* _uv_log_init() {
    int uv_ret;
    uv_thread_t th;
    SAFE_MALLOC(uv_log_handle_t, logger);
    logger->config = create_config();
    logger->task_queue = create_task_fifo_queue();
    uv_ret = uv_loop_init(&logger->uv);
    if(uv_ret < 0) {
        printf("uv loop init failed: %s\n", uv_strerror(uv_ret));
    }
    logger->uv.data = logger;
    uv_ret = uv_timer_init(&logger->uv, &logger->task_timer);
    logger->task_timer.data = logger;
    uv_ret = uv_timer_start(&logger->task_timer, uv_task_timer_cb, 10, 10);
    uv_thread_create(&th, uv_loop_thread_cb, logger);
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
            printf("uv fs open %s failed:%s\n", fname[i], uv_strerror(ret));
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

    ret = uv_fs_open(NULL, &open_req, conf_path, O_RDONLY, 0, NULL);
    if(ret < 0) {
        printf("uv fs open %s failed:%s\n", conf_path, uv_strerror(ret));
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
    CHECK_POINT_INT(h,-1);
    destory_config(h->config);
    free(h);
    return 0;
}

void uv_log_write(uv_log_handle_t *h, char *name, int level, 
                  char *file_name, char *func_name, int line,
                  char* msg, ...) {
    va_list args;
    char buf[2048]={0};
    task_log_msg_t* task = create_task_log_msg();
    string_init_cstr(task->name, name);
    task->level = (level_t)level;
    task->file_name = file_name;
    task->func_name = func_name;
    task->line = line;
    va_start(args, msg);
    vsnprintf(buf, 2047, msg, args);
    va_end(args);
    string_init_cstr(task->msg, buf);

    add_task(h->task_queue, task);
}

static void log_task_job(uv_log_handle_t* h, task_log_msg_t* task) {
    hash_map_iterator_t it_pos = hash_map_find(h->config->loggers, task->name);
    hash_map_iterator_t it_end = hash_map_end(h->config->loggers);
    printf("%s,%s,%d,%s,%s\n", string_c_str(task->name), task->file_name, task->line, task->func_name, string_c_str(task->msg));
    do{
        pair_t* pt_pair;
        logger_t* logger;
        if (iterator_equal(it_pos, it_end)) {
            break;
        }
        pt_pair = (pair_t*)iterator_get_pointer(it_pos);
        logger = *(logger_t**)pair_second(pt_pair);
    }while (0);
}

static void uv_task_timer_cb(uv_timer_t* handle) {
    uv_log_handle_t* log = (uv_log_handle_t*)handle->data;
    get_task(log, log->task_queue, log_task_job);
}