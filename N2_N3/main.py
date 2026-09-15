# main.py
import time
import os
from time import strftime
from serial_manager import GerenciadorSerial
from mqtt_manager import GerenciadorMQTT

# ========================= 1 - Configurações Gerais
Tamanho_pacote = 20
Grava_log = 1
Tempo_entre_pacotes = 2
ID_sensor = 1
ID_gateway = 0
Pacote_DL = [0] * Tamanho_pacote

# ========================= 2 - Seleção do Modo de Comunicação
print("========== Gateway LoRa ==========")
print("[1] Comunicação Direta (Serial/USB)")
print("[2] Comunicação em Nuvem (MQTT)")
modo_escolhido = input("Escolha o modo de operação (1 ou 2): ").strip()

conexao = None # Esta é a nossa variável universal

if modo_escolhido == "1":
    n_serial = input("Digite o número da serial (Ex: 3): ")
    porta_com = "COM" + str(n_serial)
    # Instancia o objeto Serial
    conexao = GerenciadorSerial(porta_com, tamanho_pacote=Tamanho_pacote)
    conexao.conectar()
    conexao.resetar_esp32()
    
elif modo_escolhido == "2":
    BROKER = "test.mosquitto.org"
    TOPIC_DL = "mot_lora_mqtt_FILIPE_SILVA/gateway/downlink"
    TOPIC_UL = "mot_lora_mqtt_FILIPE_SILVA/gateway/uplink"
    # Instancia o objeto MQTT
    conexao = GerenciadorMQTT(BROKER, porta=1883, topic_dl=TOPIC_DL, topic_ul=TOPIC_UL, qos=1, tamanho_pacote=Tamanho_pacote)
    conexao.conectar()
    time.sleep(2) # Aguarda broker
    
else:
    print("Modo inválido. Encerrando.")
    exit()

# ========================= 3 - Criação de Arquivos (Nível 4)
if Grava_log == 1:
    os.makedirs("../N4/Dados_Brutos", exist_ok=True)
    os.makedirs("../N4/Parametros", exist_ok=True)
    filename1 = strftime("../N4/Dados_Brutos/Rodada_Teste_%Y_%m_%d_%H-%M-%S.txt")
    Log_dados = open(filename1, 'w')
    Cabecalho = 'Time stamp,Contador,DL_B0... (todos os bytes) ...UL_B19'
    print(Cabecalho, file=Log_dados)

# ========================= 4 - Lógica do Experimento
num_medidas = int(input('\nEntre com o número de medidas = '))
Contador_pkt_DL = 0
perda_PK_RX = 0

try:
    for j in range(1, num_medidas + 1):
        Tempo_inicio_pacote = time.time()

        # ======== Aplicação DL (LED) ========
        Comando_LED_amarelo = 0
        try:
            with open("../N4/Parametros/cmd_led_amarelo.txt", "r") as f:
                Comando_LED_amarelo = 1 if f.readline().strip() == "1" else 0
        except FileNotFoundError:
            pass 

        Pacote_DL[16] = Comando_LED_amarelo
        
        # ======== Transporte e Rede DL ========
        Contador_pkt_DL = (Contador_pkt_DL + 1) % 256
        Pacote_DL[12] = Contador_pkt_DL
        Pacote_DL[8] = ID_sensor
        Pacote_DL[9] = ID_gateway
        Pacote_DL[4] = Tempo_entre_pacotes

        # ======== PHY DL (ENVIO UNIVERSAL) ========
        # Independentemente se é MQTT ou Serial, o comando é o mesmo:
        conexao.enviar_pacote(Pacote_DL, timeout=Tempo_entre_pacotes)
        
        time.sleep(0.05) # Folga

        # ======== PHY UL (RECEPÇÃO UNIVERSAL) ========
        Tempo_limite_RX = max(0.5, Tempo_entre_pacotes - 0.2)
        
        # Independentemente se é MQTT ou Serial, o comando é o mesmo:
        Pacote_UL = conexao.receber_pacote(timeout=Tempo_limite_RX)

        Tempo_str = time.asctime()
        Dados_DL_str = ', '.join(map(str, Pacote_DL))

        if Pacote_UL is not None:
            Dados_UL_str = ', '.join(map(str, Pacote_UL))
            print(f"Pacote = {j:03d} | {Dados_UL_str} | LED = {Comando_LED_amarelo}")
        else:
            perda_PK_RX += 1
            print(f"Cont = {j:03d} PERDEU PACOTE")
            Dados_UL_str = ', '.join(['9'] * Tamanho_pacote)

        # Grava os arquivos
        if Grava_log == 1:
            print(f"{Tempo_str},{j},{Dados_DL_str},{Dados_UL_str}", file=Log_dados)
            Log_dados.flush()

        # ======== Controle de Cadência ========
        Tempo_gasto = time.time() - Tempo_inicio_pacote
        if Tempo_gasto < Tempo_entre_pacotes:
            time.sleep(Tempo_entre_pacotes - Tempo_gasto)

    print(f"\nResumo: Pacotes = {num_medidas} | Perdidos = {perda_PK_RX}")

except KeyboardInterrupt:
    print("\n[Ctrl + C] Interrompido pelo usuário.")
finally:
    # Desconecta de forma universal (fecha a porta ou o broker)
    if conexao:
        conexao.desconectar()
    if Grava_log == 1:
        Log_dados.close()
    print('Fim da Execução')