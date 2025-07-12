import csv

catalogo = open('Gcatalog.tsv', 'r', encoding='utf-8') 
script = open('povoar_base_de_dados.sh', 'w', encoding='utf-8')

doc_file = input("Insira o path da pasta (ex: docs) : ")

reader = csv.reader(catalogo, delimiter='\t')
got_header = False

for linha in reader:
    
    if got_header is False:
        got_header = True
        continue
    
    if len(linha) != 4:
        continue
    
    filepath, titulo, ano, autores = linha

    cmd = f'./dclient -a "{titulo}" "{autores}" {ano} "{doc_file}/{filepath}"\n'
    script.write(cmd)

catalogo.close()
script.close()

print(f"Script gerado : 'povoar_base_de_dados.sh'")
