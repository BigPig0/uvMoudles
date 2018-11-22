#ifndef _UV_IPC_H_
#define _UV_IPC_H_

#if ( defined _WIN32 )
#ifndef _UV_IPC_API
#ifdef UV_IPC_EXPORT
#define _UV_IPC_API		_declspec(dllexport)
#else
#define _UV_IPC_API		extern
#endif
#endif
#elif ( defined __unix ) || ( defined __linux__ )
#ifndef _UV_IPC_API
#define _UV_IPC_API        extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif
#endif