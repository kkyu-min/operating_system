// 2021
// 운영체제
// CPU Schedule Simulator Homework(HW2)
// Student Number : B711124
// Name :	이규민
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>

#define SEED 10

// process states
#define S_IDLE 0			
#define S_READY 1
#define S_BLOCKED 2
#define S_RUNNING 3
#define S_TERMINATE 4

int NPROC, NIOREQ, QUANTUM;

struct ioDoneEvent {
	int procid;
	int doneTime;	//	IO 요청이 끝난 시간
	int len;
	struct ioDoneEvent *prev;
	struct ioDoneEvent *next;
} *ioDoneEvent;

struct ioDoneEvent ioDoneEventQueue;

struct ioDoneEvent *lastioQueue(struct ioDoneEvent *queue){
  struct ioDoneEvent *last;
  last = queue;
  while(last->next != last){
    last = last->next;
  }
  return last;
}

void enioQueue(struct ioDoneEvent *queue, struct ioDoneEvent *iodone){
  int i;
  struct ioDoneEvent *tmp;
  tmp = queue;
	int length = queue->len;
  for(i=0;i<length;i++){
    if(iodone->doneTime < tmp->next->doneTime){
      iodone->next = tmp->next;
			tmp->next = iodone;
			iodone->next->prev = iodone;
			iodone->prev = tmp;
      queue->len++;
      break;
    }
    tmp=tmp->next;
  }
  if(iodone->next == NULL && iodone->prev == NULL){
		iodone->next = iodone;
    iodone->prev = lastioQueue(queue);  
		lastioQueue(queue)->next = iodone; 
    queue->len++;
  }
}

void deioQueue(struct ioDoneEvent *queue, struct ioDoneEvent *iodone){
  //  iodone는 해당 queue에서 빼야될것
  //  빠진 iodone의 next, prev는 모두 NULL로 두기
  if(iodone->next == iodone){
    iodone->prev->next = iodone->prev;
    iodone->next == NULL;
    iodone->prev == NULL;
  }
  else{
		iodone->next->prev = iodone->prev;
    iodone->prev->next = iodone->next;
    iodone->next = NULL;
    iodone->prev = NULL;
  }
  queue->len--;

	if(queue->len == 0){
		queue->next = queue;
		queue->prev = queue;
	}
}

struct process {
	int id;
	int len;		// for queue
	int targetServiceTime;	// 프로세스의 총 수행시간, 시뮬레이션 시작 전에 저장됨
	int serviceTime;	// 프로세스가 cpu에 올라간 시간
	int startTime;
	int endTime;
	char state;
	int priority;
	int saveReg0, saveReg1;	// 프로세스의 상태가 S_RUNNGING가 되면 값을 복원? 매 사이클 compute()를 호출하여 연산함.
	struct process *prev;
	struct process *next;
} *procTable;	// 프로세스의 상태 저장, 생성되는 순서대로 배열에 저장됨,  

struct process idleProc;
struct process readyQueue;
struct process blockedQueue;
struct process *runningProc;

struct process* lastQueue(struct process *queue){
  struct process *last;
  last = queue;
  while(last->next != last){
    last = last->next;
  }
  return last;
}

void enQueue(struct process *queue, struct process *proc){
  proc->next = proc;
  proc->prev = lastQueue(queue);
  lastQueue(queue)->next = proc;
  queue->len++;
}

void deQueue(struct process *queue, struct process *proc){
  //  proc은 해당 큐에서 뺴야될 것
  //  큐의 마지막을 뺴는 경우
  //  큐에서 빠진 것은 next, prev 값 모두 null로 두기
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
  queue->len--;
}

