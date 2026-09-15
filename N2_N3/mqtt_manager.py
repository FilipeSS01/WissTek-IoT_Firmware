# mqtt_manager.py
import paho.mqtt.client as mqtt
from paho.mqtt.client import CallbackAPIVersion
import threading

class GerenciadorMQTT:
    def __init__(self, broker, porta, topic_dl, topic_ul, qos=1, tamanho_pacote=20):
        self.broker = broker
        self.porta = porta
        self.topic_dl = topic_dl
        self.topic_ul = topic_ul
        self.qos = qos
        self.tamanho_pacote = tamanho_pacote
        
        self.pacote_ul_status = threading.Event()
        self.pacote_ul_payload = bytearray(self.tamanho_pacote)

        self.client = mqtt.Client(CallbackAPIVersion.VERSION2)
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        self.client.on_publish = self.on_publish

    # ==================== Callbacks MQTT ====================
    def on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code == 0:
            print("[MQTT] Conectado ao Broker MQTT com sucesso.")
            client.subscribe(self.topic_ul, qos=self.qos)
            print(f"[MQTT] Inscrito no tópico: {self.topic_ul} (QoS{self.qos})")
        else:
            print(f"[MQTT] Falha na conexão. Código: {reason_code}")

    def on_publish(self, client, userdata, mid, reason_code, properties):
        pass

    def on_message(self, client, userdata, msg):
        payload = msg.payload
        if len(payload) >= self.tamanho_pacote:
            # Salva o pacote e aciona o evento (liberando o loop principal)
            self.pacote_ul_payload = bytearray(payload[:self.tamanho_pacote])
            self.pacote_ul_status.set()

    def on_disconnect(self, client, userdata, flags, reason_code, properties):
        if reason_code != 0:
            print(f"\n[MQTT] Desconectado inesperadamente (rc={reason_code}).")

    # ==================== Métodos Universais ====================
    def conectar(self):
        """Conecta ao broker e inicia a thread em background."""
        print(f"[MQTT] Conectando ao broker {self.broker} - porta {self.porta}...")
        self.client.connect(self.broker, self.porta, keepalive=60)
        self.client.loop_start()

    def enviar_pacote(self, pacote, timeout=2):
        """Publica o pacote (Downlink) e aguarda confirmação de entrega pelo broker."""
        # Limpa a flag de recepção para garantir que não leremos lixo antigo
        self.pacote_ul_status.clear() 
        
        if self.client.is_connected():
            result = self.client.publish(self.topic_dl, bytes(pacote), qos=self.qos)
            try:
                result.wait_for_publish(timeout=timeout)
                return True
            except Exception as e:
                print(f"[MQTT Erro] Falha ao aguardar publicação: {e}")
                return False
        else:
            print("[MQTT] Não foi possível publicar. Cliente desconectado.")
            return False

    def receber_pacote(self, timeout):
        """Trava a execução aguardando o Uplink. Retorna uma lista de inteiros ou None se estourar o tempo."""
        chegou = self.pacote_ul_status.wait(timeout=timeout)
        if chegou:
            return list(self.pacote_ul_payload)
        return None

    def desconectar(self):
        """Para a thread e fecha a conexão com o broker de forma limpa."""
        self.client.loop_stop()
        self.client.disconnect()
        print("[MQTT] Desconectado do broker com sucesso.")