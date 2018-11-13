#include "uv.h"
#include "st.h"
#include "uvLog.h"
#include "cptr/cptr.h"
#include "faster/fasterjson.h"
#include "faster/fasterxml.h"


int CallbackOnJsonNode(int type, char *jpath, int jpath_len, int jpath_size
                       , char *nodename, int nodename_len
                       , char *content, int content_len, void *p ) {
                           if( type & FASTERJSON_NODE_BRANCH ) {
                               if( type & FASTERJSON_NODE_ENTER ) {
                                   printf( "ENTER-BRANCH p[%s] jpath[%.*s] nodename[%.*s]\n" , (char*)p , jpath_len , jpath , nodename_len , nodename );
                               } else if( type & FASTERJSON_NODE_LEAVE ) {
                                   printf( "LEAVE-BRANCH p[%s] jpath[%.*s] nodename[%.*s]\n" , (char*)p , jpath_len , jpath , nodename_len , nodename );
                               }
                           } else if( type & FASTERJSON_NODE_ARRAY ) {
                               if( type & FASTERJSON_NODE_ENTER ) {
                                   printf( "ENTER-ARRAY  p[%s] jpath[%.*s] nodename[%.*s]\n" , (char*)p , jpath_len , jpath , nodename_len , nodename );
                               } else if( type & FASTERJSON_NODE_LEAVE ) {
                                   printf( "LEAVE-ARRAY  p[%s] jpath[%.*s] nodename[%.*s]\n" , (char*)p , jpath_len , jpath , nodename_len , nodename );
                               }
                           } else if( type & FASTERJSON_NODE_LEAF ) {
                               printf( "LEAF         p[%s] jpath[%.*s] nodename[%.*s] content[%.*s]\n" , (char*)p , jpath_len , jpath , nodename_len , nodename , content_len , content );
                           }

                           return 0;
}

int parse_json(uv_log_handle_t *h, char* buff) {
    char	jpath[ 1024 + 1 ] ;
    int     nret = 0;

    memset( jpath , 0x00 , sizeof(jpath) );
    nret = TravelJsonBuffer( buff , jpath , sizeof(jpath) , & CallbackOnJsonNode , (void*)h ) ;
    if( nret && nret != FASTERJSON_ERROR_END_OF_BUFFER ){
        printf( "TravelJsonTree failed[%d]\n" , nret );
    }
    return nret;
}

int CallbackOnXmlProperty(int type,  char *xpath, int xpath_len, int xpath_size
                          , char *propname, int propname_len
                          , char *propvalue, int propvalue_len
                          , char *content , int content_len , void *p ) {
    printf( "    PROPERTY xpath[%s] propname[%.*s] propvalue[%.*s]\n", xpath , propname_len , propname , propvalue_len , propvalue );

    return 0;
}

int CallbackOnXmlNode(int type, char *xpath, int xpath_len, int xpath_size
                      , char *nodename , int nodename_len 
                      , char *properties , int properties_len 
                      , char *content , int content_len , void *p ) {
    int nret = 0 ;
    uv_log_handle_t* h = (uv_log_handle_t*)p;

    if( type & FASTERXML_NODE_BRANCH ) {
        if( type & FASTERXML_NODE_ENTER ) {
            printf( "ENTER-BRANCH p[%s] xpath[%s] nodename[%.*s] properties[%.*s]\n" , (char*)p , xpath , nodename_len , nodename , properties_len , properties );
        } else if( type & FASTERXML_NODE_LEAVE ) {
            printf( "LEAVE-BRANCH p[%s] xpath[%s] nodename[%.*s] properties[%.*s]\n" , (char*)p , xpath , nodename_len , nodename , properties_len , properties );
        } else {
            printf( "BRANCH       p[%s] xpath[%s] nodename[%.*s] properties[%.*s]\n" , (char*)p , xpath , nodename_len , nodename , properties_len , properties );
        }
    } else if( type & FASTERXML_NODE_LEAF ) {
        printf( "LEAF         p[%s] xpath[%s] nodename[%.*s] properties[%.*s] content[%.*s]\n" , (char*)p , xpath , nodename_len , nodename , properties_len , properties , content_len , content );
    }

    if( properties && properties[0] ) {
        nret = TravelXmlPropertiesBuffer(properties, properties_len, type, xpath, xpath_len, xpath_size, content, content_len, CallbackOnXmlProperty, p) ;
        if ( nret )
            return nret;
    }

    return 0;
}

int parse_xml(uv_log_handle_t *h, char* buff) {
    char	xpath[ 1024 + 1 ] ;
    int     nret = 0 ;

    memset( xpath , 0x00 , sizeof(xpath) );
    nret = TravelXmlBuffer( buff , xpath , sizeof(xpath) , CallbackOnXmlNode , h) ;
    if( nret && nret != FASTERXML_ERROR_END_OF_BUFFER ) {
        printf( "TravelXmlTree failed[%d]\n" , nret );
        return nret;
    }
    return nret;
}

static int _uv_log_init_conf_buff(uv_log_handle_t *h, char* conf_buff) {
    int ret = parse_json(h, conf_buff);
    if(ret < 0) {
        ret = parse_xml(h, conf_buff);
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