int cpuReg0, cpuReg1;
int currentTime = 0;
int *procIntArrTime, *procServTime, *ioReqIntArrTime, *ioServTime;
/* 프로세스 만들어지는 시간 -> 시뮬레이션이 시작되고 [0]만쿰 시간이 흐르고 첫 번째 프로세스 생성, 두 번째는 첫 번째 프로세스 생성 후 [1]만큼 흐른후 생성 */
/* 실제 프로세스총 수행시간 */
/* IO요청은 cpu 사용 시간을 기준으로 생성 -> 첫 번째 IO 요청은 cpu 사용이 시작되고 [0]에 기록된 시간만큼 지난 후 생성 */
/* 실제 IO처리 시간 */

void compute() {
	// DO NOT CHANGE THIS FUNCTION
	cpuReg0 = cpuReg0 + runningProc->id;
	cpuReg1 = cpuReg1 + runningProc->id;
	//printf("In computer proc %d cpuReg0 %d\n",runningProc->id,cpuReg0);
}

void initProcTable() {
	int i;
	for(i=0; i < NPROC; i++) {
		procTable[i].id = i;
		procTable[i].len = 0;
		procTable[i].targetServiceTime = procServTime[i];
		procTable[i].serviceTime = 0;
		procTable[i].startTime = 0;
		procTable[i].endTime = 0;
		procTable[i].state = S_IDLE;
		procTable[i].priority = 0;
		procTable[i].saveReg0 = 0;
		procTable[i].saveReg1 = 0;
		procTable[i].prev = NULL;
		procTable[i].next = NULL;
	}
}

void procExecSim(struct process *(*scheduler)()) {
	int pid, qTime=0, cpuUseTime = 0, nproc=0, termProc = 0, nioreq=0;
	char schedule = 0, nextState = S_IDLE;
	int nextForkTime, nextIOReqTime;
	int count=0;
	int i;
  
	nextForkTime = procIntArrTime[nproc];
	nextIOReqTime = ioReqIntArrTime[nioreq];

	runningProc =&idleProc;

	while(count<NPROC) {
		currentTime++;
		qTime++;
		runningProc->serviceTime++;
		if (runningProc != &idleProc ) cpuUseTime++;
		
		// MUST CALL compute() Inside While loop
		compute();

		if (currentTime == nextForkTime) { /* CASE 2 : a new process created */
			/*  ready queue에 들어감, 현재 프로세스 running중에 새로운 프로세스 생성시 ready queue에 새로운 프로세스 넣고 running 끝나면 새로운 프로세스 뒤로 들어감 */
      /* 생성 시간에 맞게 ready queue에 들어감 
      상태 ready로 바꾸고 넣기 , nproc 증가*/
			procTable[nproc].state = S_READY;
			procTable[nproc].startTime = currentTime;
			enQueue(&readyQueue, &procTable[nproc]);
			nproc++;
			nextForkTime += procIntArrTime[nproc];
			schedule=2;
		}
		if (qTime == QUANTUM ) { /* CASE 1 : The quantum expires */
	    /* 현재 running중인 프로세스는 ready_queue로 들어감 */
			if(runningProc != &idleProc){
				runningProc->state = S_READY;
				runningProc->priority--;
			}
			schedule = 1;
		}
		while (ioDoneEventQueue.next->doneTime == currentTime) { /* CASE 3 : IO Done Event */
			/*  cpu 사용시간에 상관없이 처리시간이 지나면 IO Done 발생 blocked에서 ready로 감*/
      // io끝난 프로세스 찾아서 ready queue에 넣고 완료된 io는 ioDoneEventQueue에서 빼기
			struct process *tmp;
			tmp = &blockedQueue;
			int num = blockedQueue.len;

			for(i=0;i<num;i++){
				if(ioDoneEventQueue.next->procid == tmp->next->id){
					if(tmp->next->state == S_TERMINATE){
						deioQueue(&ioDoneEventQueue, ioDoneEventQueue.next);
						deQueue(&blockedQueue, tmp->next);
						schedule=3;
            //tmp=&blockedQueue;
						break;
					}
					else{
						deioQueue(&ioDoneEventQueue, ioDoneEventQueue.next);
						tmp->next->state=S_READY;
						struct process * tmp1;
						tmp1 = tmp->next;
						deQueue(&blockedQueue, tmp->next);
						enQueue(&readyQueue, tmp1);
						schedule=3;
            //tmp=&blockedQueue;
						break;
					}
				}
				tmp=tmp->next;
			}
		}
		if (cpuUseTime == nextIOReqTime) { /* CASE 5: reqest IO operations (only when the process does not terminate) */
			/* running 프로세스가 있는 상태에서만 IO요청 발생 가능.  */
			runningProc->state = S_BLOCKED;
			if(qTime < QUANTUM){
				runningProc->priority++;
			}
			ioDoneEvent[nioreq].procid = runningProc->id;
			ioDoneEvent[nioreq].doneTime = currentTime + ioServTime[nioreq];
			ioDoneEvent[nioreq].prev = NULL;
			ioDoneEvent[nioreq].next = NULL;
      if(runningProc != &idleProc){
			enioQueue(&ioDoneEventQueue, &ioDoneEvent[nioreq]);
      }// 왜 라스트에 접근 하는게 어렵지?
			nioreq++;
			nextIOReqTime += ioReqIntArrTime[nioreq];
			if(runningProc != &idleProc){
				enQueue(&blockedQueue, runningProc);
			}
			schedule=5;
		}
		if (runningProc->serviceTime == runningProc->targetServiceTime) { /* CASE 4 : the process job done and terminates */
			/* 프로세스 수행시간 다 끝나면 S_TERMINATE상태로 감. scheduler 호출 */
			runningProc->state = S_TERMINATE;
			runningProc->endTime = currentTime;
			count++;
			schedule=4;
		}

		if(schedule==1 || schedule==2 || schedule==3){
			if(runningProc != &idleProc){
				enQueue(&readyQueue, runningProc);
			}
			runningProc->saveReg0 = cpuReg0;
			runningProc->saveReg1 = cpuReg1;
			runningProc = (*scheduler)();
			//runningProc = scheduler();
			qTime=0;
		}
		else if(schedule==4 || schedule==5){
			runningProc->saveReg0 = cpuReg0;
			runningProc->saveReg1 = cpuReg1;
			runningProc = (*scheduler)();
			//runningProc = scheduler();
			qTime=0;
		}


		schedule=0;
		// call scheduler() if needed
		// scheduler() 호출 할때마다 퀀텀 초기화!
    // process terminate 될때마다 count 하나씩 증가 다 되면 종료
		
	} // while loop
}

