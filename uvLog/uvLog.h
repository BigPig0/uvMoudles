#ifndef _UV_LOG_H_
#define _UV_LOG_H_

#if ( defined _WIN32 )
#ifndef _WINDLL_FUNC
#ifdef UV_LOG_EXPORT
#define _WINDLL_FUNC		_declspec(dllexport)
#else
#define _WINDLL_FUNC		extern
#endif
#endif
#elif ( defined __unix ) || ( defined __linux__ )
#ifndef _WINDLL_FUNC
#define _WINDLL_FUNC        extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct _uv_log_handle_ uv_log_handle_t;

    _WINDLL_FUNC int uv_log_init(uv_log_handle_t **h);

    _WINDLL_FUNC int uv_log_init_conf(uv_log_handle_t **h, char* conf_path);

    _WINDLL_FUNC int uv_log_init_conf_buff(uv_log_handle_t **h, char* conf_buff);

    _WINDLL_FUNC int uv_log_close(uv_log_handle_t *h);

#ifdef __cplusplus
}
#endif
#endif