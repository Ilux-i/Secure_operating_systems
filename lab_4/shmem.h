#ifndef SHMEM_H
#define SHMEM_H

#include <sys/types.h>
#include <sys/ipc.h>

#define SHMEM_KEY_PATH      "/tmp"
#define SHMEM_KEY_ID        42
#define SHARED_MEM_CAPACITY 4096
#define HALT_MESSAGE        "sender dead"

static key_t shmem_get_key(void)
{
    return ftok(SHMEM_KEY_PATH, SHMEM_KEY_ID);
}

#endif