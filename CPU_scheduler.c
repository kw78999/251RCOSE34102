#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // usleep

#define MAX_PROCESSES 10
#define TICK_MS 100
#define TICKS_PER_SEC (1000 / TICK_MS)

int current_time = 0;
int context_switch_time = 1; // in ticks
int context_switch_timer = 0;

// 프로세스 구조체
typedef struct Process {
    int pid;
    int arrival_time;
    int burst_time;
    int executed_time;
    int remaining_time;
    int io_burst_time;
    int io_request_time;
    int priority;

    int start_time;
    int finish_time;
    int waiting_time;
    int turnaround_time;
    int response_time;

    int has_started;
    int is_completed;
    int is_waiting_io;
} Process;

// Waiting Queue에 존재하는 프로세스
typedef struct WaitingEntry {
    Process *p;
    int remaining_io_time;
} WaitingEntry;

Process* ready_queue[MAX_PROCESSES];
int ready_count = 0;

WaitingEntry* waiting_queue[MAX_PROCESSES];
int waiting_count = 0;

Process* finished_queue[MAX_PROCESSES];
int finished_count = 0;

Process* cpu_running = NULL;
int in_context_switch = 0;

Process* Create_Process(int pid, int current_time) {
    Process* p = (Process*)malloc(sizeof(Process));
    p->pid = pid;
    p->arrival_time = current_time;
    p->burst_time = 1 + rand() % 10;
    p->remaining_time = p->burst_time * TICKS_PER_SEC;
    p->io_burst_time = (rand() % 5) * TICKS_PER_SEC;
    p->io_request_time = (rand() % p->burst_time) * TICKS_PER_SEC;
    p->priority = rand() % 5;

    p->start_time = -1;
    p->finish_time = -1;
    p->waiting_time = 0;
    p->turnaround_time = 0;
    p->response_time = -1;
    p->has_started = 0;
    p->is_completed = 0;
    p->is_waiting_io = 0;
    p->executed_time = 0;

    return p;
}

void print_status() {
    printf("\n\n\n[TIME %.1f sec]\n", current_time / (float)TICKS_PER_SEC);
    if (in_context_switch) {
        printf("CPU: CONTEXT SWITCHING (%.1f sec left)\n", context_switch_timer / (float)TICKS_PER_SEC);
    } else if (cpu_running) {
        printf("CPU: PID %d (rem: %.1f sec)\n",
            cpu_running->pid,
            cpu_running->remaining_time / (float)TICKS_PER_SEC);
    } else {
        printf("CPU: IDLE\n");
    }

    printf("Ready Queue\n");
    printf("PID | Arrival | CPU Burst | IO Req Time | IO Burst | Priority\n");
    printf("---------------------------------------------------------------\n");
    for (int i = 0; i < ready_count; i++) {
        printf("%3d | %7.1f | %9.1f | %11.1f | %8.1f | %8d\n",
            ready_queue[i]->pid,
            ready_queue[i]->arrival_time * 1.0,
            ready_queue[i]->burst_time * 1.0,
            ready_queue[i]->io_request_time / (float)TICKS_PER_SEC,
            ready_queue[i]->io_burst_time / (float)TICKS_PER_SEC,
            ready_queue[i]->priority);
    }
    printf("\n");

    printf("Waiting Queue\n");
    printf("PID | remaining_io_time (sec) |\n");
    printf("-------------------------------\n");
    for (int i = 0; i < waiting_count; i++) {
        printf("%3d | %23.1f\n",
            waiting_queue[i]->p->pid,
            waiting_queue[i]->remaining_io_time / (float)TICKS_PER_SEC);
    }
    printf("\n");

    printf("Finished Queue\n");
    printf("PID | Arrival | CPU Burst | IO Req Time | IO Burst | Priority\n");
    printf("---------------------------------------------------------------\n");
    for (int i = 0; i < finished_count; i++) {
        printf("%3d | %7.1f | %9.1f | %11.1f | %8.1f | %8d\n",
            finished_queue[i]->pid,
            finished_queue[i]->arrival_time * 1.0,
            finished_queue[i]->burst_time * 1.0,
            finished_queue[i]->io_request_time / (float)TICKS_PER_SEC,
            finished_queue[i]->io_burst_time / (float)TICKS_PER_SEC,
            finished_queue[i]->priority);
    }
}

void update_waiting_queue() {
    for (int i = 0; i < waiting_count; i++) {
        waiting_queue[i]->remaining_io_time--;
        if (waiting_queue[i]->remaining_io_time <= 0) {
            ready_queue[ready_count++] = waiting_queue[i]->p;
            free(waiting_queue[i]);
            for (int j = i; j < waiting_count - 1; j++) {
                waiting_queue[j] = waiting_queue[j + 1];
            }
            waiting_count--;
            i--;
        }
    }
}

void schedule_fcfs() {
    if (!cpu_running && ready_count > 0 && !in_context_switch) {
        cpu_running = ready_queue[0];
        for (int i = 0; i < ready_count - 1; i++) {
            ready_queue[i] = ready_queue[i + 1];
        }
        ready_count--;
    }
}

void update_cpu() {
    if (in_context_switch) {
        context_switch_timer--;
        if (context_switch_timer <= 0) in_context_switch = 0;
        return;
    }
    if (!cpu_running) return;

    cpu_running->executed_time++;
    cpu_running->remaining_time--;

    if (cpu_running->executed_time == cpu_running->io_request_time && cpu_running->io_burst_time > 0) {
        WaitingEntry* we = (WaitingEntry*)malloc(sizeof(WaitingEntry));
        we->p = cpu_running;
        we->remaining_io_time = cpu_running->io_burst_time;
        waiting_queue[waiting_count++] = we;
        cpu_running = NULL;
        in_context_switch = 1;
        context_switch_timer = context_switch_time;
        return;
    }

    if (cpu_running->remaining_time <= 0) {
        cpu_running->is_completed = 1;
        finished_queue[finished_count++] = cpu_running;
        cpu_running = NULL;
        in_context_switch = 1;
        context_switch_timer = context_switch_time;
    }
}

void maybe_create_process(int* pid_seq) {
    if (*pid_seq <= MAX_PROCESSES && (rand() % 100) < 30) {
        Process* new_p = Create_Process((*pid_seq)++, current_time / TICKS_PER_SEC);
        ready_queue[ready_count++] = new_p;
        printf("[+] Created P%d\n", new_p->pid);
    }
}

int main() {
    srand(time(NULL));
    int pid_seq = 1;

    while (1) {
        usleep(TICK_MS * 1000);  // 0.1초 대기

        if (current_time % TICKS_PER_SEC == 0) {
            print_status();
            maybe_create_process(&pid_seq);
        }
        schedule_fcfs();
        update_cpu();
        update_waiting_queue();

        if (pid_seq > MAX_PROCESSES && ready_count == 0 && waiting_count == 0 && cpu_running == NULL && !in_context_switch) {
            print_status();
            printf("[Simulation Finished]\n");
            break;
        }
        current_time++;
    }

    for (int i = 0; i < finished_count; i++) {
        free(finished_queue[i]);
    }
    return 0;
}
