//
// Virual Memory Simulator Homework
// One-level page table system with FIFO and LRU
// Two-level page table system with LRU
// Inverted page table with a hashing system 
// Submission Year: 2021 - 2
// Student Name:	이규민
// Student Number:	B711124
// Operating System
// Virtual Memory Maangement Simulator(hw3)
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define PAGESIZEBITS 12			// page size = 4Kbytes
#define VIRTUALADDRBITS 32		// virtual address space size = 4Gbytes

int numProcess;
int firstLevelBits;
int nFrame=1;
int phyMemSizeBits;

struct procEntry {
	char *traceName;			// the memory trace name
	int pid;					// process (trace) id
	int ntraces;				// the number of memory traces
	int num2ndLevelPageTable;	// The 2nd level page created(allocated);
	int numIHTConflictAccess; 	// The number of Inverted Hash Table Conflict Accesses
	int numIHTNULLAccess;		// The number of Empty Inverted Hash Table Accesses
	int numIHTNonNULLAcess;		// The number of Non Empty Inverted Hash Table Accesses
	int numPageFault;			// The number of page faults
	int numPageHit;				// The number of page hits
	struct pageTableEntry *firstLevelPageTable;
	FILE *tracefp;
} *procTable;

void initProcTable() {
	int i; 
	for(i =0;i<numProcess;i++){
		procTable[i].pid = -1;
		procTable[i].ntraces = 0;
		procTable[i].num2ndLevelPageTable = 0;
		procTable[i].numIHTConflictAccess = 0;
		procTable[i].numIHTNULLAccess = 0;
		procTable[i].numIHTNonNULLAcess = 0;
		procTable[i].numPageFault=0;
		procTable[i].numPageHit = 0;
	}
}

struct pageTableEntry {
	int procid;
	int index;
	int len;
	int index1, index2;
	int hashindex;
	struct pageTableEntry *prev;
	struct pageTableEntry *next;
};

struct pageTableEntry pageQueue;

struct pageTableEntry* lastQueue(struct pageTableEntry *queue) {
	struct pageTableEntry *last;
	last = queue;
	while(last->next != last){
		last = last->next;
	}
	return last;
}

void enfirstQueue(struct pageTableEntry *queue, struct pageTableEntry *proc){
	if(queue->len==0){
		queue->next = proc;
		proc->next = proc;
		proc->prev = queue;
	}
	else{
		proc->next = queue->next;
		queue->next->prev = proc;
		queue->next = proc;
		proc->prev = queue;
	}
}

void delastQueue(struct pageTableEntry *queue) {
	struct pageTableEntry *last;
	last = lastQueue(queue);

	last->prev->next = last->prev;
	last->next = NULL;
	last->prev = NULL;
}

void deQueue(struct pageTableEntry *queue, struct pageTableEntry *proc){
  if(proc->next==proc){
    proc->prev->next = proc->prev;
    proc->next = NULL;
    proc->prev = NULL;
  }
  else{
    proc->next->prev = proc->prev;
    proc->prev->next = proc->next;
    proc->next = NULL;
    proc->prev = NULL;
  }
}

void replaceLRU(struct pageTableEntry *queue, struct pageTableEntry *proc){
	if(queue->next == proc){ //	첫번째
		return;
	}
	else if(proc->next == proc){	//	마지막
		proc->prev->next = proc->prev;

		queue->next->prev = proc;
		proc->next = queue->next;
		queue->next = proc;
		proc->prev = queue;
	}
	else{
		proc->next->prev = proc->prev;
		proc->prev->next = proc->next;

		queue->next->prev = proc;
		proc->next = queue->next;
		queue->next = proc;
		proc->prev = queue;
	}
}

