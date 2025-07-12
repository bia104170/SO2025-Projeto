#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>

#include "servidor_config.h"

#include "cache.h"
#include "client_queue.h"
#include "task_queue.h"

#include "database.h"
#include "respostas.h"

int canal_status;
int database_pipe_status;
int resposta_pipe_status;

int n_processos = 0;
int pipe_anonimo_database[2];
int pipe_respostas[2];
int fd_canal;
mensagem mensagem_recebida;
int mensagem_id = 0;
bool servidor_terminou;

ClientQueue client_queue;
TaskQueue task_queue;

Cache cache;
enum Cache_Tipo cache_tipo;
int numero_cache_entries;
int cache_hit = 0;
int cache_miss = 0;

const char* tipo_mensagem_string[] = {"---","ADD","GET","DELETE","LINES","LIST","SHUTDOWN"};

void enviarPedidoBaseDados() {

    PedidoBaseDados pedido_base_dados;
    pedido_base_dados.ID_pedido = mensagem_id;
    int mensagem_valida = 1;

    switch (mensagem_recebida.cmd) {

        case PEDIDO_INSERT:

            pedido_base_dados.tipo = TIPO_PEDIDO_BD_INDEXAR;
            strncpy(pedido_base_dados.titulo,mensagem_recebida.string1,sizeof(mensagem_recebida.string1));
            strncpy(pedido_base_dados.autores,mensagem_recebida.string2,sizeof(mensagem_recebida.string2));
            pedido_base_dados.year = mensagem_recebida.inteiro;
            strncpy(pedido_base_dados.path,mensagem_recebida.string3,sizeof(mensagem_recebida.string3));

            break;

        case PEDIDO_GET:

            pedido_base_dados.tipo = TIPO_PEDIDO_BD_CONSULTAR;
            pedido_base_dados.index = mensagem_recebida.index;

            break;

        case PEDIDO_DELETE:

            pedido_base_dados.tipo = TIPO_PEDIDO_BD_DELETAR;
            pedido_base_dados.index = mensagem_recebida.index;

            break;

        case PEDIDO_LINES:

            pedido_base_dados.tipo = TIPO_PEDIDO_BD_N_LINHAS;
            pedido_base_dados.index = mensagem_recebida.index;
            strncpy(pedido_base_dados.titulo,mensagem_recebida.string1,sizeof(pedido_base_dados.titulo));

            break;

        case PEDIDO_LIST:

            pedido_base_dados.tipo = TIPO_PEDIDO_BD_LISTAR;
            pedido_base_dados.nr_processos = mensagem_recebida.inteiro;
            strncpy(pedido_base_dados.titulo,mensagem_recebida.string1,sizeof(pedido_base_dados.titulo));

            break;

        case PEDIDO_SHUTDOWN:

            pedido_base_dados.tipo = TIPO_PEDIDO_BD_FECHAR;

            break;

        default:

            mensagem_valida = 0;

            break;


    }

    //Mensagem válida a enviar
    if (mensagem_valida == 1) {
        write(pipe_anonimo_database[1],&pedido_base_dados,sizeof(PedidoBaseDados));
    }

}

void escreveRespostaServidor(CacheEntry* entry, mensagem msg) {

    GArray* lista_respostas = convertToResposta(entry, msg);

    for (int i = 0 ; i < lista_respostas->len ; i++) {

        Resposta r = g_array_index(lista_respostas,Resposta,i);
        ssize_t bytes_escritos = write(pipe_respostas[1], &r, sizeof(Resposta));

        if (bytes_escritos == -1) {
            perror("ERRO : Couldnt write in response's pipe");
        }

    }

}

void adeusCliente(mensagem msg) {
    
    Resposta r;
    r.tipo = PEDIDO_SHUTDOWN;
    r.pid = msg.pid;

    ssize_t bytes_escritos = write(pipe_respostas[1], &r, sizeof(Resposta));

    if (bytes_escritos == -1) {
        perror("ERRO : Couldnt write in response's pipe");
    }

    printf("CLIENT | BYE\n");

}

