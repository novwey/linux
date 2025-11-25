#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>

#define NUM_PROCESS 10
#define TIME_QUANTUM 3  
#define SIM_SPEED 100000 

typedef enum { READY, RUNNING, SLEEP, DONE } State;

typedef struct {
    pid_t pid;
    int remain_quantum; 
    int io_wait_time;   
    State state;
    int waiting_time;   
} PCB;

PCB pcb_table[NUM_PROCESS];
int current_idx = -1;       
int active_process_cnt = NUM_PROCESS; 

void child_usr1_handler(int sig) {
}

void do_child_task(int id) {
    int cpu_burst = (rand() % 10) + 1; 
    signal(SIGUSR1, child_usr1_handler);

    while (1) {
        pause(); 
        cpu_burst--;
        
        if (cpu_burst <= 0) {
            if (rand() % 2 == 0) { 
                exit(0); 
            } else { 
                kill(getppid(), SIGUSR2);
                cpu_burst = (rand() % 10) + 1;
            }
        }
    }
}

void parent_io_handler(int sig) {
    if (current_idx != -1) {
        pcb_table[current_idx].state = SLEEP;
        pcb_table[current_idx].io_wait_time = (rand() % 5) + 1; 
        printf("[IO Req] Process %d requested I/O (Wait: %d ticks)\n", 
               pcb_table[current_idx].pid, pcb_table[current_idx].io_wait_time);
    }
}

void parent_exit_handler(int sig) {
    pid_t p;
    int status;
    while ((p = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < NUM_PROCESS; i++) {
            if (pcb_table[i].pid == p) {
                pcb_table[i].state = DONE;
                active_process_cnt--;
                printf("[DONE] Process %d Finished. (Avg Wait Time: %d)\n", p, pcb_table[i].waiting_time);
                break;
            }
        }
    }
}

void timer_handler(int sig) {
    if (active_process_cnt == 0) return; 

    for (int i = 0; i < NUM_PROCESS; i++) {
        if (pcb_table[i].state == SLEEP) {
            pcb_table[i].io_wait_time--;
            if (pcb_table[i].io_wait_time <= 0) {
                pcb_table[i].state = READY;
                printf("[IO Done] Process %d moved to READY queue\n", pcb_table[i].pid);
            }
        } else if (pcb_table[i].state == READY) {
            pcb_table[i].waiting_time++;
        }
    }

    int switch_needed = 0;
    if (current_idx != -1 && pcb_table[current_idx].state == RUNNING) {
        pcb_table[current_idx].remain_quantum--;
        if (pcb_table[current_idx].remain_quantum <= 0) {
            pcb_table[current_idx].state = READY;
            switch_needed = 1;
        }
    } else {
        switch_needed = 1; 
    }

    int available_quantum = 0;
    for (int i = 0; i < NUM_PROCESS; i++) {
        if (pcb_table[i].state != DONE && pcb_table[i].remain_quantum > 0) {
            available_quantum = 1;
            break;
        }
    }
    if (!available_quantum) {
        printf("[System] Reset All Time Quantums\n");
        for (int i = 0; i < NUM_PROCESS; i++) {
            if (pcb_table[i].state != DONE) 
                pcb_table[i].remain_quantum = TIME_QUANTUM;
        }
    }

    if (switch_needed) {
        int next_idx = -1;
        for (int i = 1; i <= NUM_PROCESS; i++) {
            int idx = (current_idx + i) % NUM_PROCESS;
            if (pcb_table[idx].state == READY && pcb_table[idx].remain_quantum > 0) {
                next_idx = idx;
                break;
            }
        }

        if (next_idx != -1) {
            current_idx = next_idx;
            pcb_table[current_idx].state = RUNNING;
            kill(pcb_table[current_idx].pid, SIGUSR1);
        } else {
            current_idx = -1;
        }
    } else {
        kill(pcb_table[current_idx].pid, SIGUSR1);
    }
}

int main() {
    srand(time(NULL));

    signal(SIGUSR2, parent_io_handler);
    signal(SIGCHLD, parent_exit_handler);
    signal(SIGALRM, timer_handler);

    printf("=== OS Simulation Start ===\n");

    for (int i = 0; i < NUM_PROCESS; i++) {
        pid_t pid = fork();
        if (pid == 0) { 
            do_child_task(i);
            exit(0); 
        } else { 
            pcb_table[i].pid = pid;
            pcb_table[i].remain_quantum = TIME_QUANTUM;
            pcb_table[i].state = READY;
            pcb_table[i].io_wait_time = 0;
            pcb_table[i].waiting_time = 0;
        }
    }

    ualarm(SIM_SPEED, SIM_SPEED);

    while (active_process_cnt > 0) {
        pause(); 
    }

    printf("\n=== Simulation Completed! ===\n");
    double total_wait = 0;
    for(int i=0; i<NUM_PROCESS; i++) total_wait += pcb_table[i].waiting_time;
    printf("Total Average Waiting Time: %.2f ticks\n", total_wait / NUM_PROCESS);

    return 0;
}
