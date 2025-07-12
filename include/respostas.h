#ifndef SERVIDOR_RESPOSTAS
#define SERVIDOR_RESPOSTAS

#include "servidor_config.h"
#include "cache.h"

void processo_respostas(int pipe[2]);
char* criarPipeClienteNome(pid_t pid);
GArray* convertToResposta(CacheEntry* entry, mensagem pedido);

#endif