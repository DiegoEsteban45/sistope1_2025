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
    int token = si->si_value.sival_int;
    printf("Proceso %d recibió el token: %d\n", getpid(), token);
    token_recibido = 1;
}

int main() {
    // Estructura para manejar la señal
    // Usamos SA_SIGINFO para recibir información adicional
    struct sigaction sa; 
    sigset_t mask, oldmask;

    // Configura el manejador para SIGUSR1
    sa.sa_flags = SA_SIGINFO; 
    sa.sa_sigaction = manejador;
    sigemptyset(&sa.sa_mask);

    // Configura la acción para SIGUSR1
    // Esto permite que el manejador reciba información adicional
    // como el valor enviado por sigqueue
    // int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Bloquea temporalmente SIGUSR1 para evitar que llegue antes de que estemos listos
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &mask, &oldmask) < 0) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);

    } else if (pid == 0) {
        // Proceso hijo: espera de forma segura hasta que reciba la señal
        while (!token_recibido) {
            sigsuspend(&oldmask); // Espera SIGUSR1 desbloqueada temporalmente
        }
        exit(0);

    } else {

        // Proceso padre: envía un token al hijo usando sigqueue
        union sigval value;
        value.sival_int = 42;  // Token de ejemplo

        if (sigqueue(pid, SIGUSR1, value) == -1) {
            perror("sigqueue");
            exit(EXIT_FAILURE);
        }

        wait(NULL); // Espera a que el hijo termine
    }

    return 0;
}