//RR,SJF(Modified),SRTN,Guaranteed Scheduling(modified),Simple Feed Back Scheduling
//readyQueue에서 뭘 러닝에 넣을지 순서 정하기
struct process* RRschedule() {
	/* ready_queue에 들어오는 순서대로 할당 받음, 퀀텀 시간만큼 할당 받음  */
	if(readyQueue.len == 0){
		return &idleProc;
	}
	else {
		struct process *tmp;
		tmp = readyQueue.next;
		// if(readyQueue.next->startTime == 0){
		// 	readyQueue.next->startTime = currentTime;
		// }
		readyQueue.next->state = S_RUNNING;
		deQueue(&readyQueue, readyQueue.next);
		cpuReg0 = tmp->saveReg0;
		cpuReg1 = tmp->saveReg1;
		return tmp;
	}
}
struct process* SJFschedule() {
	/* 퀀텀 시간 만큼 할당, 프로세스의 총 수행시간(targetServiceTime)이 적은 프로세스부터 할당*/
	int i;
	if(readyQueue.len == 0){
		return &idleProc;
	}
	else if(readyQueue.len == 1){
		struct process *answer;
		answer = readyQueue.next;
		deQueue(&readyQueue, readyQueue.next);
		cpuReg0 = answer->saveReg0;
		cpuReg1 = answer->saveReg1;
		return answer;
	}
	else {
		int i;
		struct process *answer;
		answer = readyQueue.next;
		int comp = readyQueue.next->targetServiceTime;
		struct process *tmp;
		tmp = readyQueue.next;
		int n = readyQueue.len;

		for(i=0;i<n;i++){
			if(tmp->next->targetServiceTime<comp){
				comp=tmp->next->targetServiceTime;
				answer=tmp->next;
			}
			tmp=tmp->next;
		}
		deQueue(&readyQueue, answer);
		cpuReg0 = answer->saveReg0;
		cpuReg1 = answer->saveReg1;
		return answer;
	}
	}