void oneLevelVMSim(int replace) {
	int flag=1;
	int i,j;
	int faultcheck=1;
	
	unsigned addr;
	char rw;
	struct pageTableEntry *tmp;

	while(flag){
		struct pageTableEntry *pte = malloc(sizeof(struct pageTableEntry) * numProcess);
		for(i=0;i<numProcess;i++){
			pte[i].procid = i;
			pte[i].next = NULL;
			pte[i].prev = NULL;
			if(fscanf(procTable[i].tracefp, "%x %c", &addr,&rw) == EOF){
				flag = 0;
				break;
			}
			addr /= 4096;
			pte[i].index = addr;
			procTable[i].ntraces++;
			if(replace==0){
				tmp = &pageQueue;
				for(j=0;j<pageQueue.len+1;j++){
					if(pte[i].procid == tmp->procid && pte[i].index == tmp->index){
						procTable[i].numPageHit++;
						faultcheck=0;
						break;
					}
					tmp=tmp->next;
				}
				if(faultcheck==1){
					procTable[i].numPageFault++;
					if(pageQueue.len==nFrame){
						enfirstQueue(&pageQueue, &pte[i]);
						delastQueue(&pageQueue);
					}
					else{
						enfirstQueue(&pageQueue, &pte[i]);
						pageQueue.len++;
					}
				}
				faultcheck=1;
			}
			else{
				tmp = &pageQueue;
				for(j=0;j<pageQueue.len+1;j++){
					if(pte[i].procid==tmp->procid&&pte[i].index==tmp->index){
						procTable[i].numPageHit++;
						faultcheck=0;
						replaceLRU(&pageQueue,tmp);
						break;
					}
					tmp=tmp->next;
				}
				if(faultcheck==1){
					procTable[i].numPageFault++;
					if(pageQueue.len==nFrame){
						enfirstQueue(&pageQueue, &pte[i]);
						delastQueue(&pageQueue);
					}
					else{
						enfirstQueue(&pageQueue, &pte[i]);
						pageQueue.len++;
					}
				}
				faultcheck=1;
			}
		}
	}

	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
	
}

void twoLevelVMSim(int firstsize) {
	int i,j,k;
	int flag = 1;
	int check;
	int n=1;
	int size=1;
	int firstpagefault = 1; // 1이면 Fault
	int secondpagefault = 1; //	1이면 Fault
	unsigned addr;
	char rw;
	struct pageTableEntry *tmp;
	struct pageTableentry *find;

	for(i=0;i<firstsize;i++){
		n*=2;
	}
	for(i=0;i<(20-firstsize);i++){
		size*=2;
	}

	int **ary = malloc(sizeof(int *) * numProcess);
	for(i=0;i<numProcess;i++){
		ary[i]=malloc(sizeof(int) * n);
	}
	for(i=0;i<numProcess;i++){
		for(j=0;j<n;j++){
			ary[i][j]=0;
		}
	}

	while(flag){
		struct pageTableEntry *pte = malloc(sizeof(struct pageTableEntry) * numProcess); //	이거

		for(i=0;i<numProcess;i++){
			pte[i].procid = i;
			pte[i].next = NULL;
			pte[i].prev = NULL;
			if(fscanf(procTable[i].tracefp, "%x %c", &addr, &rw) == EOF){
				flag = 0;
				break;
			}
			addr /= 4096;
			pte[i].index = addr;
			pte[i].index1 = addr/size;
			pte[i].index2 = addr%size;
			check = addr/size;
			procTable[i].ntraces++;
			if(ary[i][check]==0){
				ary[i][check]=1;
				procTable[i].num2ndLevelPageTable++;
			}
			tmp = &pageQueue;
			for(j=0;j<pageQueue.len+1;j++){
				if(pte[i].procid==tmp->procid && pte[i].index1==tmp->index1){
					firstpagefault=0;
					if(pte[i].index2==tmp->index2){
						secondpagefault=0;
						procTable[i].numPageHit++;
						replaceLRU(&pageQueue,tmp);
					}
				}
				tmp=tmp->next;
			}
			if(firstpagefault==0&&secondpagefault==1){
				procTable[i].numPageFault++;
					if(pageQueue.len==nFrame){
						enfirstQueue(&pageQueue, &pte[i]);
						delastQueue(&pageQueue);
					}
					else{
						enfirstQueue(&pageQueue, &pte[i]);
						pageQueue.len++;
					}
			}
			if(firstpagefault==1&&secondpagefault==1){
				procTable[i].numPageFault++;
					if(pageQueue.len==nFrame){
						enfirstQueue(&pageQueue, &pte[i]);
						delastQueue(&pageQueue);
					}
					else{
						enfirstQueue(&pageQueue, &pte[i]);
						pageQueue.len++;
					}
			}
			firstpagefault=1;
			secondpagefault=1;
		}
	}

	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of second level page tables allocated %d\n",i,procTable[i].num2ndLevelPageTable);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
}

