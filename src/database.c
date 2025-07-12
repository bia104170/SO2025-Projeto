#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>

#include "servidor_config.h"
#include "database.h"

#include <glib.h>

//Obter offset na Base de Dados de um certo indice
off_t get_index_basedados(int index) {
    return index * sizeof(Index);
}

/*
Indexar um novo documento
dclient -a "title" "authors" "year" "path"

    title : titulo do documento
    authors : autores do documento (separar por ; quando existem vários)
    year: ano do documento
    path : caminho relativo do documento

Exemplo de uso:
./dclient -a "Romeo and Juliet" "William Shaspeare" "1997" "1112.txt"
Document 1 indexed

*/
int tarefa_basedados_indexar(DataBaseConfig* config,long tempo, char* titulo, char* autores, int ano, char* path) {
    
    int index = -1;

    //Ver se tem indices que podem ser reciclados
    if (g_queue_get_length(config->indexes_eliminados) > 0) {

        int* index_coletado = (int*) g_queue_pop_head(config->indexes_eliminados);
        index = GPOINTER_TO_INT(index_coletado);
    }
    else {

        index = config->N;
        config->N++;

    }

    //Índice válido
    if (index >= 0) {

        int fd = open(FICHEIRO_NOME,O_WRONLY);
        off_t offset = get_index_basedados(index);
        lseek(fd,offset,SEEK_SET);

        Index dado;

        dado.valido = 1;
        dado.tempo_logico = tempo;

        strncpy(dado.titulo,titulo,200);
        strncpy(dado.autores,autores,200);
        dado.ano = ano;
        strncpy(dado.path,path,64);
        ssize_t bytes_escritos = 0;

        bytes_escritos = write(fd,&dado,sizeof(Index));

        if (bytes_escritos != sizeof(Index)) {

            config->N--;
            index = -1;
            config->elementos--;

        }

        config->elementos++;
        close(fd);

    }
    
    return index;

}

/*
Consultar um documento
dclient -c "key"

    key : ID do documento

Exemplo de uso:
./dclient -c 1
Title: Romeo and Juliet
Authors: William Shakespeare
Year: 1997
Path: 1112.txt

*/

int tarefa_basedados_consultar(int key,long tempo, Index* item) {
    
    int fd = open(FICHEIRO_NOME,O_RDONLY);
    off_t offset = get_index_basedados(key);
    lseek(fd,offset,SEEK_SET);

    int index = -1;

    if (read(fd,item,sizeof(Index)) == sizeof(Index)) {

        if (item->tempo_logico <= tempo && item->valido == 1)
            index = key;

    }

    close(fd);
    return index;

}

/*
Remover um documento
dclient -d "key"

    key : ID do documento

Exemplo de uso:
./dclient -d 1
Index entry 1 deleted

*/

int tarefa_basedados_deletar(int key, long tempo) {
    
    int fd = open(FICHEIRO_NOME,O_RDWR);
    off_t offset = get_index_basedados(key);
    lseek(fd,offset,SEEK_SET);

    Index aux;
    int index = -1;

    if (read(fd,&aux,sizeof(Index)) == sizeof(Index)) {

        if (aux.valido == 1 && aux.tempo_logico <= tempo) {

            aux.valido = 0;
            aux.tempo_logico = tempo;

            off_t offset = get_index_basedados(key);
            lseek(fd,offset,SEEK_SET);
            write(fd,&aux,sizeof(Index));
            index = key;

        }

    }

    close(fd);
    return index;

}

/*
Número de linhas um documento que contêm uma dada palavra-chave
    Nota: usar grep e wc

dclient -l "key" "keyword"

    key : ID do documento
    keyword : palavra-chave

Exemplo de uso:
./dclient -l 1 "Romeo"
150

*/

