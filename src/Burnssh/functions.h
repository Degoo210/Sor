#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "structs.h"

extern volatile pid_t abort_watcher_pid;
extern pid_t abort_targets[MAX_ACTIVE_PROCESSES];
extern int abort_targets_count;

// Funciones para historial de procesos
void add_to_history_with_data(HistoryList* history, pid_t pid, const char* name, struct timeval start, struct timeval end, int exit_code, int signal_value);
void free_history(HistoryList* history);
void print_history(HistoryList* history);

// Funciones para procesos activos
int find_free_slot(ActiveProcesses* active);
void add_active_process(ActiveProcesses* active, pid_t pid, const char* name, int slot);
void remove_active_process(ActiveProcesses* active, int slot);

// Funciones cmd
void cmd_launcher(char **input, ActiveProcesses* active, HistoryList* history);
void cmd_status(ActiveProcesses* active, HistoryList* history);
void cmd_abort(char **input, ActiveProcesses* active);
void cmd_pause(char **input, ActiveProcesses* active);
void cmd_resume(char **input, ActiveProcesses* active);

//Funciones otras
double get_seconds(ProcessInfo *p);
int is_number(char *str);