#include "linux_ipc_shm.h"
using namespace SHM;

static void getMutex() {
    g_semKey = ftok(TOKEN_PATH, MUTEX_PROJID);
    int ret = semget(g_semKey, 1, IPC_CREAT | IPC_EXCL | 0755);
    if (ret < 0) {
        perror("semget");
        exit(-1);
    }
    g_semId = ret;
}

static void mutexInit() {
    getMutex();
    union semun arg;
    arg.val = 1;
    int ret = semctl(g_semId, 0, SETVAL, arg);
    if (ret < 0) {
        perror("semctl");
        exit(-1);
    }
}

void SHM::lock() {
    sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = -1;
    buf.sem_flg = SEM_UNDO;
    int ret = semop(g_semId, &buf, 1);
    if (ret < 0) {
        perror("semop");
        exit(-1);
    }
}

void SHM::unlock() {
    sembuf buf;
    buf.sem_num = 0;
    buf.sem_op = 1;
    buf.sem_flg = SEM_UNDO;
    int ret = semop(g_semId, &buf, 1);
    if (ret < 0) {
        perror("semop");
        exit(-1);
    }
}


int SHM::initShmPool() {
    size_t bucketArrSize = sizeof(Element*) * g_bucketNum;
    TotalElemNum = SHM_SIZE / sizeof(Element);
    UsedElemNum = 0;
    size_t realSize = TotalElemNum * sizeof(Element);
    size_t totalSize = realSize + bucketArrSize;
    g_shmKey = ftok(TOKEN_PATH, SHM_PROJID);
    
    mutexInit();
    int ret = shmget(g_shmKey, totalSize, IPC_CREAT | IPC_EXCL | 0755);
    if (ret < 0) {
        perror("shmget");
        exit(-1);
    }
    g_shmId = ret;
    pShmHead = (Element*)shmat(ret, nullptr, 0);
    memset(pShmHead, totalSize, 0);
    Element *pcur;
    for (int i = 0; i < TotalElemNum - 1; i++) {
        pcur = pShmHead + i;
        pcur->index = i;
        pcur->next = i + 1;
        pShmHead[pcur->next].prev = i;
    }
    pShmHead->prev = -1;
    pShmHead[pcur->next].index = TotalElemNum - 1;
    pShmHead[pcur->next].next = -1;
    pShmTail = &pShmHead[pcur->next];
    pHashTable = (Element**)(pShmTail + 1);
    return 0;
}

void SHM::destroy() {
    int ret = shmctl(g_shmId, IPC_RMID, nullptr);
    if (ret < 0) {
        perror("shmctl");
        exit(-1);
    }
    ret = semctl(g_semId, 0, IPC_RMID);
    if (ret < 0) {
        perror("semctl");
        exit(-1);
    }
}

int SHM::getShmPool() {
    getMutex();
    g_shmKey = ftok(TOKEN_PATH, SHM_PROJID);
    int ShmId = shmget(g_shmKey, SHM_SIZE, 0755);
    if (ShmId < 0) {
        perror("shmget");
        exit(-1);
    }
    TotalElemNum = SHM_SIZE / sizeof(Element);
    UsedElemNum = 0;
    pShmHead = (Element*)shmat(ShmId, nullptr, 0);
    pShmTail = pShmHead + TotalElemNum - 1;
}

Element *SHM::shmAlloc() {
    if (UsedElemNum == TotalElemNum - 2) {
        cerr << "share memory use up!!" << endl;
        exit(-1);
    }
    lock();
    Element *palloc = pShmHead + pShmHead->next;
    pShmHead->next = palloc->next;
    pShmHead[palloc->next].prev = 0;
    unlock();
    return palloc;
}

void SHM::shmFree(Element *p) {
    lock();
    pShmHead[pShmTail->prev].next = p->index;
    p->next = TotalElemNum - 1;
    p->prev = pShmTail->prev;
    pShmTail->prev = p->index;
    unlock();
}

Element *find(int jobId) {
    int index = jobId % g_bucketNum;
    Element *pcur = pHashTable[index];
    while (pcur) {
        if (pcur->jobId == jobId) {
            break;
        }
        pcur = NEXT(pcur);
    }
    return pcur;
}

void SHM::insert(int jobId, char *text) {
    lock();
    Element *p;
    if (p = find(jobId)) {
        memcpy(text, p->queryInfo, strlen(text) + 1);
        return ;
    }
    cout << "can not find result of jobid: " << jobId << endl;
    p = shmAlloc();
    p->jobId = jobId;
    memcpy(text, p->queryInfo, strlen(text) + 1);
    p->next = pHashTable[jobId % g_bucketNum]->index;
    pHashTable[jobId % g_bucketNum]->prev = p->index;
    pHashTable[jobId % g_bucketNum] = p;
    unlock();
}