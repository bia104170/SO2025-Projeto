#ifndef ENTRY_QUEUE
#define ENTRY_QUEUE

#include "servidor_config.h"
#include <glib.h>

typedef struct client_queue {

    GArray* get;
    GArray* insert;
    GArray* delete;
    GArray* lines;
    GArray* list;

} ClientQueue;

ClientQueue criaClientQueue();
int adicionaClient(ClientQueue* queue,mensagem msg);
GArray* resolveClientes(ClientQueue* queue, CacheEntry* entry);
#endif