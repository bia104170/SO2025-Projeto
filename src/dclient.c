#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>

#include "servidor_config.h"
#include "cliparser.h"
#include "dclient.h"
#include "task_queue.h"
#include "respostas.h"

int comparar_ints(const void* a_, const void* b_) {
    const int* a = a_;
    const int* b = b_;
    return *a - *b;
}

//Função que dá a lista ordenada e sem repetidos
GArray* ordenar_lista_indices(GArray* lista) {

    if (lista == NULL || lista->len <= 0) 
        return g_array_new(FALSE, FALSE, sizeof(int));

    int* temp = malloc(sizeof(int) * lista->len);

    for (int i = 0; i < lista->len; i++)
        temp[i] = g_array_index(lista, int, i);

    //Ordena a lista temporaria
    qsort(temp, lista->len, sizeof(int), comparar_ints);

    //Remover indices duplicados
    GArray* lista_ordenada = g_array_new(FALSE, FALSE, sizeof(int));
    g_array_append_val(lista_ordenada, temp[0]); // adiciona o primeiro

    for (guint i = 1; i < lista->len; i++) {

        if (temp[i] != temp[i - 1]) {
            g_array_append_val(lista_ordenada, temp[i]);
        }

    }

    free(temp);
    g_array_free(lista,TRUE);

    return lista_ordenada;
}

//Função que processa a resposta
void processar_resposta(Resposta* r, int* pacotes_restantes, bool* primeiro_pacote, GArray* lista_de_indices) {

    if (*primeiro_pacote == false) {
        *pacotes_restantes = r->max_fragmentos;
        *primeiro_pacote = true;
    }

    if (r == NULL) {
        printf("Empty response.\n");
        return;
    }

    *pacotes_restantes -= r->n_fragmentos;

    switch (r->tipo) {

        case PEDIDO_INSERT:
            printf("Document %d indexed\n", r->index);   
            break;
            
        case PEDIDO_DELETE:

            if (r->index == -1) {
                printf("Document does not exist.\n");
            }
            else {
                printf("Index entry %d deleted\n", r->index);
            }  

            break;
        case PEDIDO_LINES:

            printf("%d\n", r->inteiro);
            break;

        case PEDIDO_LIST:
            
            //Adicionar números na lista
            int N = r->inteiro;
            int* lista = (int*) r->string1;

            for (int i = 0 ; i < N ; i++)
                g_array_append_val(lista_de_indices,lista[i]);

            //Lista completa -> devemos apresentar a resposta
            if (*pacotes_restantes <= 0) {

                GArray* lista_a_apresentar = ordenar_lista_indices(lista_de_indices);

                printf("[");

                bool primeiro_elemento = true;
                for(int i = 0; i < lista_a_apresentar->len; i++) {
                    
                    if (primeiro_elemento == true) {

                        primeiro_elemento = false;
                        printf("%d", g_array_index(lista_a_apresentar, int, i));

                    }
                    else {

                        printf(", %d", g_array_index(lista_a_apresentar, int, i));

                    }                    
                    
                }

                printf("]\n");

            }

            break;

        case PEDIDO_GET:
        
            if (r->index >= 0) {
                printf("Title: %s\n", r->string1);
                
                char* token = strtok(r->string2, ";");

                printf("Authors:\n");
                while (token != NULL) {
                    printf("  %s\n", token);
                    token = strtok(NULL, ";");
                }

                printf("Year: %d\n", r->inteiro);
                printf("Path: %s\n", r->string3);
            }
            else {
                printf("Document does not exist.\n");
            }
            break;

        case PEDIDO_SHUTDOWN:
            printf("Server is shuting down.\n");
            break;

        default:
            printf("Invalid Response.\n");
            break;

    }

}

//Função que inicia o cliente
int main(int argc, char** argv) {

    // vai ler o input do terminal, fazer parse e enviar para o servidor [tamanho: sizeof(mensagem)]
    mensagem msg;
    int mensagem_valida = 0;
    parseInput(argv,argc,&msg,&mensagem_valida); // a estrutura "mensagem" é modificada pelo parser

    if (mensagem_valida == 0) {
        return 1;
    }

    //printMensagem(msg);

    //String do pipe do cliente
    char* meu_pipe = criarPipeClienteNome(getpid());

    //Abrir o meu pipe
    int meu_pipe_status = mkfifo(meu_pipe,0666);

    //Correu bem
    if (meu_pipe_status == 0) {

        //Abrir pipe do servidor
        int pipe_servidor = open(PIPE_CANAL,O_WRONLY);

        //Conseguiu abrir
        if (pipe_servidor >= 0) {


            write(pipe_servidor,&msg,sizeof(mensagem));

            //Fechar pipe do servidor
            pipe_servidor = close(pipe_servidor);

            //Erro
            if (pipe_servidor == -1) {
                unlink(meu_pipe);
                return 1;
            }
                
            int fd_cliente = open(meu_pipe, O_RDONLY);

            if (fd_cliente == -1) {
                perror("ERRO : Couldnt open the client's named pipe\n");
            } else {
        
                Resposta resposta;
                ssize_t bytes_lidos = 0;

                GArray* lista_indices = g_array_new(FALSE,FALSE,sizeof(int));
                int pacotes_recebidos_restantes = 9999;
                bool primeiro_pacote = false;

                while (pacotes_recebidos_restantes > 0) {
        
                    fflush(stdout);
                    bytes_lidos = read(fd_cliente, &resposta, sizeof(Resposta));

                    if (bytes_lidos == sizeof(Resposta)) {
                        processar_resposta(&resposta, &pacotes_recebidos_restantes, &primeiro_pacote, lista_indices);
                    } 
                    else if (bytes_lidos < 0) {
                        perror("ERRO : Couldnt read client's pipe");
                        break;
                    }

                }
        
                close(fd_cliente);

            }

        }
        //Não conseguiu abrir
        else
            printf("ERRO : Couldnt connect with server...\n");
    
        //Destruir o meu pipe
        meu_pipe_status = unlink(meu_pipe);

        //Correu mal
        if (meu_pipe_status == -1)
            return 1;

    }
    //Ocorreu um erro
    else {
        perror("ERRO : Couldnt create client's named pipe\n");
        return 1;
    }

    return 0;

}
