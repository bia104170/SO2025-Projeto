#ifndef SERVIDOR_CONFIG
#define SERVIDOR_CONFIG

#define PIPE_CANAL "pipes/canal.pipe"
#define FICHEIRO_NOME "database/array"

#define FIFO_PATH_MAX 128
#define MENSAGEM_MAX 380

#include <glib.h>
#include <stdbool.h>

typedef enum TIPO_PEDIDO {PEDIDO_INVALIDO, PEDIDO_INSERT, PEDIDO_GET, PEDIDO_DELETE, PEDIDO_LINES, PEDIDO_LIST, PEDIDO_SHUTDOWN} TipoPedido;

// cliente vai enviar mensagens nesta estrutura para o servidor
typedef struct msg {
    TipoPedido cmd;
    
    char string1[200]; // titulo || palavra para a pesquisa
    char string2[200]; // autores
    char string3[64]; // path
    int inteiro; // ano || nº de processos || index do doc
    pid_t pid;
    int index;
    int index2;
    int n_fragmentos;
    int max_fragmentos;
    bool comesFromServer;

} mensagem;

typedef struct resposta{ // no máximo 512 bytes
    char string1[200]; // titulo || palavra para a pesquisa
    char string2[200]; // autores
    char string3[64]; // path
    int inteiro; // ano ou numero de linhas
    int index;
    pid_t pid;
    int n_fragmentos;
    int max_fragmentos;
    TipoPedido tipo;  
} Resposta;

#endif