//Função que trata das mensagens do servidor
void server_handler() {

    printf("SERVER | %s",tipo_mensagem_string[mensagem_recebida.cmd]);

    //Transforma numa entrada de cache
    CacheEntry* nova_entry = getCacheEntry(&task_queue,mensagem_recebida);

    if (nova_entry != NULL) {

        GArray* clientes_pendentes = resolveClientes(&client_queue,nova_entry);

        for (int i = 0 ; i < clientes_pendentes->len ; i++) {
            
            mensagem msg = g_array_index(clientes_pendentes,mensagem,i);
            escreveRespostaServidor(nova_entry,msg);
            
        }

        cache_filter(&cache,nova_entry); //Filtrar cache
        cache_insert(&cache,nova_entry); //Inserir na cache
        g_array_free(clientes_pendentes, TRUE);

    }
}


void client_handler() {

    CacheEntry* entry = cache_get(&cache,mensagem_recebida);

    if (entry == NULL) {
        
        cache_miss++;
        printf("CLIENT | %s -> CACHE MISS (%d)",tipo_mensagem_string[mensagem_recebida.cmd],cache_miss);
        
        //Adiciona cliente.
        // return : 1 -> novo pedido
        // return : 0 -> já existe um pedido igual
        if (adicionaClient(&client_queue,mensagem_recebida) == 0) {
            enviarPedidoBaseDados();
        }
        

    }
    else {

        cache_hit++;
        printf("CLIENT | %s -> CACHE HIT (%d)",tipo_mensagem_string[mensagem_recebida.cmd],cache_hit);

        escreveRespostaServidor(entry, mensagem_recebida);

    }

}

//Obtem as informações do servidor e inicia a cache
int getInfoCache(int N, char** tokens) {

    const char* uso = "[USAGE] ./dclient <cache> <number_of_cache_entries>\n\nTypes of cache:\n\tnon : No cache\n\tfifo : Cache with FIFO policy\n\tfrequency : Cache which removes the least used\n\ttime : Cache which removes the oldest entry\n\n";

    if (N < 3) {
        printf("%s",uso);
        return 0;
    }

    numero_cache_entries = atoi(tokens[2]);

    if (numero_cache_entries <= 0) {
        printf("Please insert a valid number of cache entries\n");
        return 0;
    }

    char* tipo = tokens[1];

    if (strcmp(tipo,"non") == 0) {
        cache_tipo = NO_CACHE;
        return 1;
    }
    if (strcmp(tipo,"fifo") == 0) {
        cache_tipo = CACHE_FIFO;
        return 1;
    }
    if (strcmp(tipo,"frequency") == 0) {
        cache_tipo = CACHE_FREQUENCY;
        return 1;
    }
    if (strcmp(tipo,"time") == 0) {
        cache_tipo = CACHE_TIME;
        return 1;
    }

    printf("%s",uso);
    return 0;

}

