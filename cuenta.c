#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("=== PROGRAMA DE PRUEBA ===\n");
    printf("PID: %d\n", getpid());
    
    if (argc > 1) {
        printf("Argumentos recibidos: ");
        for (int i = 1; i < argc; i++) {
            printf("%s ", argv[i]);
        }
        printf("\n");
    }
    
    printf("\nComenzando a contar...\n");
    
    for (int i = 1; i <= 10; i++) {
        printf("Contando: %d\n", i);
        sleep(1);  // Espera 1 segundo entre números
    }
    
    printf("\n¡Cuenta terminada!\n");
    return 0;
}