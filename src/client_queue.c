

#include "servidor_config.h"
#include <glib.h>
#include "cache.h"
#include <stdio.h>
#include "task_queue.h"
#include "client_queue.h"

ClientQueue criaClientQueue() {

    ClientQueue queue;

    queue.get = g_array_new(FALSE,FALSE,sizeof(mensagem));
    queue.insert = g_array_new(FALSE,FALSE,sizeof(mensagem));
    queue.delete = g_array_new(FALSE,FALSE,sizeof(mensagem));
    queue.lines = g_array_new(FALSE,FALSE,sizeof(mensagem));
    queue.list = g_array_new(FALSE,FALSE,sizeof(mensagem));

    return queue;

}

//Função que verifica se o pedido já existe
int pedidoJaExiste(GArray* lista, mensagem msg) {

    for (int i = 0 ; i < lista->len ; i++) {

        mensagem m = g_array_index(lista,mensagem,i);

        if (compare_message(msg,m) == 1)
            return 1;

    }

    return 0;

}

//Adiciona cliente à fila de espera
int adicionaClient(ClientQueue* queue,mensagem msg) {

    int res = 1;

    switch (msg.cmd) {

        case PEDIDO_GET:

            g_array_append_val(queue->get,msg);
            res = 1 - pedidoJaExiste(queue->get,msg);

            break;

        case PEDIDO_INSERT:

            g_array_append_val(queue->insert,msg);
            res = 1 - pedidoJaExiste(queue->insert,msg);

            break;

        case PEDIDO_DELETE:

            g_array_append_val(queue->delete,msg);
            res = 1 - pedidoJaExiste(queue->delete,msg);

            break;

        case PEDIDO_LINES:

            g_array_append_val(queue->lines,msg);
            res = 1 - pedidoJaExiste(queue->lines,msg);

            break;

        case PEDIDO_LIST:

            g_array_append_val(queue->list,msg);
            res = 1 - pedidoJaExiste(queue->list,msg);

            break;

        default:
            break;

    }

    return res;

}

//Dá os clientes que já podem ser respondidos
GArray* resolveClientes(ClientQueue* queue, CacheEntry* entry) {
    
    GArray* lista = g_array_new(FALSE,FALSE,sizeof(mensagem));

    switch (entry->tipo) {

        case PEDIDO_GET:

            for (int i = queue->get->len - 1; i >= 0; i--) {

                mensagem msg = g_array_index(queue->get,mensagem,i);

                if (compare_cache_entry_message(entry,msg) == 1) {

                    g_array_append_val(lista,msg);
                    g_array_remove_index(queue->get,i);

                }
            }

            break;

        case PEDIDO_INSERT:

            for (int i = queue->insert->len - 1; i >= 0; i--) {

                mensagem msg = g_array_index(queue->insert,mensagem,i);

                if (compare_cache_entry_message(entry,msg) == 1) {

                    g_array_append_val(lista,msg);
                    g_array_remove_index(queue->insert,i);

                }

            }

            break;

        case PEDIDO_DELETE:

            for (int i = queue->delete->len - 1; i >= 0; i--) {

                mensagem msg = g_array_index(queue->delete,mensagem,i);

                if (compare_cache_entry_message(entry,msg) == 1) {

                    g_array_append_val(lista,msg);
                    g_array_remove_index(queue->delete,i);

                }
            }

            break;

        case PEDIDO_LINES:

            for (int i = queue->lines->len - 1; i >= 0; i--) {

                mensagem msg = g_array_index(queue->lines,mensagem,i);

                if (compare_cache_entry_message(entry,msg) == 1) {

                    g_array_append_val(lista,msg);
                    g_array_remove_index(queue->lines,i);

                }

            }

            break;

        case PEDIDO_LIST:

            for (int i = queue->list->len - 1; i >= 0; i--) {

                mensagem msg = g_array_index(queue->list,mensagem,i);

                if (compare_cache_entry_message(entry,msg) == 1) {

                    g_array_append_val(lista,msg);
                    g_array_remove_index(queue->list,i);

                }
            }

            break;

        default:
            break;

    }

    return lista;

}