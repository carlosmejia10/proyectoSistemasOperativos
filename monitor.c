#include "monitor.h" // Incluye el archivo de cabecera "monitor.h"
#include <unistd.h> 

// Inicializa un buffer circular
void circular_buffer_init(CircularBuffer* buffer) {
    buffer->in = 0; // Inicializa la posición de entrada en 0
    buffer->out = 0; // Inicializa la posición de salida en 0
    buffer->count = 0; // Inicializa el contador en 0
    sem_init(&buffer->empty, 0, BUFFER_SIZE); // Inicializa el semáforo 'empty' con el tamaño del buffer
    sem_init(&buffer->full, 0, 0); // Inicializa el semáforo 'full' en 0
}

// Destruye un buffer circular
void circular_buffer_destroy(CircularBuffer* buffer) {
    sem_destroy(&buffer->empty); // Destruye el semáforo 'empty'
    sem_destroy(&buffer->full); // Destruye el semáforo 'full'
}

// Inserta un valor en el buffer circular
void circular_buffer_push(CircularBuffer* buffer, int value) {
    sem_wait(&buffer->empty); // Espera hasta que haya espacio en el buffer
    buffer->buffer[buffer->in] = value; // Inserta el valor en la posición de entrada
    buffer->in = (buffer->in + 1) % BUFFER_SIZE; // Actualiza la posición de entrada
    buffer->count++; // Incrementa el contador
    sem_post(&buffer->full); // Incrementa el semáforo 'full'
}

// Extrae un valor del buffer circular
int circular_buffer_pop(CircularBuffer* buffer) {
    int value;
    sem_wait(&buffer->full); // Espera hasta que haya un elemento en el buffer
    value = buffer->buffer[buffer->out]; // Extrae el valor en la posición de salida
    buffer->out = (buffer->out + 1) % BUFFER_SIZE; // Actualiza la posición de salida
    buffer->count--; // Decrementa el contador
    sem_post(&buffer->empty); // Incrementa el semáforo 'empty'
    return value; // Devuelve el valor extraído
}

// Escribe un valor en un archivo
void write_to_file(BufferInfo* buffer_info, int value) {
    time_t rawtime;
    struct tm* timeinfo;
    FILE* file;

    file = fopen(buffer_info->file_name, "a"); // Abre el archivo en modo de adición
    if (file == NULL) { // Si no se pudo abrir el archivo
        perror("Error al abrir el archivo"); // Imprime un mensaje de error
        return; // Sale de la función
    }

    time(&rawtime); // Obtiene el tiempo actual
    timeinfo = localtime(&rawtime); // Convierte el tiempo a la hora local

    fprintf(file, "%s: %d\n", asctime(timeinfo), value); // Escribe la hora y el valor en el archivo
    fclose(file); // Cierra el archivo
}

