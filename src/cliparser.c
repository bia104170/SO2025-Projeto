#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "cliparser.h"
#include "dclient.h"
#include "ctype.h"

//Verifica se uma string é um número
int isNumber(const char* s) {

    for (int i = 0 ; i < strlen(s) ; i++) {
        if (isdigit(s[i]) == 0)
            return 0;
    }

    return 1;

}

char validarInput(char** tokens, int numero_tokens) {

    //Numero de tokens
    if (numero_tokens < 2) {
        printf("[ERRO] Número de argumentos insuficientes\n[USAGE] : ./dclient -x\n");
        return 'z';
    }

    //Instrução sempre "-X"
    if (strlen(tokens[1]) != 2) {
        printf("[ERRO] Comando inválido. Experimente estes:\n\n\t-a : Indexar documento\n\t-c : Consultar documento\n\t-d : Apagar indexação de documento\n\t-l : Número de vezes uma keyword aparece num documento\n\t-s : Lista de documentos que contêm keywords\n\t-f : Fechar servidor\n\n");
        return 'z';
    }

    //Add
    if (strcmp(tokens[1],"-a") == 0) {

        if (numero_tokens < 6) {
            printf("[ERRO] Argumentos em falta\n[USAGE] ./dclient -a <title> <authors> <year> <file_path>\n");
            return 'z';
        }

        if (strlen(tokens[2]) < 200){
        
            if (strlen(tokens[3]) < 200){
        
                if (isNumber(tokens[4]) == 1){
        
                    if (strlen(tokens[5]) < 64){
        
                        int fd = open(tokens[5],O_RDONLY);

                        if (fd >= 0) {

                            close(fd);
                            return 'a';

                        }
                        else {
                            printf("[ERRO] File tem que existir e ser acessivel\n[USAGE] ./dclient -a <title> <authors> <year> <file_path>\n");
                            return 'z';
                        }

                    }
                    else {
                        printf("[ERRO] File Path muito longo\n[USAGE] ./dclient -a <title> <authors> <year> <file_path>\n");
                        return 'z';
                    }

                }
                else {
                    printf("[ERRO] Ano tem que ser um número\n[USAGE] ./dclient -a <title> <authors> <year> <file_path>\n");
                    return 'z';
                }

            }
            else {
                printf("[ERRO] Lista de Autores muito longa\n[USAGE] ./dclient -a <title> <authors> <year> <file_path>\n");
                return 'z';
            }

        }
        else {
            printf("[ERRO] Titulo muito longo\n[USAGE] ./dclient -a <title> <authors> <year> <file_path>\n");
            return 'z';
        }

    }
    //Consultar
    else if (strcmp(tokens[1],"-c") == 0) {

        if (isNumber(tokens[2]) == 1) {
            return 'c';
        }
        else {
            printf("[ERRO] Esperado um índice\n[USAGE] ./dclient -c <document_index>\n");
            return 'z';
        }

    }
    //Apagar
    else if (strcmp(tokens[1],"-d") == 0) {

        if (isNumber(tokens[2]) == 1) {
            return 'd';
        }
        else {
            printf("[ERRO] Esperado um índice\n[USAGE] ./dclient -d <document_index>\n");
            return 'z';
        }

    }
    //Número de linhas
    else if (strcmp(tokens[1],"-l") == 0) {

        if (numero_tokens >= 4) {

            if (isNumber(tokens[2]) == 1) {
                
                if (strlen(tokens[3]) < 200){
                    return 'l';
                }
                else {

                    printf("[ERRO] Keyword muito longa\n[USAGE] ./dclient -l <document_index> <keyword>\n");
                    return 'z';

                }

            }
            else {
                printf("[ERRO] Esperado um índice\n[USAGE] ./dclient -l <document_index> <keyword>\n");
                return 'z';
            }

        } 
        else {
            printf("[ERRO] Argumentos em falta\n[USAGE] ./dclient -l <document_index> <keyword>\n");
            return 'z';
        }

    }
    //Lista de documentos
    else if (strcmp(tokens[1],"-s") == 0) {

        if (strlen(tokens[2]) < 200){
            
            if (numero_tokens >= 4) {

                if (isNumber(tokens[3]) == 1) {
                    return 's';
                }
                else {
                    printf("[ERRO] Esperado um número\n[USAGE] ./dclient -s <keyword> [nr_processes]\n");
                    return 'z';
                }

            }
            else {
                return 's';
            }


        }
        else {
            printf("[ERRO] Keyword muito longa\n[USAGE] ./dclient -s <keyword> [nr_processes]\n");
            return 'z';
        }

    }
    //Fechar servidor
    else if (strcmp(tokens[1],"-f") == 0) {
        return 'f';
    }
    //Inválido
    else {
        printf("[ERRO] Comando Desconhecido. Experimente estes:\n\n\t-a : Indexar documento\n\t-c : Consultar documento\n\t-d : Apagar indexação de documento\n\t-l : Número de vezes uma keyword aparece num documento\n\t-s : Lista de documentos que contêm keywords\n\t-f : Fechar servidor\n\n");
        return 'z';
    }

}

// testei e passou em tudo, mas não há correção/ verificação de erros de aspas
void parseInput(char **tokens,int numero_tokens, mensagem *msg, int* mensagem_valida) {

    //Verificação do
    char instrucao = validarInput(tokens,numero_tokens);

    if (instrucao == 'z') {
        *mensagem_valida = 0;
        return;
    }

    //Inicializar a mensagem
    msg->cmd = PEDIDO_INVALIDO;
    msg->comesFromServer = false;
    msg->index = 0;
    msg->index2 = 0;
    msg->max_fragmentos = 1;
    msg->n_fragmentos = 1;
    msg->inteiro = 0;
    msg->pid = getpid();
    msg->string1[0] = '\0';
    msg->string2[0] = '\0';
    msg->string3[0] = '\0';

    *mensagem_valida = 1;

    switch (instrucao) {

        case 'a':
            msg->cmd = PEDIDO_INSERT;
            strncpy(msg->string1,tokens[2],sizeof(msg->string1));
            strncpy(msg->string2,tokens[3],sizeof(msg->string2));
            msg->inteiro = atoi(tokens[4]);
            strncpy(msg->string3,tokens[5],sizeof(msg->string3));
            break;

        case 'c':
            msg->cmd = PEDIDO_GET;
            msg->index = atoi(tokens[2]);
            break;

        case 'd':
            msg->cmd = PEDIDO_DELETE;
            msg->index = atoi(tokens[2]);
            break;

        case 'l':
            msg->cmd = PEDIDO_LINES;
            msg->index = atoi(tokens[2]);
            strncpy(msg->string1,tokens[3],sizeof(msg->string1));
            break;

        case 's':
            msg->cmd = PEDIDO_LIST;
            msg->inteiro = 1;
            strncpy(msg->string1,tokens[2],sizeof(msg->string1));
            
            if (numero_tokens > 3) {
                msg->inteiro = atoi(tokens[3]);
            }
            break;

        case 'f':
            msg->cmd = PEDIDO_SHUTDOWN;
            break;

        default:
            msg->cmd = PEDIDO_INVALIDO;
            *mensagem_valida = 0;
            break;
    }
}
