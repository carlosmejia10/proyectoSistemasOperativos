#include "monitor.h" // Incluye el archivo de cabecera "monitor.h"
#include <unistd.h> // Incluye el archivo de cabecera necesario para usar funciones del sistema operativo

int main(int argc, char* argv[]) { // Función principal del programa
    // Si el número de argumentos no es 5, imprime un mensaje de uso y termina el programa
    if (argc != 9) {
        printf("Usage: %s -b <tam_buffer> -t <file-temp> -h <file-ph> -p <pipe-nominal>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Convierte el tercer argumento a un entero y lo guarda en la variable tam_buffer
    int tam_buffer = atoi(argv[2]);
    // Guarda el quinto argumento en la variable file_temp
    char* file_temp = argv[4];
    // Guarda el séptimo argumento en la variable file_ph
    char* file_ph = argv[6];
    // Guarda el noveno argumento en la variable pipe_name
    char* pipe_name = argv[8];

    // Crea un pipe con el nombre almacenado en pipe_name y permisos de lectura y escritura para el propietario
    mkfifo(pipe_name, 0666);

    // Define una variable para el identificador del hilo recolector
    pthread_t h_recolector_id;

    // Crea el hilo recolector
    pthread_create(&h_recolector_id, NULL, recolector, NULL);

    // Espera a que termine el hilo recolector
    pthread_join(h_recolector_id, NULL);

    // Elimina el pipe
    unlink(pipe_name);

    // Retorna 0 para indicar que el programa terminó correctamente
    return 0;
}