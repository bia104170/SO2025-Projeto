#ifndef DATABASE
#define DATABASE

#include "servidor_config.h"

typedef struct basedados_index {
    int valido;
    long tempo_logico;
    char titulo[200];
    char autores[200];
    int ano;
    char path[64];
} Index;

typedef enum TIPO_PEDIDO_DB {TIPO_PEDIDO_BD_INDEXAR, TIPO_PEDIDO_BD_CONSULTAR, TIPO_PEDIDO_BD_DELETAR, TIPO_PEDIDO_BD_N_LINHAS, TIPO_PEDIDO_BD_LISTAR, TIPO_PEDIDO_BD_FECHAR} TipoPedidoBaseDados;
typedef struct pedido_basedados {

    TipoPedidoBaseDados tipo;
    char titulo[200];
    char autores[200];
    int year;
    char path[64];
    int index;
    int nr_processos;
    int ID_pedido;

} PedidoBaseDados;

typedef struct database_config {

    int elementos;
    int N;
    GQueue* indexes_eliminados;

} DataBaseConfig;

void lerFicheiroDatabase(int fich);
DataBaseConfig* iniciarFicheiro();
void run_database(int pipe[2],DataBaseConfig* config);

#endif