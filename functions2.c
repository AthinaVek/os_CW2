#include "functions2.h"

int sem_down(int sem_id) {
   struct sembuf sem_d;
   sem_d.sem_num = 0;
   sem_d.sem_op = -1;
   sem_d.sem_flg = 0;
   if (semop(sem_id,&sem_d,1) == -1) {
       return -1;
   }
   return 0;
}

int sem_up(int sem_id) {
   struct sembuf sem_d;
   sem_d.sem_num = 0;
   sem_d.sem_op = 1;
   sem_d.sem_flg = 0;
   if (semop(sem_id,&sem_d,1) == -1) {
       return -1;
   }
   return 0;
}

int sem_Init(int sem_id, int val) {
   union semun arg;
   arg.val = val;
   if (semctl(sem_id,0,SETVAL,arg) == -1) {
       perror("Semaphore Setting Value ");
       return -1;
   }
   return 0;
}

