#ifndef TASK_QUEUE
#define TASK_QUEUE

#include "servidor_config.h"
#include <glib.h>
#include "cache.h"

typedef struct task_queue {

    GHashTable* tabela;

} TaskQueue;

void printMensagem(mensagem msg);
TaskQueue criaTaskQueue();
CacheEntry* getCacheEntry(TaskQueue* queue,mensagem msg);
int compare_message(mensagem msg1 ,mensagem msg2);

#endif