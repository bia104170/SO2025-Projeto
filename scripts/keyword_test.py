import os
import sys

def ficheiros_com_keyword(diretorio, keyword):
    
    for root, _, files in os.walk(diretorio):
        
        for nome_ficheiro in files:
            caminho_ficheiro = os.path.join(root, nome_ficheiro)

            try:
                with open(caminho_ficheiro, 'r', encoding='utf-8', errors='ignore') as f:
                    
                    for linha in f:
                        
                        if keyword in linha:
                            print(caminho_ficheiro)
                            break
                        
            except Exception as e:
                print(f"Erro ao ler {caminho_ficheiro}: {e}")

ficheiros_com_keyword("./docs/", sys.argv[1])
