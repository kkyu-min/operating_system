# operating_system

####hw2

이번 프로그래밍 과제 수업 시간에 배운 프로세스의 상태전환(Process State Transitions)과 CPU Scheduling에서 여러 가지 Scheduling 기법을 구현하여 시뮬레이션 해 보는 것입니다. 
실 제 process 가 수행하는 것을 정확히 시뮬레이션 하는 것은 여러 가지 어려움이 있기에 CPU Scheduling을 간단한 프로세스 수행 모델 (Process Execution Model) 하에서 구현 합니다.



####hw3

이번 프로그래밍 과제 수업 시간에 배운 Virtual Memory Systems에서 one-level, two-level Page Table 과 Inverted Page Table system을 구현하여 시뮬레이션 해 보는 것입니다. 
제공 되는 mtraces 디렉토리에 실제 프로그램 수행 중 접근한 메모리의 주소(Virtual address)를 순 차적으로 모아 놓은 memory trace가 있습니다. 
각각의 trace file에 들어 있는 memory trace 포맷은 다음과 같습니다.
0041f7a0 R 
13f5e2c0 R 
05e78900 R 
004758a0 R 
31348900 W

앞의 8문자는 접근된 메모리의 주소를 16진수로 나타낸 것이고(32bits) 그 뒤의 R 또는 W 해당 메모리 주소에 Read를 하는지 Write를 하는지 각각을 나타냅니다.
이 trace는 다음 코드 (fscanf())를 사용하여 읽어 들이면 됩니다. (본 과제에서 R/W 는 중요하지 않습니다.)


unsigned addr;
char rw;
...
fscanf(file,"%x %c",&addr,&rw);


Virtual Memory System simulator에서 virtual address 크기는 32bits (4Gbytes)로 나타나고
page의 사이즈는 12bits (4Kbytes)로 가정 합니다.