int tarefa_numero_linhas(int key, char* keyword, long tempo) {

    int p1[2]; // comunicação grep -> wc
    int status = pipe(p1);

    if (status == -1) {
        perror("pipe");
    }

    int fd = open(FICHEIRO_NOME,O_RDWR);
    off_t offset = get_index_basedados(key);
    lseek(fd,offset,SEEK_SET);

    Index a;
    ssize_t bytes_lidos = read(fd,&a,sizeof(Index));
    close(fd);

    if (bytes_lidos == sizeof(Index)) {

        if (a.valido == 1 && a.tempo_logico <= tempo) {

            pid_t pid1 = fork(); 

            if (pid1 == 0) {
                close(p1[0]); // Fecha leitura
                dup2(p1[1], 1); // redirecionar o stdout (que foi escrito pelo ls) para o pipe de escrita
                close(p1[1]);
                int exec_ret = execlp("grep", "grep", keyword, a.path, NULL); // o -l é para o grep listar apenas os nomes dos ficheiros
                if (exec_ret == -1) {
                    perror("execlp grep");
                    _exit(-1);
                }
                _exit(1);
            }
             // processo pai
            close(p1[1]); // fecha a escrita

            int p2[2]; // pipe que vai fazer a comunicação de wc para o processo pai porque a função precisa de retornar algo e não imprimir no terminal
            int pipe_ret = pipe(p2);

            if (pipe_ret == -1) {
                perror("pipe 2");
            }

            pid_t pid2 = fork();

            if (pid2 == 0) {  // filho
                dup2(p1[0], 0); // ler da extremidade de leitura (ler o output do grep)
                close(p1[0]);
                
                close(p2[0]); // fechar a extremidade de leitura
                dup2(p2[1], 1);
                close(p2[1]);
                int exec_ret = execlp("wc", "wc", "-l", NULL); // o -l é para o grep listar apenas os nomes dos ficheiros
                if (exec_ret == -1) {
                    perror("execlp wc");
                    _exit(-1);
                }
                _exit(1);
            }

            // processo pai
            close(p1[0]);
            close(p2[1]);

            char buffer[64];
            for(int i = 0; i < 2; i++){ // esperar os dois filhos
                wait(NULL);
            }

            int n = read(p2[0], buffer, sizeof(buffer));
            buffer[n] = '\0';
            close(p2[0]);

            return atoi(buffer);
        }
    
    }

    close(p1[0]);
    close(p1[1]);
    return -1;
}

//Função que trata do número de linhas
void processo_numero_linhas(int fd_canal ,int key, char* keyword, long tempo, PedidoBaseDados pedido) {


    mensagem Resposta;
    Resposta.cmd = PEDIDO_LINES;
    Resposta.index = pedido.index;
    Resposta.comesFromServer = true;
    Resposta.max_fragmentos = 1;
    Resposta.n_fragmentos = 1;
    Resposta.pid = pedido.ID_pedido;
    strncpy(Resposta.string1,keyword,200);

    int numero_linhas = tarefa_numero_linhas(pedido.index,pedido.titulo,tempo);
    Resposta.inteiro = numero_linhas;

    write(fd_canal,&Resposta,sizeof(mensagem));
    close(fd_canal);

    _exit(0);

}

/*
Pesquisa de documentos que contêm uma dada palavra-chave deve ser efetuada concorrentemente por vários processos
    Nota: podemos usar grep
    
dclient -s "keyword" [nr_processes]

    keyword : palavra-chave
    nr_processes : número de processos para esta tarefa
                 | Se o argumento não existir, o nr_processes = 1

Exemplo de uso:
./dclient -s "praia" 5
[2,3,1438]

*/

GArray* tarefa_lista_documentos(char* keyword,long tempo, int start, int end)
{
    GArray* resultado = g_array_new(FALSE, FALSE, sizeof(int));

    if (start <= end) {

        for (int i = start; i < end; i++) {
            int linhas = 0;
            linhas = tarefa_numero_linhas(i, keyword,tempo);
            if ( linhas > 0) {
                g_array_append_val(resultado, i);
            }
        } 

    }

    return resultado;
}

