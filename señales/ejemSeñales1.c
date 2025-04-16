#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

void miHandler(int sig) {
    printf("🔔 Señal recibida: %d\n", sig);
    exit(0);
}

int main() {
    signal(SIGINT, miHandler);   // Ctrl+C
    signal(SIGTERM, miHandler);  // kill <pid>

    while (1) {
        printf("Esperando señales...\t");
        printf("PID: %d\n", getpid());
        sleep(2);
    }

    return 0;
}

