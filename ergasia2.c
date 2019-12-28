#include "functions2.h"

struct hash *hashTable=NULL;

struct hashStruct *createNode(struct hashStruct hNode){
   struct hashStruct *newnode;
   newnode = (struct hashStruct*) malloc(sizeof(struct hashStruct));
   *newnode = hNode;
   newnode->next = NULL;
   return newnode;
}

void insertPgHash(struct hashStruct hNode, int numFrames, int *diskW, int *pgFaults, int k){
    int hashindex=hNode.page%numFrames;
    struct hashStruct *newnode = createNode(hNode);

    if(!hashTable[hashindex].head){
	    hashTable[hashindex].head = newnode;
	    hashTable[hashindex].count = 1;
	    printf("INSERT TO HASHED TABLE: %d,FROM PM%d\n", hashTable[hashindex].head->page, hNode.pm);
    }

    else {
	    newnode->next = (hashTable[hashindex].head);
	    hashTable[hashindex].head = newnode;
	    hashTable[hashindex].count++;
	    printf("INSERT TO HASHED TABLE: %d,FROM PM%d\n", hashTable[hashindex].head->page, hNode.pm);
    }

    if((*pgFaults) == k+1){
    	struct hashStruct *s1node, *s2node;
    	
    	for(int i=0; i<numFrames; i++){
    		s1node = hashTable[i].head;
    		if (s1node){
	    		while (s1node != NULL){
	    			// printf("########## %c   %d   %d  +++%d\n", s1node->action, s1node->pm, s1node->page, hNode.pm);
	    			if(s1node->pm == hNode.pm){
	    				if (s1node->action == 'W'){
		    				// printf("^^^^^^^^^\n");
			    			(*diskW)++;
			    		}
			    		if (s1node == hashTable[i].head){
			    			hashTable[i].head = s1node->next;
			    			free(s1node);
			    			s1node = hashTable[i].head;
			    		}
			    		else {
			    			s2node->next = s1node->next;
			    			free(s1node);
			    			s1node = s2node->next;
			    		}
		    		}
		    		else {
		    			s2node = s1node;
						s1node = s1node->next;		    			
		    		}
	    		}
	    	}
    	}
    	(*pgFaults) = 0;
    }

    return;
}

int searchUpHashPT(struct hashStruct hNode, int numFrames){
    int hashindex=hNode.page%numFrames, flag = 0;
    struct hashStruct *snode;
    snode = hashTable[hashindex].head;

    if(!snode) return flag;

    while(snode != NULL){
        if((snode->page == hNode.page) && (snode->pm == hNode.pm)){
            flag = 1;
			if(snode->action == 'R') snode->action = hNode.action;
			// searnode->timetoken = time;
            break;
        }
        snode = snode->next;
    }
    return flag;
}



////////////////   MAIN   ///////////////////

