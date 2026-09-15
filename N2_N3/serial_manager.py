# serial_manager.py
import serial
import time

class GerenciadorSerial:
    def __init__(self, porta, tamanho_pacote=20, baudrate=115200):
        self.porta = porta
        self.baudrate = baudrate
        self.tamanho_pacote = tamanho_pacote
        self.timeout_serial = 0.5
        self.ser = None

    # ==================== Métodos Específicos da Serial ====================
    def resetar_esp32(self):
        """Realiza a rotina de reset do ESP32 via pinos DTR e RTS."""
        if not self.ser or not self.ser.is_open:
            return

        print("[Serial] Iniciando rotina de reset do ESP32...")
        self.ser.setDTR(False)
        self.ser.setRTS(False)
        time.sleep(0.1)
        self.ser.setDTR(True)
        self.ser.setRTS(True)
        time.sleep(1.5)  # Tempo de estabilização pós-reset
        self.limpar_buffers()
        print("[Serial] ESP32 resetado e pronto.")

    def limpar_buffers(self):
        """Garante que não há lixo antigo nas filas de I/O da serial."""
        if self.ser and self.ser.is_open:
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()

    # ==================== Métodos Universais ====================
    def conectar(self):
        """Abre a porta COM do sistema operacional."""
        print(f"[Serial] Conectando à porta {self.porta} a {self.baudrate} bps...")
        self.ser = serial.Serial(self.porta, self.baudrate, timeout=self.timeout_serial, parity=serial.PARITY_NONE)

    def enviar_pacote(self, pacote, timeout=2):
        """Envia o pacote convertido para bytearray (muito mais rápido) e limpa os buffers."""
        self.limpar_buffers()
        self.ser.write(bytearray(pacote))
        self.ser.flush()
        return True

    def receber_pacote(self, timeout):
        """
        Lê a serial até atingir exatamente o tamanho do pacote esperado ou estourar o timeout.
        Retorna uma lista de inteiros ou None caso perca o pacote.
        """
        pacote_ul = b''
        tempo_inicial_rx = time.time()

        # Continua lendo a serial enquanto não atingir os 20 bytes ou o tempo limite
        while len(pacote_ul) < self.tamanho_pacote and (time.time() - tempo_inicial_rx) < timeout:
            bytes_faltantes = self.tamanho_pacote - len(pacote_ul)
            pacote_ul += self.ser.read(bytes_faltantes)

        # Se leu tudo com sucesso
        if len(pacote_ul) == self.tamanho_pacote:
            return list(pacote_ul) 
        
        # Se deu timeout e não leu os bytes suficientes
        else:
            self.limpar_buffers()
            return None

    def desconectar(self):
        """Libera a porta serial do sistema operacional."""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("[Serial] Porta serial fechada com sucesso.")