#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "servidor_config.h"
#include "dclient.h"
#include "respostas.h"
#include "cache.h"

//Converte a entrada de cache para uma resposta
GArray* convertToResposta(CacheEntry* entry, mensagem pedido) {

    GArray* lista_de_respostas = g_array_new(FALSE,FALSE,sizeof(Resposta));

    Resposta r;

    r.pid = pedido.pid;
    r.tipo = entry -> tipo;
    r.max_fragmentos = 1;
    r.n_fragmentos = 1;

    switch (entry -> tipo) {

        case PEDIDO_INSERT:
            r.index = entry->index_inserido;
            g_array_append_val(lista_de_respostas,r);
            break;
        
        case PEDIDO_GET:
            strncpy(r.string1, entry->titulo, sizeof(r.string1));
            strncpy(r.string2, entry->autores, sizeof(r.string2));
            strncpy(r.string3, entry->path, sizeof(r.string3));
            r.inteiro = entry->year;
            r.index = entry->index_inserido;
            g_array_append_val(lista_de_respostas,r);
            break;
    
        case PEDIDO_DELETE:
            r.index = entry->index_inserido;
            g_array_append_val(lista_de_respostas,r);
            break;

        case PEDIDO_LINES:
            r.inteiro = entry->n_linhas;
            g_array_append_val(lista_de_respostas,r);
        break;

        case PEDIDO_LIST:

            int ints_por_resposta = sizeof(r.string1) / sizeof(int);
            r.max_fragmentos = (entry->indices->len / ints_por_resposta) + 1;
            r.n_fragmentos = 1;
        
            int i_lista = 0;
            int i = 0;
            int numero_ints_por_pacote = 0;
            int fragmentos_enviados = 0;
        
            while (fragmentos_enviados < r.max_fragmentos) {
        
                numero_ints_por_pacote = 0;
        
                //Meter números na string1
                i = 0;
                while (i < ints_por_resposta && i_lista < entry->indices->len) {
            
                    int* lista_ints = (int*) r.string1;
                    lista_ints[i] = g_array_index(entry->indices,int,i_lista);
                    
                    i_lista++;
                    numero_ints_por_pacote++;
                    i++;
                
                }
        
                //Enviar mensagem
                r.inteiro = numero_ints_por_pacote;
                fragmentos_enviados++;
                g_array_append_val(lista_de_respostas,r);
                        
            }

        break;

        case PEDIDO_SHUTDOWN:
        case PEDIDO_INVALIDO:
            g_array_append_val(lista_de_respostas,r);
        break;

    }

    return lista_de_respostas;
}

//Cria o nome do pipe do cliente
char* criarPipeClienteNome(pid_t pid) {
 
    //String do pipe do cliente
    char meu_pipe[1024];
    sprintf(meu_pipe,"pipes/pipe_%d.pipe",pid); //Dar nome do pipe com base no seu ID

    return strdup(meu_pipe);

}

void processo_respostas(int pipe[2]) {

    Resposta r;
    ssize_t bytes_lidos;
    int i = 0;

    while((bytes_lidos = read(pipe[0], &r, sizeof(Resposta))) > 0) {

        char* pipe_client =  criarPipeClienteNome(r.pid);
        int fd_cliente = open(pipe_client, O_WRONLY);
        
        if (fd_cliente == -1) {
            perror("ERRO : Couldnt open client's pipe");
        }

        ssize_t bytes_escritos = write(fd_cliente, &r, sizeof(Resposta));

        if (bytes_escritos == -1) {
            perror("ERRO : Couldnt write in client's pipe");
        }

        close(fd_cliente);
        i++;
    }
    
    close(pipe[0]);
    _exit(0);

}
