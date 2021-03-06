#ifndef __QEMU_THREAD_WIN32_H
#define __QEMU_THREAD_WIN32_H 1
#include "windows.h"

typedef struct QemuThreadData QemuThreadData;
struct QemuThread {
    QemuThreadData *data;
    unsigned tid;
};

/* Only valid for joinable threads.  */
HANDLE qemu_thread_get_handle(QemuThread *thread);

#endif