//Processo principal para o servidor
int main(int argn, char** args) {

    if (getInfoCache(argn,args) == 0)
        return 1;

    int status_error = 0;
    servidor_terminou = false;

    // INICIALIZAÇÃO DOS RECURSOS ####################
    cache = createCache(cache_tipo,numero_cache_entries);
    client_queue = criaClientQueue();
    task_queue = criaTaskQueue();


    //Apagar o canal caso seja necessário
    unlink(PIPE_CANAL);

    // CANAL ####################
    canal_status = mkfifo(PIPE_CANAL,0666);

    if (canal_status == -1) {

        perror("Canal : Erro na criação do pipe com nome\n");
        goto fechar_servidor;

    }

    // DATABASE #######################
    DataBaseConfig* database_config = iniciarFicheiro();

    if (database_config == NULL) {

        status_error = 1;
        goto fechar_servidor;

    }

    //Criar pipe anonimo da base de dados
    database_pipe_status = pipe(pipe_anonimo_database);

    if (database_pipe_status == -1) {

        perror("Base de Dados : Erro na criação dos pipes anonimos\n");
        status_error = 1;
        goto fechar_servidor;

    }

    pid_t pid_database = fork();

    switch (pid_database) {

        case -1:

            perror("Base de Dados : Erro na criação do processo\n");
            status_error = 1;
            goto fechar_servidor;
            break;

        case 0:

            //Processo da base de dados fecha o escritor do pipe anonimo
            close(pipe_anonimo_database[1]);
            run_database(pipe_anonimo_database,database_config);
            _exit(0);

            break;


        default:

            //Processo pai fecha o leitor do pipe anonimo e limpa a config do servidor
            n_processos++;
            close(pipe_anonimo_database[0]);
            g_queue_free(database_config->indexes_eliminados);
            free(database_config);
            database_config = NULL;

            break;

    }

    // RESPOSTAS ######################
    resposta_pipe_status = pipe(pipe_respostas);

    if (resposta_pipe_status == -1) {

        perror("Respostas : Erro na criação dos pipes anonimos\n");
        status_error = 1;
        goto fechar_servidor;
    }

    pid_t pid_respostas = fork();
    switch (pid_respostas) {

        case -1:
            perror("Respostas : Erro na criação do processo\n");
            status_error = 1;
            goto fechar_servidor;
            break;

        case 0:

            n_processos++;
            close(pipe_anonimo_database[1]);
            close(pipe_respostas[1]);
            processo_respostas(pipe_respostas);
            _exit(0);
            break;

        default:

            close(pipe_respostas[0]);
            break;
    }    


    // LEITURA DE PEDIDOS ##############

    fd_canal = open(PIPE_CANAL,O_RDONLY);
    ssize_t bytes_lidos = 0;

    while ((bytes_lidos = read(fd_canal,&mensagem_recebida,sizeof(mensagem))) > 0) {
            
        printf("%d. ",mensagem_id);

        //Servidor não atende mais pedidos
        if (servidor_terminou == true) {
            adeusCliente(mensagem_recebida);
        }

        //Pedido válido
        else {
            //Pedido para fechar o servidor
            if (mensagem_recebida.cmd == PEDIDO_SHUTDOWN) {

                if (mensagem_recebida.comesFromServer == 1) {

                    n_processos--;
                    servidor_terminou = true; //Base de Dados confirmou que fechou, então o servidor pode encerrar
                    printf("SERVER | BYE\n");

                }
                else {

                    //Envia um pedido na base de dados a informar que deve encerrar as suas atividades
                    enviarPedidoBaseDados();
                    adeusCliente(mensagem_recebida);

                }

            }
            //Mensagens uteis
            else {

                //Mensagem do servidor
                if (mensagem_recebida.comesFromServer == 1) {

                    server_handler();

                }
                //Mensagem do cliente
                else {

                    client_handler();

                }

                printf("\n");
                mensagem_id++;

            }
        }

    }

    //Encerrar
    close(pipe_respostas[1]);

    //Aguardar os processos filhos
    for (int i = 0 ; i < n_processos ; i++)
        wait(NULL);

    printf("Server closed...\n");


fechar_servidor:

    //Fechar canal
    if (canal_status >= 0) {
        unlink(PIPE_CANAL);
    }

    //Fechar pipe anonimo database
    if (database_pipe_status >= 0) {
        close(pipe_anonimo_database[0]);
        close(pipe_anonimo_database[1]);
    }

    //Fechar pipe anonimo database
    if (resposta_pipe_status >= 0) {
        close(pipe_respostas[0]);
        close(pipe_respostas[1]);
    }

    //Apagar as config da base de dados
    if (database_config != NULL) {

        g_queue_free(database_config->indexes_eliminados);
        free(database_config);
    }

    return status_error;

}