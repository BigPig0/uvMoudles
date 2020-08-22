#ifndef _UV_HTTP_PUBLIC_DEF_
#define _UV_HTTP_PUBLIC_DEF_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _HTTP_METHOD_
{
    METHOD_OPTIONS = 0,
    METHOD_HEAD,
    METHOD_GET,
    METHOD_POST,
    METHOD_PUT,
    METHOD_DELETE,
    METHOD_TRACE,
    METHOD_CONNECT
}HTTP_METHOD;

typedef struct _uv_node_  uv_node_t;


typedef struct _response_ response_t;

#define REQUEST_PUBLIC \
    HTTP_METHOD method;\
    const char* url;\
    const char* host;\
    int         keep_alive;\
    int         chunked;\
    int         content_length;\
    void*       user_data;

    typedef struct _request_ {
        /*
        HTTP_METHOD method;		    //http请求方法
        const char* url;		    //一个完整的http地址("http://"可以省略)
        const char* host;			//http请求优先使用host作为目标地址，当host为空时从url中获取目标地址
        int         keep_alive;     //0表示Connection为close，非0表示keep-alive
        int         chunked;        //POST使用 0表示不使用chuncked，非0表示Transfer-Encoding: "chunked"
        int         content_length; //POST时需要标注内容的长度
        void*       user_data;      //设置用户数据
        */
        REQUEST_PUBLIC
        response_t* res;
    }request_t;

    

#define RESPONSE_PUBLIC \
    int         status;\
    int         keep_alive;\
    int         chunked;\
    int         content_length;

    typedef struct _response_ {
        /*
        int         status;
        int         keep_alive;     //0表示Connection为close，非0表示keep-alive
        int         chunked;        //POST使用 0表示不使用chuncked，非0表示Transfer-Encoding: "chunked"
        int         content_length; //POST时需要标注内容的长度
        */
        RESPONSE_PUBLIC
        request_t*  req;
    }response_t;

    /** 发送http请求回调函数，参数为错误码和请求句柄 */
    typedef void(*request_cb)(request_t*, int);
    /** 设置收到应答的内容 */
    typedef void(*response_data)(request_t*, char*, int);
    /** 设置应答接收完成的处理 */
    typedef void(*response_cb)(request_t*, int);



    /** 根据错误码获取详细的错误信息 */
    extern const char* uvhttp_err_msg(int err);

    /** 公共的工具方法 */
    extern char* url_encode(char* src);
    extern char* url_decode(char* src);


#ifdef __cplusplus
}
#endif
#endif
