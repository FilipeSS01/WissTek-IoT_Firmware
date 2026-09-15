# CURSO FEE247 Turma 2
# EXP8 com MAC centralizada
# ========================== 1 - Bibliotecas
import serial
import math
import time
import struct
from time import localtime, strftime
import os
#Tamanho_pacote = input("Tamanho do pacote em bytes = ")
Tamanho_pacote = 20
# ========================= 2 - Variáveis e arquivos
# Cria os pacotes de DL e UL
Pacote_DL =[0]*Tamanho_pacote
PacoteUL=[0]*Tamanho_pacote
# Garante que os pacotes de DL e UL estão com valor 0
for i in range(Tamanho_pacote):
   Pacote_DL[i] = 0
   PacoteUL[i] = 0
# ================== 3 - Arquivos criados para o Nível 4 Armazenamento
# Para gravar arquivo de Log.
#Grava_log = input('0 para não gravar e 1 para gravar = ')
Grava_log = 1
# Arquivos de armazenamento de logs que devem ser guardados
# em todas as rodadas de medidas
if Grava_log == 1:
   # Cria as pastas do Nível 4, caso elas não existam
   if not os.path.exists("N4"):
      os.mkdir("N4")
   if not os.path.exists("N4/Dados_Brutos"):
      os.mkdir("N4/Dados_Brutos")
   if not os.path.exists("N4/Parametros"):
      os.mkdir("N4/Parametros")
   filename1 = strftime("N4/Dados_Brutos/Rodada_Teste_%Y_%m_%d_%H-%M-%S.txt")
   Log_dados = open(filename1, 'w')
   print("Arquivo de log: %s" % filename1)
   Cabecalho = 'Time stamp,Contador,DL_B0,DL_B1,DL_B2,DL_B3,DL_B4,DL_B5,DL_B6,DL_B7,DL_B8,DL_B9,DL_B10,DL_B11,DL_B12,DL_B13,DL_B14,DL_B15,DL_B16,DL_B17,DL_B18,DL_B19,UL_B0,UL_B1,UL_B2,UL_B3,UL_B4,UL_B5,UL_B6,UL_B7,UL_B8,UL_B9,UL_B10,UL_B11,UL_B12,UL_B13,UL_B14,UL_B15,UL_B16,UL_B17,UL_B18,UL_B19'
   print(Cabecalho,file=Log_dados)
   
# ============= INICIALIZAÇÃO
# Abre a porta serial
# Configura a serial
n_serial = input("Digite o número da serial = ")
n_serial1 = int(n_serial) - 1
ser = serial.Serial("COM"+str(n_serial),115200,timeout=0.5,parity=serial.PARITY_NONE)

# --- INÍCIO DA ROTINA DE RESET DO ESP32 ---
ser.setDTR(False)
ser.setRTS(False)
time.sleep(0.1)
ser.setDTR(True)
ser.setRTS(True)
time.sleep(1.5) # Tempo de estabilização de 1,5 segundos

# Limpa buffers
ser.reset_input_buffer()
ser.reset_output_buffer()
# =============== Camada de aplicação DL
Comando_LED_amarelo = 0  # Inicia apagado
# ================ Camada de Transporte DL
Contador_pkt_DL = 0
perda_PK_RX = 0
# ================ Camada de Rede DL
Grava = 0
#ID_sensor = input("Identificação do sensor = ")
ID_sensor = 1
#ID_gateway = input ("Identificação do gateway =")
ID_gateway = 0
# ================ Camada MAC DL
#Tempo_entre_pacotes = input("Tempo entre pacotes (s) =")
Tempo_entre_pacotes = 2
num_medidas = input('Entre com o número de medidas = ')
Variavel_loop = int(num_medidas) + 1
# ================ Envio de pacote de DL
try:
   for j in range(1, int(Variavel_loop)):