//Função que trata da lista de índices com uma certa keyword - Worker
void processo_worker_lista_indices(int fd_canal,int max_fragmentos, char* keyword, long tempo,int min, int max, PedidoBaseDados pedido) {

    mensagem Resposta;
    Resposta.cmd = PEDIDO_LIST;
    Resposta.comesFromServer = true;
    Resposta.max_fragmentos = max_fragmentos;
    Resposta.n_fragmentos = 1;
    Resposta.pid = pedido.ID_pedido;
    strncpy(Resposta.string1,keyword,200);

    GArray* lista = tarefa_lista_documentos(keyword,tempo,min,max);

    int i_lista = 0;
    int i = 0;
    int numero_ints_por_pacote = 0;
    int n_fragmentos = 0;
    int quantidade_ints_string2 = 200 / sizeof(int);

    while (i_lista < lista->len) {

        numero_ints_por_pacote = 0;

        //Meter números na string1
        i = 0;
        while (i < quantidade_ints_string2 && i_lista < lista->len) {

            int* lista_ints = (int*) Resposta.string2;
            lista_ints[i] = g_array_index(lista,int,i_lista);

            i_lista++;
            numero_ints_por_pacote++;
            i++;

        }

        //Enviar mensagem
        Resposta.inteiro = numero_ints_por_pacote;
        write(fd_canal,&Resposta,sizeof(mensagem));

        n_fragmentos++;

    }

    g_array_free(lista,true);
    close(fd_canal);

    _exit(n_fragmentos);

}

//Função que trata da lista de índices com uma certa keyword - Controller
void processo_controller_lista_indices(int fd_canal ,int N_indices,int nr_processos, char* keyword, long tempo, PedidoBaseDados pedido) {

    mensagem Resposta;
    Resposta.cmd = PEDIDO_LIST;
    Resposta.comesFromServer = true;
    Resposta.pid = pedido.ID_pedido;
    strncpy(Resposta.string1,keyword,200);

    int max_fragmentos = N_indices;
    int n_fragmentos = max_fragmentos;

    //Criação dos processos
    for (int i = 0 ; i < nr_processos ; i++) {

        int gap = (nr_processos > 1) ? (N_indices / (nr_processos - 1)) : N_indices; // -1 para garantir que é lido a mais do que a menos

        if (fork() == 0) {
            processo_worker_lista_indices(fd_canal,max_fragmentos,keyword,tempo,i*gap,(i+1)*gap,pedido);
        }

    }

    //Espera dos processos
    for (int i = 0 ; i < nr_processos ; i++) {

        int status_filho;
        wait(&status_filho);
    
        if (WIFEXITED(status_filho))
            n_fragmentos -= WEXITSTATUS(status_filho);

    }

    //Enviar a resposta final

    Resposta.inteiro = 0;
    Resposta.n_fragmentos = n_fragmentos;
    Resposta.max_fragmentos = max_fragmentos;

    write(fd_canal,&Resposta,sizeof(mensagem));
    close(fd_canal);

    _exit(0);

}


/*
Parar servidor
dclient -f

Exemplo de uso:
./dclient -f
Server is shutting down

*/

//Iniciar ficheiro e carregar as configurações
DataBaseConfig* iniciarFicheiro() {

    int fd = open(FICHEIRO_NOME,O_RDWR | O_CREAT, 0777);

    if (fd== -1)
        return NULL;

    const int tamanho_buffer = 100;
    Index* buffer = malloc(sizeof(Index) * tamanho_buffer);
    
    if (buffer == NULL) {
        close(fd);
        return NULL;
    }
    
    DataBaseConfig* config = malloc(sizeof(DataBaseConfig));
    if (config == NULL) {
        close(fd);
        free(buffer);
        return NULL;
    }

    ssize_t bytes_lidos = 0;
    GQueue* indices_reciclados = g_queue_new();
    int numero_indices = 0;
    int indice_max = 0;

    while ( (bytes_lidos = read(fd,buffer,sizeof(Index) * tamanho_buffer)) > 0) {

        int N = bytes_lidos / sizeof(Index);
        for (int i = 0 ; i < N ; i++) {

            buffer[i].tempo_logico = 0;

            if (buffer[i].valido == 0) {
                g_queue_push_head(indices_reciclados,GINT_TO_POINTER(indice_max));
            }
            else {
                numero_indices++;
            }
            
            indice_max++;

        }

        lseek(fd,-bytes_lidos,SEEK_CUR);
        write(fd,buffer,bytes_lidos);

    }

    config->elementos = numero_indices;
    config->N = indice_max;
    config->indexes_eliminados = indices_reciclados;

    free(buffer);
    close(fd);

    return config;

}

