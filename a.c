#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>

volatile sig_atomic_t token_recibido = 0;
int pid_siguiente = -1;
bool estoy_muerto = false;

int maxDecremento;
int tokenInicial;
int numProcesos;
int *pids = NULL;
int ProcesosVivos;
int pidProc;


int numProcFaltantes;

// ---------------------- FUNCIONES AUXILIARES ----------------

int esta_muerto(pid_t pid) {
    if (kill(pid, 0) == -1) {
        sleep(2);
        if (errno == ESRCH) {
            return 1; // El proceso no existe
        }
        // Si errno es EPERM, el proceso existe pero no tienes permisos
    }
    return 0; // El proceso existe
}

bool comprobarGanador(int* pids){
    int numProcesosVivos = numProcesos;
    for(int i = 0; i < numProcesos; i++){
        if(numProcesosVivos == 1){
            printf("\nComprobando ganadores\n");
            return true;
        }
        else if(esta_muerto(pids[i])){
            numProcesosVivos = numProcesosVivos - 1;
        }
    }
    return false;
}


void imprimir_array(int *array, int tamano) {
    printf("[");
    for (int i = 0; i < tamano; i++) {
        printf("%d", array[i]);
        if (i < tamano - 1) {
            printf(", ");
        }
    }
    printf("]\n");
}

int seleccionar_proceso_vivo(int pid) {
    int inicio = pid; // Guardar el PID inicial para evitar bucles infinitos
    int index = -1;
    // Encontrar el índice del PID actual en el arreglo
    for (int i = 0; i < numProcesos; i++) {
        
        if (pids[i] == pid) {
            //printf("Encontramos pid: %d\n", pid);
            index = i;
            break;
        }
    }
    //printf("\nDoy a este pid: %d", pid);
    if (index == -1) {
        printf("Error: PID %d no encontrado en el arreglo.\n", pid);
        return -1;
    }

    // Recorrer el arreglo para encontrar el siguiente proceso vivo
    do {
        index = (index + 1) % numProcesos; // Avanzar al siguiente índice en el anillo
        if (!esta_muerto(pids[index])) {
            //printf("El proceso %d encontro al: %d\n", getpid(), pids[index]);
            sleep(1);
            return pids[index]; // Encontró un proceso vivo
        }
    } while (pids[index] != pid); // Detener si ha recorrido todo el anillo

    // Si solo queda un proceso vivo, devolver su propio PID
    if (!esta_muerto(pid)) {
        return pid;
    }

    // Si no hay procesos vivos, devolver -1
    return -1;
}

int siguiente(int *array, int pib){
    for(int i = 0; i < numProcesos; i++){
        if (i == numProcesos-1){
            return array [0];
        }
        if(array[i] == pib){
            return array[i+1];
        }
    }
}

// --------------------- MANEJADORES -------------------------

void manejadorSIGUSR2(int sig, siginfo_t *si, void *context) {
    pidProc = si->si_value.sival_int;

    //añadir al arreglo de pids 
    pids[numProcesos-numProcFaltantes] = pidProc;
    numProcFaltantes--;
    token_recibido = 1;
    printf("El proceso %d recibio el pid %d , numero de procesos faltantes es: %d\n", getpid(), pidProc ,numProcFaltantes);
    if(numProcFaltantes == 0){
        imprimir_array(pids,numProcesos);
        pid_siguiente = siguiente(pids,getpid());
    }
}

void manejadorSIGUSR1(int sig, siginfo_t *si, void *context) {

    int token = si->si_value.sival_int;
    srand(time(NULL)); // Semilla para la generación de números aleatorios
    int resta = (rand() % (maxDecremento - 1)) + 1;
    token -= resta;

    sleep(1); // Simular un tiempo de procesamiento
    if(comprobarGanador(pids)){
        printf("No hay procesos vivos restantes. Terminando el juego.\n");
        printf("Proceso %d es el último vivo por lo cual es el ganador", getpid());
        exit(0);
    }
    else{
        printf("Proceso %d recibió token, restó %d → nuevo token: %d\n", getpid(), resta, token);

        if (token < 0) {
            union sigval val;
            val.sival_int = tokenInicial;  // El líder podría reiniciar token aquí
            int pid_vivo = seleccionar_proceso_vivo(getpid());
            sigqueue(pid_vivo, SIGUSR1, val);
            printf("------------------------------Proceso %d eliminado\n", getpid());
            free(pids);
            exit(0);
        } else {
            union sigval val;
            val.sival_int = token;
            int pid_vivo = seleccionar_proceso_vivo(getpid());
            sleep(1);
            sigqueue(pid_vivo, SIGUSR1, val);
        }
    }
}