void* recolector(void* arg) {
    // Se definen dos buffers circulares para almacenar los valores de PH y temperatura
    CircularBuffer ph_buffer;
    CircularBuffer temp_buffer;

    // Se crean punteros a los buffers
    CircularBuffer* ph_buffer_ptr = &ph_buffer;
    CircularBuffer* temp_buffer_ptr = &temp_buffer;

    // Se crea un array de punteros a los buffers
    CircularBuffer* buffers[] = {&ph_buffer, &temp_buffer};

    // Se definen los nombres de los archivos donde se almacenarán los valores de PH y temperatura
    char* file_ph = "./file-ph.txt";
    char* file_temp = "./file-temp.txt";

    // Se definen las variables para los descriptores de archivo de los pipes
    int ph_fd, temp_fd;

    // Se definen las variables para almacenar los valores de PH y temperatura
    int ph_value, temp_value;

    // Se define una variable booleana para controlar si todos los sensores están desconectados
    bool all_disconnected = false;

    // Se definen las variables para los streams de los pipes
    FILE* ph_pipe, *temp_pipe;

    // Se inicializan los buffers circulares
    circular_buffer_init(&ph_buffer);
    circular_buffer_init(&temp_buffer);

// Se abren los pipes en modo de escritura
ph_fd = open("./pipe-ph", O_WRONLY);
temp_fd = open("./pipe-temp", O_WRONLY);

// Se abren los streams de los pipes en modo de lectura
ph_pipe = fopen("./pipe-ph", "r");
temp_pipe = fopen("./pipe-temp", "r");

// Check if the files were successfully opened
if (ph_pipe == NULL || temp_pipe == NULL) {
    perror("Error al abrir el archivo");
    return NULL;
}

    // Se definen las variables para los identificadores de los hilos
    pthread_t h_ph_id, h_temp_id;

    // Se definen las estructuras de información de los buffers
    BufferInfo ph_buffer_info = {ph_buffer_ptr, file_ph};
    BufferInfo temp_buffer_info = {temp_buffer_ptr, file_temp};

    // Se crean los hilos para manejar los valores de PH y temperatura
    pthread_create(&h_ph_id, NULL, h_ph, &ph_buffer_info);
    pthread_create(&h_temp_id, NULL, h_temperature, &temp_buffer_info);

    // Se abren los streams de los pipes en modo de lectura
    ph_pipe = fdopen(ph_fd, "r");
    temp_pipe = fdopen(temp_fd, "r");

    // Mientras no todos los sensores estén desconectados
    while (!all_disconnected) {
        // Si se puede leer un valor de PH del pipe
        if (fscanf(ph_pipe, "%d", &ph_value) == 1) {
            // Si el valor de PH es negativo, se imprime un error
            if (ph_value < 0) {
                printf("Error: Valor negativo recibido en PH\n");
            } else {
                // Si no, se inserta el valor en el buffer de PH
                circular_buffer_push(ph_buffer_ptr, ph_value);
            }
        }

        // Si se puede leer un valor de temperatura del pipe
        if (fscanf(temp_pipe, "%d", &temp_value) == 1) {
            // Si el valor de temperatura es negativo, se imprime un error
            if (temp_value < 0) {
                printf("Error: Valor negativo recibido en temperatura\n");
            } else {
                // Si no, se inserta el valor en el buffer de temperatura
                circular_buffer_push(temp_buffer_ptr, temp_value);
            }
        }

        // Se cuenta cuántos buffers tienen al menos un valor
        int count = 0;
        for (int i = 0; i < 2; i++) {
            if (buffers[i]->count > 0) {
                count++;
            }
        }

        // Si ninguno de los buffers tiene valores
        if (count == 0) {
            // Se duerme el hilo durante 10 segundos
            sleep(10);
            // Se marca que todos los sensores están desconectados
            all_disconnected = true;
            // Se inserta un valor de -1 en todos los buffers para indicar que no hay más datos
            for (int i = 0; i < 2; i++) {
                circular_buffer_push(buffers[i], -1);
            }
        }
    }

    // Se espera a que terminen los hilos
    pthread_join(h_ph_id, NULL);
    pthread_join(h_temp_id, NULL);

    // Se cierran los streams de los pipes
    fclose(ph_pipe);
    fclose(temp_pipe);

    // Se destruyen los buffers circulares
    circular_buffer_destroy(&ph_buffer);
    circular_buffer_destroy(&temp_buffer);

    // Se imprime un mensaje indicando que se ha terminado el procesamiento de las medidas
    printf("Finalización del procesamiento de medidas\n");

    return NULL;
}

// Función para manejar los valores de PH
void* h_ph(void* arg) {
    // Se obtiene la información del buffer a partir del argumento
    BufferInfo* buffer_info = (BufferInfo*)arg;
    // Se obtiene el buffer a partir de la información del buffer
    CircularBuffer* buffer = buffer_info->buffer;
    // Se obtiene el nombre del archivo a partir de la información del buffer

    // Se entra en un bucle infinito
    while (1) {
        // Se extrae un valor del buffer
        int value = circular_buffer_pop(buffer);

        // Si el valor es -1, se rompe el bucle
        if (value == -1) {
            break;
        }

        // Si el valor está fuera del rango de PH (0-14), se imprime una alerta
        if (value < 0 || value > 14) {
            printf("Alerta: Valor de PH fuera de rango: %d\n", value);
        }

        // Se escribe el valor en el archivo
        write_to_file(buffer_info, value);
    }

    // La función retorna NULL
    return NULL;
}

// Función para manejar los valores de temperatura
void* h_temperature(void* arg) {
    // Se obtiene la información del buffer a partir del argumento
    BufferInfo* buffer_info = (BufferInfo*)arg;
    // Se obtiene el buffer a partir de la información del buffer
    CircularBuffer* buffer = buffer_info->buffer;
    // Se obtiene el nombre del archivo a partir de la información del buffer

    // Se entra en un bucle infinito
    while (1) {
        // Se extrae un valor del buffer
        int value = circular_buffer_pop(buffer);

        // Si el valor es -1, se rompe el bucle
        if (value == -1) {
            break;
        }

        // Si el valor está fuera del rango de temperatura (0-50), se imprime una alerta
        if (value < 0 || value > 50) {
            printf("Alerta: Valor de temperatura fuera de rango: %d\n", value);
        }

        // Se escribe el valor en el archivo
        write_to_file(buffer_info, value);
    }

    // La función retorna NULL
    return NULL;
}