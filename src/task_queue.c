#include <stdio.h>
#include "servidor_config.h"
#include <glib.h>
#include "task_queue.h"
#include "cache.h"

typedef struct task_queue_waiting {

    CacheEntry* entry;
    int fragmentos_restantes;

} TaskWaiting;

TaskQueue criaTaskQueue() {

    TaskQueue queue;

    queue.tabela = g_hash_table_new(g_int_hash, g_int_equal);

    return queue;

}

//Converter uma mensagem numa entrada de cache
CacheEntry* MensagemToCacheEntry(mensagem msg) {

    CacheEntry* entry = malloc(sizeof(CacheEntry));

    if (entry == NULL)
        return NULL;

    entry->indices = g_array_new(FALSE,FALSE,sizeof(int));

    switch (msg.cmd) {

        case PEDIDO_INSERT:

            entry->tipo = PEDIDO_INSERT;
            entry->index = msg.index;
            entry->index_inserido = msg.index2;
            strncpy(entry->titulo,msg.string1,sizeof(entry->titulo));
            strncpy(entry->autores,msg.string2,sizeof(entry->autores));
            entry->year = msg.inteiro;
            strncpy(entry->path,msg.string3,sizeof(entry->path));

            break;

        case PEDIDO_GET:

            entry->tipo = PEDIDO_GET;
            entry->index = msg.index;
            strncpy(entry->titulo,msg.string1,sizeof(entry->titulo));
            strncpy(entry->autores,msg.string2,sizeof(entry->autores));
            entry->year = msg.inteiro;
            strncpy(entry->path,msg.string3,sizeof(entry->path));
            entry->index_inserido = msg.index2;
            break;

        case PEDIDO_DELETE:

            entry->tipo = PEDIDO_DELETE;
            entry->index = msg.index;
            entry->index_inserido = msg.index2;
            break;

        case PEDIDO_LINES:

            entry->tipo = PEDIDO_LINES;
            strncpy(entry->keyword,msg.string1,200);
            entry->index = msg.index;
            entry->n_linhas = msg.inteiro;
            break;

        case PEDIDO_LIST:

            entry->tipo = PEDIDO_LIST;

            int numeros_restantes = msg.inteiro;
            int i = 0;

            while (i < 200 / sizeof(int) && numeros_restantes > 0) {

                int* int_list = (int*) msg.string2;
                int indice = int_list[i];
                g_array_append_val(entry->indices,indice);
                
                i++;
                numeros_restantes--;

            }

            strncpy(entry->keyword,msg.string1,200);

            break;

        default:
            return NULL;
            break;

    }

    return entry;

}

//Obter a entrada de cache com base na resposta
CacheEntry* getCacheEntry(TaskQueue* queue,mensagem msg) {

    //Mensagem é completa
    if (msg.n_fragmentos >= msg.max_fragmentos) {

        //Converte para entrada de cache
        return MensagemToCacheEntry(msg);

    }
    //Mensagem está fragmentada
    else {
        
        CacheEntry* entry = MensagemToCacheEntry(msg);
        int entry_id = msg.pid;

        //Um fragmento da mensagem já existe
        if (g_hash_table_contains(queue->tabela,&entry_id) == true) {
            
            TaskWaiting *valor = (TaskWaiting*) g_hash_table_lookup(queue->tabela, &entry_id);

            //Concatena os indices
            g_array_append_vals(valor->entry->indices, entry->indices->data, entry->indices->len);
            
            g_array_free(entry->indices,true);
            free(entry);
            
            //Atualizar os fragmentos restantes
            valor->fragmentos_restantes -= msg.n_fragmentos;
            
            //Mensagem está pronta
            if (valor->fragmentos_restantes <= 0) {

                CacheEntry* entry_pronta = valor->entry;
                g_hash_table_remove(queue->tabela, &entry_id);

                return entry_pronta;
            }
            else {
                return NULL;
            }

        }
        //Primeiro fragmento -> registar
        else {

            TaskWaiting* task_waiting = malloc(sizeof(TaskWaiting));

            if (task_waiting == NULL) {
                g_array_free(entry->indices,true);
                free(entry);
                return NULL;
            }

            task_waiting->entry = entry;
            task_waiting->fragmentos_restantes = msg.max_fragmentos - msg.n_fragmentos;

            int* chave = malloc(sizeof(pid_t));
            *chave = msg.pid;

            g_hash_table_insert(queue->tabela,chave,task_waiting);

        }

        return NULL;
    }

}

int compare_message(mensagem msg1 ,mensagem msg2) {

    if (msg1.cmd != msg2.cmd)
        return 0;

    switch (msg1.cmd) {

        case PEDIDO_GET:
            
            if (msg1.index == msg2.index) {
                return 1;
            }

            break;

        case PEDIDO_DELETE:
            
            if (msg1.index == msg2.index) {
                return 1;
            }

            break;

        case PEDIDO_INSERT:

            if (strcmp(msg1.string1,msg2.string1) == 0 && 
                strcmp(msg1.string2,msg2.string2) == 0 && 
                msg1.inteiro == msg2.inteiro && 
                strcmp(msg1.string3,msg2.string3) == 0) {
                
                return 1;
                }

            break;

        case PEDIDO_LINES:
            
            if (strcmp(msg1.string1,msg2.string1) == 0 && 
            msg1.index == msg2.index) {
                return 1;
            }

            break;

        case PEDIDO_LIST:
            
            if (strcmp(msg1.string1,msg2.string1) == 0) {
                return 1;
            }

            break;

        default:
            return 0;
            break;

    }

    return 0;

}

void printMensagem(mensagem msg) {

    printf("============\n");
    printf("PID : %d\n",msg.pid);
    printf("Comando : %d\n",msg.cmd);
    printf("String 1 : %s\n",msg.string1);
    printf("String 2 : %s\n",msg.string2);
    printf("String 3 : %s\n",msg.string3);
    printf("Numero : %d\n",msg.inteiro);
    printf("Vem do Servidor : %d\n",msg.comesFromServer);
    printf("Índice : %d\n",msg.index);
    printf("Número de fragmentos : %d\n",msg.n_fragmentos);
    printf("Número de fragmentos Máximo : %d\n",msg.max_fragmentos);
    printf("============\n");

}