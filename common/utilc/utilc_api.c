#include "utilc_api.h"

#if defined(WINDOWS_IMPL)
#include <windows.h>

int getpid() {
    return GetCurrentProcessId();
}

int gettid() {
    return GetCurrentThreadId();
}

void sleep(uint32_t dwMilliseconds) {
    Sleep(dwMilliseconds);
}
#endif