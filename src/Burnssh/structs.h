
#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "../input_manager/manager.h"

#define MAX_ACTIVE_PROCESSES 10

typedef struct ProcessNode {
    pid_t pid;                // ID del proceso
    char name[BUFFER_SIZE];   // Nombre del ejecutable
    struct timeval start;     // Momento en que inició
    struct timeval end;
    int running;              // 1 si está corriendo, 0 si terminó
    int paused;               // 1 si está pausado, 0 si no
    int exit_code;            // Código de salida (-1 si sigue corriendo)
    int signal_value;         // Señal recibida (-1 si no recibió ninguna)
    struct ProcessNode* next; // Puntero al siguiente nodo
    pid_t watcher_pid;        // watcher para ejecutar en <time_max>
    int *paused_flag;
} ProcessInfo;


typedef struct {
    ProcessInfo* head; // Primer elemento
    int count;         // Número total de procesos históricos
} HistoryList;


typedef struct {
    ProcessInfo processes[MAX_ACTIVE_PROCESSES];
    int count;
} ActiveProcesses;