struct process* SRTNschedule() {
	/* 현재 남아있는 수행 시간이 가장 적은 프로세스부터 할당 (targetServiceTime - serviceTime) */
	int i;
	if(readyQueue.len == 0){
		return &idleProc;
	}
	else if(readyQueue.len == 1){
		struct process *answer;
		answer = readyQueue.next;
		deQueue(&readyQueue, readyQueue.next);
		cpuReg0 = answer->saveReg0;
		cpuReg1 = answer->saveReg1;
		return answer;
	}
	else {
		int i;
		struct process *answer;
		answer = readyQueue.next;
		int comp = (readyQueue.next->targetServiceTime - readyQueue.next->serviceTime);
		struct process *tmp;
		tmp = readyQueue.next;
		int n = readyQueue.len;

		for(i=0;i<n;i++){
			if((tmp->next->targetServiceTime - tmp->next->serviceTime)<comp){
				comp=(tmp->next->targetServiceTime - tmp->next->serviceTime);
				answer=tmp->next;
			}
			tmp=tmp->next;
		}
		deQueue(&readyQueue, answer);
		cpuReg0 = answer->saveReg0;
		cpuReg1 = answer->saveReg1;
		return answer;
	}
}
struct process* GSschedule() {
	/* ready queue중 serviceTime/targetServiceTime 의 ratio가  가장 작은 프로세스부터 할당 */
	int i;
	if(readyQueue.len == 0){
		return &idleProc;
	}
	else if(readyQueue.len == 1){
		struct process *answer;
		answer = readyQueue.next;
		deQueue(&readyQueue, readyQueue.next);
		cpuReg0 = answer->saveReg0;
		cpuReg1 = answer->saveReg1;
		return answer;
	}
	else {
		int i;
		struct process *answer;
		answer = readyQueue.next;
		float comp = ((float)readyQueue.next->serviceTime / (float)readyQueue.next->targetServiceTime);
		struct process *tmp;
		tmp = readyQueue.next;
		int n = readyQueue.len;

		for(i=0;i<n;i++){
			if(((float)tmp->next->serviceTime / (float)tmp->next->targetServiceTime)<comp){
				comp=((float)tmp->next->serviceTime / (float)tmp->next->targetServiceTime);
				answer=tmp->next;
			}
			tmp=tmp->next;
		}
		deQueue(&readyQueue, answer);
		cpuReg0 = answer->saveReg0;
		cpuReg1 = answer->saveReg1;
		return answer;
	}
}

struct process* SFSschedule() {
	/* 할당된 퀀텀을 다 사용하지 못하고 IO 요청을 받은 프로세스(IO bound 프로세스)의 우선순위 증가, 할당된 퀀텀 다 사용하고 바뀌는 프로세스(cpu bound 프로세스)의 우선순위 감소,
	ready_queue중 우선순위가 가장 높은 프로세스부터  cpu에 할당 */
	int i;
	if(readyQueue.len == 0){
		return &idleProc;
	}
	else if(readyQueue.len == 1){
		struct process *answer;
		answer = readyQueue.next;
		deQueue(&readyQueue, readyQueue.next);
		cpuReg0 = answer->saveReg0;
		cpuReg1 = answer->saveReg1;
		return answer;
	}
	else {
		int i;
		struct process *answer;
		answer = readyQueue.next;
		int comp = readyQueue.next->priority;
		struct process *tmp;
		tmp = readyQueue.next;
		int n = readyQueue.len;

		for(i=0;i<n;i++){
			if(tmp->next->priority>comp){
				comp=tmp->next->priority;
				answer=tmp->next;
			}
			tmp=tmp->next;
		}
		deQueue(&readyQueue, answer);
		cpuReg0 = answer->saveReg0;
		cpuReg1 = answer->saveReg1;
		return answer;
	}
}
/* 선정 방식 동일하면 FCFS 방식으로 선택 */

