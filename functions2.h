#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/times.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <string.h>
#include <sys/ptrace.h>
#include <signal.h>
#include <time.h>



struct hashStruct{
    int page;
    int frame;
    char action;  // W or R
    int pm;
    struct hashStruct *next;
};

struct hash{
   struct hashStruct *head;
   int count;
};

struct sharedStruct{
    char buf[12];
    int pm;
};

union semun {
   int val;
   struct semid_ds *buf;
   unsigned short *array;
};

int sem_down(int);
int sem_up(int);
int sem_Init(int, int);