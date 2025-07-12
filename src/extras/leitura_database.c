#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "servidor_config.h"
#include "database.h"

#include <glib.h>

void lerFicheiroDatabase(int fich) {

    int fd = open(FICHEIRO_NOME,O_RDONLY);

    if (fd == -1)
        return;

    const int tamanho_buffer = 100;
    Index* buffer = malloc(sizeof(Index) * tamanho_buffer);
    char buf[1024];
    
    if (buffer == NULL) {
        close(fd);
        return;
    }

    ssize_t bytes_lidos = 0;
    int indice = 0;

    while ( (bytes_lidos = read(fd,buffer,sizeof(Index) * tamanho_buffer)) > 0) {

        int N = bytes_lidos / sizeof(Index);

        for (int i = 0 ; i < N ; i++) {

            Index index = buffer[i];
            sprintf(buf,"Documento %d :\n\tValido : %d\n\tTempo Lógico : %ld\n\tTitulo : %s\n\tAutores : %s\n\tAno : %d\n\tPath : %s\n\n",indice,index.valido,index.tempo_logico,index.titulo,index.autores,index.ano,index.path);
            write(fich,buf,strlen(buf));
            fsync(fich);

            indice++;

        }

    }

    free(buffer);
    close(fd);

}

//Ficheiro que lê a BD
int main() {

    int fd = open("bd_log.txt",O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if (fd == -1) {
        perror("Não foi possivel abrir o ficheiro");
        return 1;
    }

    while (1) {

        lseek(fd,0,SEEK_SET);
        lerFicheiroDatabase(fd);
        sleep(1);

    }

    return 0;

}