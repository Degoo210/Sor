#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>


#include "../input_manager/manager.h"
#include "structs.h"
#include "functions.h"

void cmd_launcher(char **input, ActiveProcesses* active, HistoryList* history, int time_max) {
    if (input[1] == NULL || strlen(input[1]) == 0) {
        printf("No se ha indicado ningún ejecutable\n");
        return;
    }
    
    int slot = find_free_slot(active);
    if (slot == -1) {
        printf("Los %d slots activos están ocupados, no se pudo lanzar el proceso\n", MAX_ACTIVE_PROCESSES);
        return;
    }
    
    int argc = 0;
    while (input[argc + 1] != NULL && strlen(input[argc + 1]) > 0) {
        argc++;
    }
    
    char **args = calloc(argc + 1, sizeof(char *));
    for (int i = 0; i < argc; i++) {
        args[i] = input[i + 1];
    }
    args[argc] = NULL;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        free(args);
        return;
    }
    fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // HIJO
        close(pipefd[0]);
        execvp(args[0], args);
  
        write(pipefd[1], "e", 1);
        close(pipefd[1]);
        free(args);
        exit(1);
    } 
    else if (pid > 0) {
        // PADRE
        close(pipefd[1]);
        char buf[1];
        int n = read(pipefd[0], buf, 1);
        close(pipefd[0]);

        if (n > 0) {
            waitpid(pid, NULL, 0);
            printf("Ejecutable no encontrado o no se pudo lanzar\n");
            free(args);
            return;
        }

        add_active_process(active, pid, args[0], slot);

        if (time_max > 0) {
    int *paused_flag = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *paused_flag = 0;
    active->processes[slot].paused_flag = paused_flag;

    pid_t target_pid = pid;
    pid_t watcher = fork();
    if (watcher == 0) {
        int elapsed = 0;
        while (elapsed < time_max) {
            sleep(1);
            if (!(*paused_flag)) {
                elapsed++;
            }
        }
        if (kill(target_pid, 0) == 0) {
            printf("\nProceso %d superó el tiempo límite, enviando SIGTERM\n", target_pid);
            fflush(stdout);
            kill(target_pid, SIGTERM);
            sleep(5);
            if (kill(target_pid, 0) == 0) {
                printf("\nProceso %d no terminó, enviando SIGKILL\n", target_pid);
                fflush(stdout);
                kill(target_pid, SIGKILL);
            }
        }
        munmap(paused_flag, sizeof(int));
        exit(0);
    }
    active->processes[slot].watcher_pid = watcher;
        }

        printf("Proceso lanzado: PID=%d, ejecutable='%s' (Activos ahora: %d/%d)\n", 
               pid, args[0], active->count, MAX_ACTIVE_PROCESSES);
    }
    
    free(args);
}

void cmd_status(ActiveProcesses* active, HistoryList* history) {
    print_active(active);
    print_history(history);
}

void cmd_abort(char **input, ActiveProcesses* active) {
    if (abort_watcher_pid != 0) {
        printf("Ya hay un abort en progreso\n");
        return;
    }

    if (active->count == 0){
        printf("No hay procesos en ejecución. abort no se puede ejecutar.\n");
        return;
    }

    if (input[1] == NULL || strlen(input[1]) == 0) {
        printf("No se ha indicado ningun ejecutable\n");
        return;
    }

    if (!is_number(input[1])){
        printf("Por favor indica un tiempo válido\n");
        return;
    }

    int time = atoi(input[1]);

    if (time == 0) {
        printf("Por favor indica un tiempo mayor a 0\n");
        return;
    }

    abort_targets_count = 0;

    for (int i = 0; i < MAX_ACTIVE_PROCESSES; i++) {
        if (active->processes[i].pid != 0 && active->processes[i].running) {
            abort_targets[abort_targets_count++] = active->processes[i].pid;
        }
    }

    // Lanzar watcher
    pid_t watcher = fork();
    if (watcher == 0) {
        sleep(time);
        exit(0);
    }

    abort_watcher_pid = watcher;
    printf("Abort programado en %d segundos para %d procesos\n\n", time, abort_targets_count);
}

void cmd_pause(char **input, ActiveProcesses* active) {
    if (active->count == 0){
        printf("No hay procesos en ejecución, pause no se puede ejecutar.\n");
        return;
    }

    if (!is_number(input[1])){
        printf("Por favor indica un pid válido\n");
        return;
    }

    pid_t pid_pause = atoi(input[1]);

    for (int i = 0; i < MAX_ACTIVE_PROCESSES; i++){
        if (active->processes[i].pid == pid_pause){

            kill(pid_pause, SIGSTOP);
            active->processes[i].paused = 1; 
            gettimeofday(&active->processes[i].end, NULL);

            if (active->processes[i].paused_flag != NULL) {
                *(active->processes[i].paused_flag) = 1;
            }   

            double time = get_seconds(&active->processes[i]);

            printf("Proceso pausado: PID = %d\n\n", pid_pause);

            printf("%-8s %-16s %-10s %-8s %-10s %-12s\n", 
                "PID", "Nombre", "Tiempo(s)", "Pausado", "Exit code", "Signal value");

            printf("%-8d %-16s %-10.1f %-8d %-10d %-12d\n\n",
                            pid_pause,
                            active->processes[i].name,
                            time,
                            active->processes[i].paused,
                            active->processes[i].exit_code,
                            active->processes[i].signal_value);
            
            return;
        }
    }
    printf("Por favor indica un pid lanzado por burnssh\n");
}

void cmd_resume(char **input, ActiveProcesses* active) {
    if (active->count == 0) {
        printf("No hay procesos en ejecución, resume no se puede ejecutar.\n");
        return;
    }

    if (!is_number(input[1])) {
        printf("Por favor indica un pid válido\n");
        return;
    }

    pid_t pid_resume = atoi(input[1]);

    for (int i = 0; i < MAX_ACTIVE_PROCESSES; i++) {
        if (active->processes[i].pid == pid_resume) {
            if (!active->processes[i].paused) {
                printf("El proceso %d no está pausado\n", pid_resume);
                return;
            }

            struct timeval now;
            gettimeofday(&now, NULL);

            // Calcular cuánto tiempo estuvo pausado
            double paused_duration = (now.tv_sec - active->processes[i].end.tv_sec);
            paused_duration += (now.tv_usec - active->processes[i].end.tv_usec) / 1000000.0;

            // Adelantar el start para que no cuente el tiempo pausado
            active->processes[i].start.tv_sec += (long)paused_duration;
            active->processes[i].start.tv_usec += (long)((paused_duration - (long)paused_duration) * 1000000);

            active->processes[i].paused = 0;
            kill(pid_resume, SIGCONT);
            if (active->processes[i].paused_flag != NULL) {
                *(active->processes[i].paused_flag) = 0;
            }

            double time = get_seconds(&active->processes[i]);

            printf("Proceso reanudado: PID = %d\n\n", pid_resume);
            printf("%-8s %-16s %-10s %-8s %-10s %-12s\n",
                "PID", "Nombre", "Tiempo(s)", "Pausado", "Exit code", "Signal value");
            printf("%-8d %-16s %-10.1f %-8d %-10d %-12d\n\n",
                pid_resume,
                active->processes[i].name,
                time,
                active->processes[i].paused,
                active->processes[i].exit_code,
                active->processes[i].signal_value);

            return;
        }
    }
    printf("Por favor indica un pid lanzado por burnssh\n");
}