int main(int argc, char *argv[]){
	int k, q, numFrames, max;
	int *memory, addr, frame=0, gframe=0, bframe=0;
	int gPgFaults=0, bPgFaults=0, pgFaults=0, diskW=0, rDisk=0;
	char action, lPage[6], locAddr, *temp;
	FILE *gccTrace, *bzipTrace;
	pid_t pid;
	int pm_mutex, mm_mutex;
    int shm;
    struct sharedStruct *shrd;

	if (argc == 4 || argc == 5){
		k = atoi(argv[1]);
		numFrames = atoi(argv[2]);
		q = atoi(argv[3]);
		if (argc == 5)
			max = atoi(argv[4]);
		else if (argc == 4)
			max = 2000000;
	}
	else{
		printf("Wrong number of arguments.\n");
		return -1;
	}

	memory = malloc(numFrames*sizeof(int));
	for(int i=0; i<numFrames; i++)
		memory[i] = 0;

	hashTable = malloc(numFrames*sizeof(struct hash));

	if((gccTrace=fopen("./gcc.trace", "r")) == NULL){
		printf("Error gcc.trace.\n");
		return -1;
	}
    if ((bzipTrace = fopen("./bzip.trace", "r")) == NULL){
		printf("Error bzip.trace.\n");
		return -1;
	}

	/////// SEMAPHORES ////////
	pm_mutex = semget(IPC_PRIVATE,1,IPC_CREAT | 0666);
	if (pm_mutex == -1) {
		perror("Semaphore Creation\n");
		semctl(pm_mutex,0,IPC_RMID);
		exit(0);
	}
	if (sem_Init(pm_mutex,1) == -1) {
		perror("Semaphore Initialize\n");
		semctl(pm_mutex,0,IPC_RMID);
		exit(0);
	}
        
    mm_mutex = semget(IPC_PRIVATE,1,IPC_CREAT | 0666); 
	if (mm_mutex == -1) {   
		perror("Semaphore Creation\n");
		semctl(mm_mutex,0,IPC_RMID);
		exit(0);
	}
	if (sem_Init(mm_mutex,0) == -1) {
		perror("Semaphore Initialize\n");
		semctl(mm_mutex,0,IPC_RMID);
		exit(0);
	}

	/////// SHARED MEMORY ////////
    shm = shmget(IPC_PRIVATE,sizeof(struct sharedStruct),0666|IPC_CREAT);
	if (shm == -1) {
    	perror("Error in input shared memory creation(PAINTER)\n");
		shmctl(shm,IPC_RMID,0);
		exit(0);
	}
	shrd = (struct sharedStruct*)shmat(shm,(struct sharedStruct*)0,0);
	if (shrd == (struct sharedStruct*)-1 ) {
    	perror("Error in shared memory attach(PAINTER)\n");
    	shmctl(shm,IPC_RMID,0);
    	exit(0);
	}
	int count = 0;

    //////// PROCESSES /////////
    for (int i=0; i<2; i++){
    	pid = fork();
    	if (pid == 0) 
        { 
        	if (i == 0){               ////////  PM1  /////////
            	char gccBuf[12];
            	int reqs=0, j=0;

            	while (reqs<max && j<q){  // read from gcc.trace file
            		gccBuf[0] = '\0';
	            	fgets(gccBuf, sizeof(gccBuf), gccTrace);
	            	sem_down(pm_mutex);
	            	memcpy(shrd->buf, gccBuf, sizeof(gccBuf));  // send request to MM
	            	shrd->pm = 1;
					reqs++;
	            	j++;
	            	sem_up(mm_mutex);
	            }
            }
            else if (i == 1){          ////////  PM2  /////////
            	char bzipBuf[12];
            	int reqs=0, j=0;

            	while (reqs<max && j<q){
            		bzipBuf[0] = '\0';
	            	fgets(bzipBuf, sizeof(bzipBuf), bzipTrace);  // read from bzip.trace file
	            	sem_down(pm_mutex);
	            	memcpy(shrd->buf, bzipBuf, sizeof(bzipBuf));  // send request to MM
	            	shrd->pm = 2;
	            	reqs++;
	            	j++;
	            	sem_up(mm_mutex);
	            }
            }
            exit(0); 
        }
        else if (pid < 0){
        	perror("fork");
    		return -1;
        }
    } 
    if (pid > 0){             ////////  MM  /////////
    	char buf[12];
    	int reqs=0, j=0, pm;
    	struct hashStruct hNode;

    	while (reqs<2*max && j<2*q){
        	sem_down(mm_mutex);
        	memcpy(buf, shrd->buf, sizeof(buf));
        	pm = shrd->pm;
        	strncpy(lPage, buf, 5);
        	lPage[5] = '\0';
        	hNode.page = (int)strtol(lPage, NULL, 16);
        	temp = strtok(buf, " ");
        	temp = strtok(NULL, "\n");
        	hNode.action = temp[0];
        	hNode.pm = pm;

        	if (searchUpHashPT(hNode, numFrames) == 0){  // check if it exists in hashed table
    			if (pm == 1){   // recieve from PM1
    				gPgFaults++;
					insertPgHash(hNode, numFrames, &diskW, &gPgFaults, k);  // insert to hashed table
					memory[gframe] = hNode.page;
					gframe++;
				}
				else if (pm == 2){   // recieve from PM2
					bPgFaults++;
					insertPgHash(hNode, numFrames, &diskW, &bPgFaults, k);  // insert to hashed table
					memory[(numFrames/2)+bframe] = hNode.page;
					bframe++;
				}
				pgFaults++;
				rDisk++;
				frame++;
        	}
        	count++;
            reqs++;
            j++;
        	sem_up(pm_mutex);
        }
        struct hashStruct *snode;   // print the nodes left in hashed table
        for(int i=0; i<numFrames; i++){
    		snode = hashTable[i].head;
    		while (snode){
    			printf("LEFT IN HASHED TABLE: %d\n", snode->page);
    			snode = snode->next;
    		}
    	}
    }
    for(int i=0; i<2; i++)
    	wait(NULL); 

    printf("\n\n*******************************************\n");
    printf("GCC.TRACE ENTRIES TO HASHED TABLE: %d\n", gframe);
    printf("BZIP.TRACE ENTRIES TO HASHED TABLE: %d\n", bframe);
    printf("TOTAL WRITES TO DISK: %d\n", diskW);
    printf("TOTAL READS FROM DISK: %d\n", rDisk);
    printf("TOTAL PAGE FAULTS: %d\n", pgFaults);
    printf("*******************************************\n\n");

    fclose(gccTrace);
    fclose(bzipTrace);

    shmdt((char*)shrd);
	if(shmctl(shm,IPC_RMID,0) == -1)
    	return -1;

    if (semctl(pm_mutex,0,IPC_RMID) == -1)
    	return -1;
    if (semctl(mm_mutex,0,IPC_RMID) == -1)
    	return -1;
   
    struct hashStruct *node1, *node2;   // print the nodes left in hashed table
    for(int i=0; i<numFrames; i++){
		node1 = hashTable[i].head;
		while (node1){
			node2 = node1;
			node1 = node1->next;
			free(node2);
		}
	}

	free(memory);	

    return 0;
}