//Processo que gere a Base de Dados
void run_database(int pipe[2],DataBaseConfig* config) {

    int fd_canal = open(PIPE_CANAL,O_WRONLY);

    long tempo_logico = 1;
    ssize_t bytes_lidos = 0;
    PedidoBaseDados pedido;    
    mensagem Resposta;
    int resposta_valida = 0;
    
    while((bytes_lidos = read(pipe[0],&pedido,sizeof(PedidoBaseDados))) > 0 ) {

        //Inicializar resposta
        Resposta.cmd = PEDIDO_INVALIDO;
        Resposta.comesFromServer = true;
        Resposta.max_fragmentos = 1;
        Resposta.n_fragmentos = 1;
        Resposta.pid = pedido.ID_pedido;

        //Fechar BD
        if (pedido.tipo == TIPO_PEDIDO_BD_FECHAR) {

            Resposta.cmd = PEDIDO_SHUTDOWN;
            resposta_valida = 1;
            break;

        }
        else {

            switch (pedido.tipo) {

                case TIPO_PEDIDO_BD_INDEXAR: {

                    int insert_novo_index = tarefa_basedados_indexar(config,tempo_logico,pedido.titulo,pedido.autores,pedido.year,pedido.path);

                    Resposta.cmd = PEDIDO_INSERT;
                    strncpy(Resposta.string1,pedido.titulo,200);
                    strncpy(Resposta.string2,pedido.autores,200);
                    Resposta.inteiro = pedido.year;
                    strncpy(Resposta.string3,pedido.path,64);
                    Resposta.index2 = insert_novo_index;
                    resposta_valida = 1;

                
                break;
                }

                case TIPO_PEDIDO_BD_CONSULTAR: {

                    Index index_coletado;
                    int get_index = tarefa_basedados_consultar(pedido.index,tempo_logico,&index_coletado);

                    Resposta.cmd = PEDIDO_GET;
                    Resposta.index = pedido.index;
                    strncpy(Resposta.string1,index_coletado.titulo,200);
                    strncpy(Resposta.string2,index_coletado.autores,200);
                    Resposta.inteiro = index_coletado.ano;
                    strncpy(Resposta.string3,index_coletado.path,64);
                    Resposta.index2 = get_index;
                    resposta_valida = 1;

                break;
                }

                case TIPO_PEDIDO_BD_DELETAR:{

                    int delete_index = tarefa_basedados_deletar(pedido.index,tempo_logico);

                    Resposta.cmd = PEDIDO_DELETE;
                    Resposta.index = pedido.index;
                    Resposta.index2 = delete_index;
                    resposta_valida = 1;

                break;
                }

                case TIPO_PEDIDO_BD_N_LINHAS:{

                    pid_t pid_processo_numero_linhas = fork();

                    //Processo para o número de linhas
                    if (pid_processo_numero_linhas == 0) {
                        close(pipe[0]);
                        processo_numero_linhas(fd_canal,pedido.index,pedido.titulo,tempo_logico,pedido);
                    }
                
                break;
                }
                case TIPO_PEDIDO_BD_LISTAR: {

                    pid_t pid_processo_lista_indices = fork();

                    //Processo para a lista de indices
                    if (pid_processo_lista_indices == 0) {
                        close(pipe[0]);
                        processo_controller_lista_indices(fd_canal,config->N,pedido.nr_processos,pedido.titulo,tempo_logico,pedido);
                    }
                
                break;
                }

                default:
                break;

            }

            //Escrever mensagem para o servidor
            if (resposta_valida == 1)
                write(fd_canal,&Resposta,sizeof(mensagem));

        }
        
        tempo_logico++;

    }

    //Fechar servidor
    write(fd_canal,&Resposta,sizeof(mensagem)); //Enviar mensagem
    close(fd_canal);

    return;

}