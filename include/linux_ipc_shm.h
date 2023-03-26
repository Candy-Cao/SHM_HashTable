#ifndef LINUX_IPC_SHM_H
#define LINUX_IPC_SHM_H

#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
using namespace std;

#define MAX_TEXT_LEN 1000
#define SHM_PROJID 0x7258
#define MUTEX_PROJID 0x9484
#define SHM_SIZE 5e7
#define TOKEN_PATH "/tmp"
#define NEXT(p) ((p->next != -1) ? (&pShmHead[p->next]) : nullptr)
namespace SHM {

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                (Linux-specific) */
};
typedef struct Element {
    int jobId;      //key
    int index;
    int next;
    int prev;
    char queryInfo[MAX_TEXT_LEN];
} Element;

Element *pShmHead = nullptr;
Element *pShmTail = nullptr;
Element **pHashTable = nullptr;
int g_shmKey;
int g_shmId;
int TotalElemNum;
int UsedElemNum;
int g_semId;
int g_semKey;
const int g_bucketNum = 10009;

int initShmPool();

void destroy();

int getShmPool();

Element *shmAlloc();

void shmFree(Element *p);

void lock();

void unlock();

void insert(int jobId, char *text);
}

#endif