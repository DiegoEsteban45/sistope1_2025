#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>


// Variable global para indicar que el token fue recibido
volatile sig_atomic_t token_recibido = 0;

/**
 * Manejador de señales para SIGUSR1.
 * Usa SA_SIGINFO para acceder a información extendida (como un valor entero).
 */
void manejador(int sig, siginfo_t *si, void *context) {
    int token = si->si_value.sival_int; // que pasa aqui?
    printf("Proceso %d recibió el token: %d\n", getpid(), token);
    token_recibido = 1;
}

int main(int argc, char *argv[]) {
    int opt;
    int numProcesos = -1;
    int tokenInicial = -1;
    int maxDecremento = -1;

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = manejador;
    sigemptyset(&sa.sa_mask);

    // Configuramos para que al llegar SIGUSR1 se use nuestro manejador
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Procesar opciones de línea de comandos
    while ((opt = getopt(argc, argv, "p:t:M:")) != -1) {
        switch (opt) {
            case 'p':
                numProcesos = atoi(optarg);
                break;
            case 't':
                tokenInicial = atoi(optarg);
                break;
            case 'M':
                maxDecremento = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Uso: %s -p <num_procesos> -t <token> -M <max_decremento>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Validar que todos los parámetros hayan sido ingresados
    if (numProcesos <= 0 || tokenInicial < 0 || maxDecremento <= 0) {
        fprintf(stderr, "Parámetros inválidos o faltantes.\n");
        fprintf(stderr, "Uso: %s -p <num_procesos> -t <token> -M <max_decremento>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Mostrar los valores para verificar
    printf("Número de procesos: %d\n", numProcesos);
    printf("Token inicial: %d\n", tokenInicial);
    printf("Máximo decremento: %d\n", maxDecremento);

    // Aquí seguiría la lógica de crear procesos.

    pid_t pidsVivos[numProcesos];
    pid_t pidsMuertos[numProcesos-1];
    int n = 5;

    for (int i = 0; i < numProcesos; i++) {

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);

        } else if (pid == 0) {
            // Código del hijo

            printf("Soy el hijo %d con PID %d\n", i, getpid() );
            // Cada hijo hace lo suyo aquí...
            exit(0);

        } else {

            // Proceso padre guarda el PID del hijo
            pidsVivos[i] = pid;
        }
    }






    return 0;
}
