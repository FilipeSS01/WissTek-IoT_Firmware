# FEE247 - Desenvolvimento de Soluções IoT com LoRa e LoRaWAN
#======= Nível 5 - Gerência ============
# Extrai RSSI dos dados brutos e calcula a PSR
import time
import os

# Arquivos utilizados pelo nível 5 de gerência
arquivo_rssi = "N4/Dados_Processados/rssi.tmp"
arquivo_psr = "N4/Dados_Processados/psr.tmp"

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

    # Listas que guardam os valores calculados
    rssi_down = []
    rssi_up = []
    psr = []

    # Contadores usados para calcular a PSR
    total_pacotes = 0
    pacotes_recebidos = 0

    # Começa em 1 para pular o cabeçalho do arquivo
    for i in range(1,len(linhas)):
        partes = linhas[i].split(",")
        total_pacotes = total_pacotes + 1

        # Se todos os bytes do UL forem diferente de 9, considera pacote recebido
        pacote_recebido = 0
        for j in range(20):
            if int(partes[22+j]) != 9:
                pacote_recebido = 1

        if pacote_recebido == 1:
            pacotes_recebidos = pacotes_recebidos + 1

            # RSSI de downlink no byte UL_B0 (Posição 22)
            UL_B0 = int(partes[22])
            if UL_B0 > 128:
                RSSI_DL = ((UL_B0-256)/2.0)-74
            else:
                RSSI_DL = (UL_B0/2.0)-74

            # RSSI de uplink no byte UL_B2 (Posição 24)
            UL_B2 = int(partes[24])
            if UL_B2 > 128:
                RSSI_UL = ((UL_B2-256)/2.0)-74
            else:
                RSSI_UL = (UL_B2/2.0)-74

            rssi_down.append(RSSI_DL)
            rssi_up.append(RSSI_UL)

        # Calcula a PSR acumulada até este pacote
        PSR = (pacotes_recebidos/total_pacotes)*100
        psr.append(PSR)

    # Grava as RSSIs em arquivo temporário
    f_rssi = open(arquivo_rssi,"w")
    for i in range(len(rssi_down)):
        print(rssi_down[i],rssi_up[i],file=f_rssi)
    f_rssi.close()

    # Grava a PSR em arquivo temporário
    f_psr = open(arquivo_psr,"w")
    for i in range(len(psr)):
        print(psr[i],file=f_psr)
    f_psr.close()

    print("Arquivo = ",arquivo_entrada," | Pacotes = ",total_pacotes," | Recebidos = ",pacotes_recebidos," | PSR = ",PSR, "| RSSI Downlink = ", RSSI_DL, "| RSSI Uplink = ", RSSI_UL)
    time.sleep(1)