void printResult() {
	// DO NOT CHANGE THIS FUNCTION
	int i;
	long totalProcIntArrTime=0,totalProcServTime=0,totalIOReqIntArrTime=0,totalIOServTime=0;
	long totalWallTime=0, totalRegValue=0;
	for(i=0; i < NPROC; i++) {
		totalWallTime += procTable[i].endTime - procTable[i].startTime;
		/*
		printf("proc %d serviceTime %d targetServiceTime %d saveReg0 %d\n",
			i,procTable[i].serviceTime,procTable[i].targetServiceTime, procTable[i].saveReg0);
		*/
		totalRegValue += procTable[i].saveReg0+procTable[i].saveReg1;
		/* printf("reg0 %d reg1 %d totalRegValue %d\n",procTable[i].saveReg0,procTable[i].saveReg1,totalRegValue);*/
	}
	for(i = 0; i < NPROC; i++ ) { 
		totalProcIntArrTime += procIntArrTime[i];
		totalProcServTime += procServTime[i];
	}
	for(i = 0; i < NIOREQ; i++ ) { 
		totalIOReqIntArrTime += ioReqIntArrTime[i];
		totalIOServTime += ioServTime[i];
	}
	
	printf("Avg Proc Inter Arrival Time : %g \tAverage Proc Service Time : %g\n", (float) totalProcIntArrTime/NPROC, (float) totalProcServTime/NPROC);
	printf("Avg IOReq Inter Arrival Time : %g \tAverage IOReq Service Time : %g\n", (float) totalIOReqIntArrTime/NIOREQ, (float) totalIOServTime/NIOREQ);
	printf("%d Process processed with %d IO requests\n", NPROC,NIOREQ);
	printf("Average Wall Clock Service Time : %g \tAverage Two Register Sum Value %g\n", (float) totalWallTime/NPROC, (float) totalRegValue/NPROC);
	
}

