#include "uv.h"
#include "st.h"
#include "uvLog.h"
#include "cptr/cptr.h"
#include "faster/fasterparse.h"


static int _uv_log_init_conf_buff(uv_log_handle_t *h, char* conf_buff) {
    faster_node_t* root = NULL;
    int ret = parse_json(&root, conf_buff);
    if(ret < 0) {
        ret = parse_xml(&root, conf_buff);
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