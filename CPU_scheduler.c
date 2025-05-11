#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PROCESSES 10

// 프로세스 정의
typedef struct Process {
    int pid;                // Process ID
    int arrival_time;       // 도착 시간
    int burst_time;         // CPU 버스트 시간
    int remaining_time;     // 남은 CPU 시간 (선점형에서 사용)
    int io_burst_time;      // I/O에 소요되는 버스트 시간
    int io_request_time;    // I/O가 요청되는 시점 (CPU 시간 기준)
    int priority;           // 우선순위 (작을수록 높은 우선순위)
    
    // 상태 추적용
    int start_time;         // 처음 실행된 시간
    int finish_time;        // 종료 시간
    int waiting_time;       // 대기 시간
    int turnaround_time;    // 반환 시간
    int response_time;      // 응답 시간

    int has_started;        // 첫 실행 여부 (response time 계산용)
    int is_completed;       // 완료 여부
    int is_waiting_io;      // I/O 대기 상태 여부
} Process;

// 프로세스 하나를 랜덤 생성
Process Create_Process(int pid) {
    Process p;
    p.pid = pid;
    p.arrival_time = rand() % 10;       // 0~9
    p.burst_time = 1 + rand() % 10;      // 1~10
    p.remaining_time = p.burst_time;
    p.io_burst_time = rand() % 5;        // 0~4
    p.io_request_time = rand() % p.burst_time; // 0~(burst_time-1)
    p.priority = rand() % 5;             // 0~4

    p.start_time = -1;
    p.finish_time = -1;
    p.waiting_time = 0;
    p.turnaround_time = 0;
    p.response_time = -1;
    p.has_started = 0;
    p.is_completed = 0;
    p.is_waiting_io = 0;

    return p;
}


int main() {
    srand(time(NULL));  // 랜덤 초기화

    int num_processes = 10;
    Process processes[num_processes];

    // 랜덤하게 프로세스 생성
    for (int i = 0; i < num_processes; i++) {
        processes[i] = Create_Process(i + 1);  // pid는 1부터 시작
    }

    // 상태 추력
    printf("PID | Arrival | CPU Burst | IO Req Time | IO Burst | Priority\n");
    printf("---------------------------------------------------------------\n");

    for (int i = 0; i < num_processes; i++) {
        printf("%3d | %7d | %9d | %11d | %8d | %8d\n",
            processes[i].pid,
            processes[i].arrival_time,
            processes[i].burst_time,
            processes[i].io_request_time,
            processes[i].io_burst_time,
            processes[i].priority);
    }

    return 0;
}