// ---------------------- MAIN -------------------------------

int main(int argc, char *argv[]) {
    

    int opt;
    tokenInicial = -1;
    maxDecremento = -1;
    numProcesos = -1;
    ProcesosVivos = 0;

    // Leer argumentos
    while ((opt = getopt(argc, argv, "p:t:M:")) != -1) {
        switch (opt) {
            case 'p': numProcesos = atoi(optarg); break;
            case 't': tokenInicial = atoi(optarg); break;
            case 'M': maxDecremento = atoi(optarg); break;
            default:
                fprintf(stderr, "Uso: %s -p <num_procesos> -t <token> -M <max_decremento>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (numProcesos <= 1 || tokenInicial < 0 || maxDecremento <= 1) {
        fprintf(stderr, "Parámetros inválidos\n");
        exit(EXIT_FAILURE);
    }
    ProcesosVivos = numProcesos;
    numProcFaltantes = numProcesos;


    pids = malloc(numProcesos * sizeof(pid_t));
    if (!pids) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Crear procesos hijos
    for (int i = 0; i < numProcesos; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // ------------------- Código del hijo -------------------

            // Configurar manejadores
            struct sigaction sa1, sa2;
            sa1.sa_flags = SA_SIGINFO;
            sa1.sa_sigaction = manejadorSIGUSR1;
            sigemptyset(&sa1.sa_mask);
            if (sigaction(SIGUSR1, &sa1, NULL) == -1) {
                perror("sigaction");
                exit(EXIT_FAILURE);
            }

            sa2.sa_flags = SA_SIGINFO;
            sa2.sa_sigaction = manejadorSIGUSR2;
            sigemptyset(&sa2.sa_mask);
            if (sigaction(SIGUSR2, &sa2, NULL)== -1) {
                perror("sigaction");
                exit(EXIT_FAILURE);
            }

            // Esperar SIGUSR2 para conocer al siguiente
            sigset_t espera;
            sigemptyset(&espera);
            while (!token_recibido)
                sigsuspend(&espera);

            // Esperar tokens indefinidamente
            while (1)
                pause();

        } else {
            pids[i] = pid;
            printf("👶 Padre creó al hijo %d con PID %d\n", i, pid);
        }
    }

    sleep(1); // Dar tiempo a que los hijos instalen sus manejadores

    // Enviar SIGUSR2 a cada hijo con el PID del siguiente (anillo)
    printf("PREPARANDO JUEGO......\n");
    for (int i = 0; i < numProcesos; i++) {
        for(int j = 0; j < numProcesos; j++){
            union sigval val;
            val.sival_int = pids[j]; // le vamos a pasar los pids a cada proceso de la siguiente manera
            sigqueue(pids[i], SIGUSR2, val);
            sleep(1);
        }

    }
    printf("PREPARACIONES LISTAS\n");
    // terminamos todos los procesos ; esto se saca despues

    sleep(1); // Asegurar entrega


   
    // Enviar el token inicial al primer proceso
    union sigval token_val;
    token_val.sival_int = tokenInicial;
    sigqueue(pids[0], SIGUSR1, token_val);
    printf("🚀 Padre envió token inicial %d al proceso %d\n", tokenInicial, pids[0]);
  
    // Esperar que todos los hijos terminen
    for (int i = 0; i < numProcesos; i++) {
        wait(NULL);
    }

    free(pids);
    printf("\nTodos los procesos han terminado, FIN\n");
    
    return 0;
}
