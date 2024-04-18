#ifndef MONITOR_H
#define MONITOR_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define BUFFER_SIZE 10

typedef struct {
    int buffer[BUFFER_SIZE];
    int in;
    int out;
    sem_t empty;
    sem_t full;
    int count;
} CircularBuffer;

typedef struct {
    CircularBuffer* buffer;
    char* file_name;
    bool is_ph;
} BufferInfo;

void* recolector(void* arg);
void* h_ph(void* arg);
void* h_temperature(void* arg);
void circular_buffer_init(CircularBuffer* buffer);
void circular_buffer_destroy(CircularBuffer* buffer);
void circular_buffer_push(CircularBuffer* buffer, int value);
int circular_buffer_pop(CircularBuffer* buffer);
void write_to_file(BufferInfo* buffer_info, int value);

#endif // MONITOR_H