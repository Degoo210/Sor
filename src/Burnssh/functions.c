#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <ctype.h>

#include "../input_manager/manager.h"
#include "structs.h"
#include "functions.h"

// FUNCIONES PARA EL HSITORIAL 

// Agregar proceso al historial (sin límite)
void add_to_history_with_data(HistoryList* history, pid_t pid, const char* name, 
                              struct timeval start, struct timeval end, 
                              int exit_code, int signal_value) {
    ProcessInfo* new_node = (ProcessInfo*)malloc(sizeof(ProcessInfo));
    if (!new_node) {
        perror("malloc");
        return;
    }
    
    new_node->pid = pid;
    strncpy(new_node->name, name, BUFFER_SIZE - 1);
    new_node->name[BUFFER_SIZE - 1] = '\0';
    new_node->start = start;
    new_node->end = end;
    new_node->running = 0;
    new_node->paused = 0;
    new_node->exit_code = exit_code;
    new_node->signal_value = signal_value;
    new_node->next = NULL;
    
    new_node->next = history->head;
    history->head = new_node;
    history->count++;
}

// Liberar todo el historial
void free_history(HistoryList* history) {
    ProcessInfo* current = history->head;
    while (current != NULL) {
        ProcessInfo* temp = current;
        current = current->next;
        free(temp);
    }
    history->head = NULL;
    history->count = 0;
}

// Mostrar historial completo
void print_history(HistoryList* history) {
    ProcessInfo* current = history->head;
    int count = 0;
    
    printf("\n\n=== PROCESOS INACTIVOS ===\n");
    printf("%-5s %-8s %-16s %-10s %-8s %-10s %-12s\n", 
           "#", "PID", "Nombre", "Tiempo(s)", "Pausado", "Exit code", "Signal value");
    
    while (current != NULL) {
        double time = get_seconds(current);
        printf("%-5d %-8d %-16s %-10.1f %-8d %-10d %-12d\n", 
               ++count, current->pid, current->name, time, 
               current->paused, current->exit_code, current->signal_value);
        current = current->next;
    }
    printf("\n\n");
}

// FUNCIONES PARA PROCESOS ACTIVOS

// Buscar un slot libre en procesos activos
int find_free_slot(ActiveProcesses* active) {
    if (active->count >= MAX_ACTIVE_PROCESSES) {
        return -1;
    }
    
    for (int i = 0; i < MAX_ACTIVE_PROCESSES; i++) {
        if (active->processes[i].pid == 0) {
            return i;
        }
    }
    return -1;
}

// Agregar proceso activo
void add_active_process(ActiveProcesses* active, pid_t pid, const char* name, int slot) {
    active->processes[slot].pid = pid;
    strncpy(active->processes[slot].name, name, BUFFER_SIZE - 1);
    gettimeofday(&active->processes[slot].start, NULL);
    active->processes[slot].running = 1;
    active->processes[slot].paused = 0;
    active->processes[slot].exit_code = -1;
    active->processes[slot].signal_value = -1;
    active->processes[slot].watcher_pid = 0;
    active->count++;
}

// Remover proceso activo
void remove_active_process(ActiveProcesses* active, int slot) {
    active->processes[slot].pid = 0;
    active->processes[slot].name[0] = '\0';
    active->count--;
}

// FUNCIONES EXISTENTES ACTUALIZADAS

// Consigue el tiempo que lleva corriendo un proceso
double get_seconds(ProcessInfo *p) {
    struct timeval reference;
    if (p->paused) {
        reference = p->end;
    } else if (p->running) {
        gettimeofday(&reference, NULL);
    } else {
        reference = p->end;
    }
    double time = (reference.tv_sec - p->start.tv_sec);
    time += (reference.tv_usec - p->start.tv_usec) / 1000000.0;
    return time;
}

int is_number(const char *str){
    for(int i = 0; str[i] != '\0'; i++){
        if (!isdigit(str[i])) return 0;
    }
    return 1;
}