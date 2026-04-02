#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>

#include "../input_manager/manager.h"
#include "structs.h"
#include "functions.h"

ActiveProcesses active_processes;
HistoryList history_list;

volatile pid_t abort_watcher_pid = 0;
pid_t abort_targets[MAX_ACTIVE_PROCESSES];
int abort_targets_count = 0;
int time_max = 0;

void sigchld_handler(int sig) {
    if (abort_watcher_pid != 0) {
        int status;
        pid_t result = waitpid(abort_watcher_pid, &status, WNOHANG);
        

        if (result == abort_watcher_pid) {
            abort_watcher_pid = 0;
            printf("\n\nAbort cumplido.\n\n");
            printf("%-5s %-8s %-16s %-10s %-8s %-10s %-12s\n", 
                "#", "PID", "Nombre", "Tiempo(s)", "Pausado", "Exit code", "Signal value");

            int count = 0; 
            for (int i = 0; i < abort_targets_count; i++) {
                for (int j = 0; j < MAX_ACTIVE_PROCESSES; j++) {
                    if (active_processes.processes[j].pid == abort_targets[i]) {
                        double time = get_seconds(&active_processes.processes[j]);
                        printf("%-5d %-8d %-16s %-10.1f %-8d %-10d %-12d\n",
                            ++count,
                            active_processes.processes[j].pid,
                            active_processes.processes[j].name,
                            time,
                            active_processes.processes[j].paused,
                            active_processes.processes[j].exit_code,
                            active_processes.processes[j].signal_value);
                            kill(abort_targets[i], SIGTERM);
                            break;
                    }
                }
            }   
    abort_targets_count = 0;
        }
    }

    for (int i = 0; i < MAX_ACTIVE_PROCESSES; i++) {
        if (active_processes.processes[i].pid == 0 || !active_processes.processes[i].running) {
            continue;
        }

        int status;
        pid_t result = waitpid(active_processes.processes[i].pid, &status, WNOHANG);

        if (result > 0) {
            gettimeofday(&active_processes.processes[i].end, NULL);
            active_processes.processes[i].running = 0;

            if (WIFEXITED(status)) {
                active_processes.processes[i].exit_code = WEXITSTATUS(status);
            }
            if (WIFSIGNALED(status)) {
                active_processes.processes[i].signal_value = WTERMSIG(status);
            }

            add_to_history_with_data(&history_list,
                                     active_processes.processes[i].pid,
                                     active_processes.processes[i].name,
                                     active_processes.processes[i].start,
                                     active_processes.processes[i].end,
                                     active_processes.processes[i].exit_code,
                                     active_processes.processes[i].signal_value);

            remove_active_process(&active_processes, i);
        }
    }
}


int main(int argc, char const *argv[]) {
    active_processes.count = 0;
    history_list.head = NULL;
    history_list.count = 0;

    for (int i = 0; i < MAX_ACTIVE_PROCESSES; i++) {
        active_processes.processes[i].pid = 0;
    }

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);


    if (argc > 1) {
        if (!is_number(argv[1])) {
            printf("\nArgumento inválido\n\n");
            return 1;
        }
    time_max = atoi(argv[1]);
}

    set_buffer();

    while (1) {
        printf("burnssh> ");
        fflush(stdout);
        char** input = read_user_input();

        if (input[0] == NULL || strlen(input[0]) == 0) {
            free_user_input(input);
            continue;
        }

        if (strcmp(input[0], "launch") == 0) {
            cmd_launcher(input, &active_processes, &history_list, time_max);
        }
        else if (strcmp(input[0], "status") == 0) {
            cmd_status(&active_processes, &history_list);
        }
        else if (strcmp(input[0], "abort") == 0) {
            cmd_abort(input, &active_processes);
        }
        else if (strcmp(input[0], "pause") == 0) {
            cmd_pause(input, &active_processes);
        }
        else if (strcmp(input[0], "resume") == 0) {
            cmd_resume(input, &active_processes);
        }
        else if (strcmp(input[0], "exit") == 0) {
            printf("Saliendo...\n");
            free_user_input(input);
            break;
        }
        else {
            printf("Comando %s no reconocido\n", input[0]);
        }

        free_user_input(input);
    }

    free_history(&history_list);

    return 0;
}