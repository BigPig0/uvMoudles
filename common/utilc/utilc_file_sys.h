/**
 * 文件系统操作的方法
 */

#ifndef _UTILC_FILE_SYS_
#define _UTILC_FILE_SYS_

#include "utilc_export.h"

#if defined(WINDOWS_IMPL)
#define LINE_SEPARATOR "\r\n"
#elif defined(APPLE_IMPL)
#define LINE_SEPARATOR "\r"
#else
#define LINE_SEPARATOR "\n"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 检查一个文件的上级目录是否存在，如果不存在则创建。
 * 如果输入的是一个目录，即以'\\'或'/'结尾，则检查并创建该目录。
 * 如果输入的是一个文件，非'\\'或'/'结尾，只创建上级目录，不创建文件。
 * @return 0成功  -1失败
 */
_UTILC_API int file_sys_check_path(const char *path);

/**
 * 检查文件是否存在
 * @return 0存在 -1不存在
 */
_UTILC_API int file_sys_exist(const char *path);

#ifdef __cplusplus
}
#endif
#endif