int main(int argc, char *argv[]) {
	// DO NOT CHANGE THIS FUNCTION
	int i;
	int totalProcServTime = 0, ioReqAvgIntArrTime;
	int SCHEDULING_METHOD, MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME, MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME;
	
	if (argc < 12) {
		printf("%s: SCHEDULING_METHOD NPROC NIOREQ QUANTUM MIN_INT_ARRTIME MAX_INT_ARRTIME MIN_SERVTIME MAX_SERVTIME MIN_IO_SERVTIME MAX_IO_SERVTIME MIN_IOREQ_INT_ARRTIME\n",argv[0]);
		exit(1);
	}
	
	SCHEDULING_METHOD = atoi(argv[1]);
	NPROC = atoi(argv[2]);
	NIOREQ = atoi(argv[3]);
	QUANTUM = atoi(argv[4]);
	MIN_INT_ARRTIME = atoi(argv[5]);
	MAX_INT_ARRTIME = atoi(argv[6]);
	MIN_SERVTIME = atoi(argv[7]);
	MAX_SERVTIME = atoi(argv[8]);
	MIN_IO_SERVTIME = atoi(argv[9]);
	MAX_IO_SERVTIME = atoi(argv[10]);
	MIN_IOREQ_INT_ARRTIME = atoi(argv[11]);
	
	printf("SIMULATION PARAMETERS : SCHEDULING_METHOD %d NPROC %d NIOREQ %d QUANTUM %d \n", SCHEDULING_METHOD, NPROC, NIOREQ, QUANTUM);
	printf("MIN_INT_ARRTIME %d MAX_INT_ARRTIME %d MIN_SERVTIME %d MAX_SERVTIME %d\n", MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME);
	printf("MIN_IO_SERVTIME %d MAX_IO_SERVTIME %d MIN_IOREQ_INT_ARRTIME %d\n", MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME);
	
	srandom(SEED);
	
	// allocate array structures
	procTable = (struct process *) malloc(sizeof(struct process) * NPROC);
	ioDoneEvent = (struct ioDoneEvent *) malloc(sizeof(struct ioDoneEvent) * NIOREQ);
	procIntArrTime = (int *) malloc(sizeof(int) * NPROC);
	procServTime = (int *) malloc(sizeof(int) * NPROC);
	ioReqIntArrTime = (int *) malloc(sizeof(int) * NIOREQ);
	ioServTime = (int *) malloc(sizeof(int) * NIOREQ);

	// initialize queues
	readyQueue.next = readyQueue.prev = &readyQueue;
	
	blockedQueue.next = blockedQueue.prev = &blockedQueue;
	ioDoneEventQueue.next = ioDoneEventQueue.prev = &ioDoneEventQueue;
	ioDoneEventQueue.doneTime = INT_MAX;
	ioDoneEventQueue.procid = -1;
	ioDoneEventQueue.len  = readyQueue.len = blockedQueue.len = 0;
	
	// generate process interarrival times
	for(i = 0; i < NPROC; i++ ) { 
		procIntArrTime[i] = random()%(MAX_INT_ARRTIME - MIN_INT_ARRTIME+1) + MIN_INT_ARRTIME;
	}
	
	// assign service time for each process
	for(i=0; i < NPROC; i++) {
		procServTime[i] = random()%(MAX_SERVTIME - MIN_SERVTIME + 1) + MIN_SERVTIME;
		totalProcServTime += procServTime[i];	
	}
	
	ioReqAvgIntArrTime = totalProcServTime/(NIOREQ+1);
	assert(ioReqAvgIntArrTime > MIN_IOREQ_INT_ARRTIME);
	
	// generate io request interarrival time
	for(i = 0; i < NIOREQ; i++ ) { 
		ioReqIntArrTime[i] = random()%((ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME)*2+1) + MIN_IOREQ_INT_ARRTIME;
	}
	
	// generate io request service time
	for(i = 0; i < NIOREQ; i++ ) { 
		ioServTime[i] = random()%(MAX_IO_SERVTIME - MIN_IO_SERVTIME+1) + MIN_IO_SERVTIME;
	}
	
#ifdef DEBUG
	// printing process interarrival time and service time
	printf("Process Interarrival Time :\n");
	for(i = 0; i < NPROC; i++ ) { 
		printf("%d ",procIntArrTime[i]);
	}
	printf("\n");
	printf("Process Target Service Time :\n");
	for(i = 0; i < NPROC; i++ ) { 
		printf("%d ",procTable[i].targetServiceTime);
	}
	printf("\n");
#endif
	
	// printing io request interarrival time and io request service time
	printf("IO Req Average InterArrival Time %d\n", ioReqAvgIntArrTime);
	printf("IO Req InterArrival Time range : %d ~ %d\n",MIN_IOREQ_INT_ARRTIME,
			(ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME)*2+ MIN_IOREQ_INT_ARRTIME);
			
#ifdef DEBUG		
	printf("IO Req Interarrival Time :\n");
	for(i = 0; i < NIOREQ; i++ ) { 
		printf("%d ",ioReqIntArrTime[i]);
	}
	printf("\n");
	printf("IO Req Service Time :\n");
	for(i = 0; i < NIOREQ; i++ ) { 
		printf("%d ",ioServTime[i]);
	}
	printf("\n");
#endif
	
	struct process* (*schFunc)();
	switch(SCHEDULING_METHOD) {
		case 1 : schFunc = RRschedule; break;
		case 2 : schFunc = SJFschedule; break;
		case 3 : schFunc = SRTNschedule; break;
		case 4 : schFunc = GSschedule; break;
		case 5 : schFunc = SFSschedule; break;
		default : printf("ERROR : Unknown Scheduling Method\n"); exit(1);
	}
	initProcTable();
	procExecSim(schFunc);
	printResult();
	
}