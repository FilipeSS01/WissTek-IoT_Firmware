# FEE247 - Desenvolvimento de Soluções IoT com LoRa e LoRaWAN
#======= Nível 5 - Aplicação ============
# Extrai a luminosidade dos dados brutos e calcula a média
import time
import os

# Arquivos utilizados pelo nível 5 de aplicação
arquivo_luminosidade = "N4/Dados_Processados/luminosidade.tmp"
arquivo_media = "N4/Dados_Processados/media_luminosidade.txt"

while True:

    # Procura o último arquivo de dados brutos gravado no nível 4
    arquivo_entrada = ""
    arquivos = os.listdir("N4/Dados_Brutos")
    arquivos.sort()
    for nome in arquivos:
        if nome.endswith(".txt"):
            arquivo_entrada = "N4/Dados_Brutos/" + nome

    # Abre o arquivo de dados brutos
    arquivo = open(arquivo_entrada,"r")
    linhas = arquivo.readlines()
    arquivo.close()

    # Lista que guarda as luminosidades calculadas
    luminosidades = []

    # Começa em 1 para pular o cabeçalho do arquivo
    for i in range(1,len(linhas)):
        partes = linhas[i].split(",")

        # No arquivo bruto, os bytes do uplink começam na posição 22
        # A luminosidade está nos bytes UL_B18 (40) e UL_B19 (41)
        UL_B18 = int(partes[40])
        UL_B19 = int(partes[41])

        # Reconstrói a luminosidade usando os dois bytes
        luminosidade = (UL_B18*256 + UL_B19)
        luminosidades.append(float(luminosidade))

    # Grava a luminosidade em arquivo temporário
    f_lum = open(arquivo_luminosidade,"w")
    for i in range(len(luminosidades)):
        print(luminosidades[i],file=f_lum)
    f_lum.close()

    # Calcula a média acumulada da luminosidade
    soma = 0.0
    f_media = open(arquivo_media,"w")
    for i in range(len(luminosidades)):
        soma = soma + luminosidades[i]
        media = soma / (i + 1)
        print(media,file=f_media)
    f_media.close()

    print("Arquivo = ",arquivo_entrada," | Luminosidade = ",luminosidades[-1]," | Amostras = ",len(luminosidades)," | Média = ",media)
    time.sleep(1)
