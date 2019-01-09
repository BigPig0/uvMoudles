#ifndef _UV_HTTP_AGENT_
#define _UV_HTTP_AGENT_ 

#ifdef __cplusplus
extern "C" {
#endif

#include "public.h"


/**  */
typedef struct _agent_options_ {
    int         keep_alive; //Keep sockets around even when there are no outstanding requests, so they can be used for future requests without having to reestablish a TCP connection. Default: false.
    int         keep_alive_msecs; //When using the keepAlive option, specifies the initial delay for TCP Keep-Alive packets. Ignored when the keepAlive option is false or undefined. Default: 1000.
    int         max_sockets; //Maximum number of sockets to allow per host. Default: Infinity.
    int         max_free_sockets; //Maximum number of sockets to leave open in a free state. Only relevant if keepAlive is set to true. Default: 256.
    int         timeout; //Socket timeout in milliseconds. This will set the timeout after the socket is connected.
}agent_options_t;

typedef struct _http_agent_ {
    int i;
}http_agent_t;

typedef http_agent_t* (*agent_create_connection)(agent_options_t* options);

extern http_agent_t* new_agent(agent_options_t* options);

#ifdef __cplusplus
}
#endif

#endif