# ===================== LOOP DE ENVIO DE PACOTES =============
      Tempo_inicio_pacote = time.time()
      try:

      # ======== Camada de aplicação PACOTE DL
      # Lê o arquivo cmd_led_amarelo.txt
         with open("N4/Parametros/cmd_led_amarelo.txt", "r") as f:
            linha = f.readline()
            # Remove espaços e ENTER
            linha = linha.strip()
            # Se o valor for 0 ou 1
            if linha == "0":
               Comando_LED_amarelo = 0
            elif linha == "1":
               Comando_LED_amarelo = 1
            else:
               # Qualquer outro conteúdo assume 0
               Comando_LED_amarelo = 0
      except:
         # Se houver qualquer erro assume 0
         Comando_LED_amarelo = 0
      # Coloca o comando no byte 16 do DL
      Pacote_DL[16] = Comando_LED_amarelo
      # ======== Camada de transporte DL
      Contador_pkt_DL = Contador_pkt_DL + 1
      if Contador_pkt_DL == 256:
         Contador_pkt_DL = 0      
      Pacote_DL[12] = int(Contador_pkt_DL)
      # ======== Camada de rede DL
      Pacote_DL[8] = ID_sensor
      Pacote_DL[9] = ID_gateway
      # ======== Camada MAC de DL
      Pacote_DL[4] = Tempo_entre_pacotes
      # ======== Camada PHY de DL
      # Limpa o buffer de entrada antes de enviar um novo pacote
      ser.reset_input_buffer()
      # Envia pacote de DL via USB para ESP32
      for Bytes_DL in range(Tamanho_pacote):
         ser.write(chr(Pacote_DL[Bytes_DL]).encode('latin1'))
      ser.flush()
      # Aguarda um pouco para o gateway começar a responder
      time.sleep(0.05)
      # =========== Leitura do pacote UL recebido pela USB vindo do ESP32
      # ======= Camada física UL
      Pacote_UL = b''
      Tempo_limite_RX = Tempo_entre_pacotes - 0.2
      if Tempo_limite_RX < 0.5:
         Tempo_limite_RX = 0.5
         
      Tempo_inicial_RX = time.time()
      
      #Lê exatamente o tamanho do pacote, pelo tempo 
      #O pacote pode não estar disponível todo de uma vez no buffer da serial. Sendo assim, utilizamos um while para potencialmente fazer várias
      #leituras na serial, até atingir o tamanho máximo do Pacote (Tamanho_pacote)
      #Também estamos esperando até o Tempo_limite_RX para ler todo o pacote. O TEmpo_limite_RX sempre será menor do que o Tempo entre pacotes, mas 
      #até um limite de 0.5. Ele sempre será menor, mas não igual: isso é para dar uma folga de tempo para o próximo pacote.
      while len(Pacote_UL) < Tamanho_pacote and (time.time() - Tempo_inicial_RX) < Tempo_limite_RX:
         
         #Só realiza a leitura da serial da quantidade de bytes que ainda falta para completar o pacote (Tamanho_pacote - len(Pacote_UL)
         Pacote_UL = Pacote_UL + ser.read(Tamanho_pacote - len(Pacote_UL))
         
      if len(Pacote_UL) == Tamanho_pacote:
         print('Pacote = ',j,' | Pacote UL recebido | LED = ',Comando_LED_amarelo)
         Dados_DL = ''
         Dados_UL = ''
         #Prepara os dados dos pacotes de Downlink e Uplink para serem impressos
         for i in range(Tamanho_pacote):
            if i == 0:
               Dados_DL = str(Pacote_DL[i])
               Dados_UL = str(Pacote_UL[i])
            else:
               Dados_DL = Dados_DL + ', ' + str(Pacote_DL[i])
               Dados_UL = Dados_UL + ', ' + str(Pacote_UL[i])
         Tempo = time.asctime()
         print(Tempo + ', ' + str(j) + ', Downlink: ' + Dados_DL + ' Uplink: ' + Dados_UL)
         if Grava_log == 1:
            Dados_log = Tempo + ',' + str(j) + ',' + Dados_DL + ',' + Dados_UL
            print(Dados_log,file=Log_dados)
            Log_dados.flush()
      else:
         ser.reset_input_buffer()
         perda_PK_RX += 1
         print('Cont = ', j, ' PERDEU PACOTE ')
         Dados_DL = ''
         Dados_UL = ''
         for i in range(Tamanho_pacote):
            if i == 0:
               Dados_DL = str(Pacote_DL[i])
               Dados_UL = '9'
            else:
               Dados_DL = Dados_DL + ', ' + str(Pacote_DL[i])
               Dados_UL = Dados_UL + ', 9'
         Tempo = time.asctime()
         print(Tempo + ', ' + str(j) + ', Downlink: ' + Dados_DL + ' Uplink: ' + Dados_UL)
         if Grava_log == 1:
            Dados_log = Tempo + ',' + str(j) + ',' + Dados_DL + ',' + Dados_UL
            print(Dados_log,file=Log_dados)
            Log_dados.flush()
      Tempo_gasto = time.time() - Tempo_inicio_pacote
      if Tempo_gasto < Tempo_entre_pacotes:
         time.sleep(Tempo_entre_pacotes - Tempo_gasto)
   print('Pacotes enviados = ',j,' Pacotes perdidos = ',perda_PK_RX)
   ser.close()
   if Grava_log == 1:
      Log_dados.close()
   print('Fim da Execução')
except KeyboardInterrupt:
   ser.close()
   if Grava_log == 1:
      Log_dados.close()