void invertedPageVMSim() {
	int i,j,k;
	int flag=1;
	int faultcheck=1;
	int firsthash; // 처음으로 hashtable에 접근해서 찾을때 쓰는 인자
	int findhash, findprocid, findindex;	// HT, PQ에서 빼거나 값 찾을때 쓰는 인자
	unsigned addr;
	char rw;
	struct pageTableEntry *tmp; //	중간에 pageQueue나 hashtablequeue에 접근할때 쓰는 인자
	struct pageTableEntry *tmp1;
	
	struct pageTableEntry *hashTable = malloc(sizeof(struct pageTableEntry) * nFrame); //	프레임수만큼
	for(i=0;i<nFrame;i++){
		hashTable[i].procid=-1;
		hashTable[i].index=-1;
		hashTable[i].len=0;
		hashTable[i].next = NULL;
		hashTable[i].prev = NULL;
	}

	while(flag){
		struct pageTableEntry *pte = malloc(sizeof(struct pageTableEntry) * numProcess);
		struct pageTableEntry *hash = malloc(sizeof(struct pageTableEntry) * numProcess);

		for(i=0;i<numProcess;i++){
			pte[i].procid = i;
			pte[i].next = NULL;
			pte[i].prev = NULL;
			hash[i].procid = i;
			hash[i].next = NULL;
			hash[i].prev = NULL;
			if(fscanf(procTable[i].tracefp, "%x %c", &addr,&rw) == EOF){
				flag = 0;
				break;
			}
			addr /= 4096;
			pte[i].index = addr;
			hash[i].index = addr;
			procTable[i].ntraces++;
			pte[i].hashindex = (pte[i].index + pte[i].procid) % nFrame;
			hash[i].hashindex = (hash[i].index + hash[i].procid) % nFrame;
			firsthash=hash[i].hashindex;

			if(hashTable[firsthash].len==0){	// 연결노드 없음
				procTable[i].numPageFault++;
				procTable[i].numIHTNULLAccess++;
				enfirstQueue(&hashTable[firsthash], &hash[i]);
				hashTable[firsthash].len++;
				if(pageQueue.len==nFrame){ //	pageQueue 다참
					findhash=lastQueue(&pageQueue)->hashindex;
					findprocid=lastQueue(&pageQueue)->procid;
					findindex=lastQueue(&pageQueue)->index;
					// hashtable에 찾으러 감
					tmp=&hashTable[findhash];
					for(j=0;j<hashTable[findhash].len+1;j++){
						if(tmp->procid==findprocid&&tmp->index==findindex){
							deQueue(&hashTable[findhash],tmp);
							break;
						}
						tmp=tmp->next;
					}
					hashTable[findhash].len--;
					enfirstQueue(&pageQueue,&pte[i]);
					delastQueue(&pageQueue);
				}
				else{	// pageQueue 빔
					enfirstQueue(&pageQueue,&pte[i]);
					pageQueue.len++;
				}
			}
			else{	 // 연결노드 있음
				procTable[i].numIHTNonNULLAcess++;
				// HT에 연결된 노드 돌면서 Hit, Fault검사
				tmp=&hashTable[firsthash];
				for(j=0;j<hashTable[firsthash].len+1;j++){
					procTable[i].numIHTConflictAccess++;
					if(tmp->procid==hash[i].procid&&tmp->index==hash[i].index){ //	HIT
						procTable[i].numPageHit++;
						faultcheck=0;
						tmp1=&pageQueue;
						for(k=0;k<pageQueue.len+1;k++){
							if(tmp1->procid==pte[i].procid&&tmp1->index==pte[i].index){
								replaceLRU(&pageQueue,tmp1);
								break;
							}
							tmp1=tmp1->next;
						}
						break;
					}
					tmp=tmp->next;
				}
				procTable[i].numIHTConflictAccess--;
				if(faultcheck==1){	//	Fault
					procTable[i].numPageFault++;
					enfirstQueue(&hashTable[firsthash], &hash[i]);
					hashTable[firsthash].len++;
					if(pageQueue.len==nFrame){ //	pageQueue 다참
						findhash=lastQueue(&pageQueue)->hashindex;
						findprocid=lastQueue(&pageQueue)->procid;
						findindex=lastQueue(&pageQueue)->index;
						// hashtable에 찾으러 감
						tmp=&hashTable[findhash];
						for(j=0;j<hashTable[findhash].len+1;j++){
							if(tmp->procid==findprocid&&tmp->index==findindex){
								deQueue(&hashTable[findhash],tmp);
								hashTable[findhash].len--;
								break;
							}
							tmp=tmp->next;
						}
						enfirstQueue(&pageQueue,&pte[i]);
						delastQueue(&pageQueue);
					}
					else{	// pageQueue 빔
						enfirstQueue(&pageQueue,&pte[i]);
						pageQueue.len++;
					}
				}
			}
			faultcheck=1;
		}
	}

	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Inverted Hash Table Access Conflicts %d\n",i,procTable[i].numIHTConflictAccess);
		printf("Proc %d Num of Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNULLAccess);
		printf("Proc %d Num of Non-Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNonNULLAcess);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
		assert(procTable[i].numIHTNULLAccess + procTable[i].numIHTNonNULLAcess == procTable[i].ntraces);
	}
}

int main(int argc, char *argv[]) {
	int i,c, simType;
	
	simType = atoi(argv[1]);
	firstLevelBits = atoi(argv[2]);
	phyMemSizeBits = atoi(argv[3]);
	for(i=0;i<phyMemSizeBits-12;i++){
		nFrame *= 2;
	}
	numProcess = argc-4;
	
	procTable = (struct procEntry *) malloc(sizeof(struct procEntry) * numProcess);
	pageQueue.next = pageQueue.prev = &pageQueue;
	pageQueue.len = 0;
	pageQueue.procid=-1;
	pageQueue.index=-1;

		//	insert traceFile
	for(i=0;i<numProcess;i++){
		int j;
		procTable[i].traceName=argv[i+4];
		procTable[i].tracefp = fopen(argv[i+4],"r");
	}

	// initialize procTable for Memory Simulations
	initProcTable();
	for(i = 0; i < numProcess; i++) {
		// opening a tracefile for the process
		printf("process %d opening %s\n",i,argv[i + 4]);
		procTable[i].tracefp = fopen(argv[i + 4],"r");
		if (procTable[i].tracefp == NULL) {
			printf("ERROR: can't open %s file; exiting...",argv[i+4]);
			exit(1);
		}
	}

	printf("Num of Frames %d Physical Memory Size %ld bytes\n",nFrame, (1L<<phyMemSizeBits));
	
	if (simType == 0) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(simType);
	}
	
	if (simType == 1) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(simType);
	}
	
	if (simType == 2) {
		printf("=============================================================\n");
		printf("The Two-Level Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		twoLevelVMSim(firstLevelBits);
	}
	
	if (simType == 3) {
		printf("=============================================================\n");
		printf("The Inverted Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		invertedPageVMSim();
	}

	return(0);
}
