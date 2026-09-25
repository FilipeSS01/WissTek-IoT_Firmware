/*
  MoT LoRa Mesh - Gateway (Inundação Gerenciada)
  Versão: ESP32 + RFM95 (Kit PK-LoRa) com Transporte Wi-Fi / MQTT (QoS 1)
*/

//=======================================================================
//                     1 - Bibliotecas
//=======================================================================
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <MQTT.h>

//=======================================================================
//                     2 - Configurações Wi-Fi e MQTT
//=======================================================================
WiFiMulti wifiMulti;
WiFiClient wifiClient;

// Broker MQTT
const char* MQTT_BROKER = "test.mosquitto.org"; // ou "broker.hivemq.com"
const int   MQTT_PORT   = 1883;
const char* TOPIC_DL    = "mot_lora_mqtt_FILIPE_SILVA/gateway/downlink";  // Python -> Gateway
const char* TOPIC_UL    = "mot_lora_mqtt_FILIPE_SILVA/gateway/uplink";    // Gateway -> Python
String CLIENT_ID;

const int MQTT_QOS = 1;
MQTTClient mqttClient(256);

// Buffer de recepção do Downlink MQTT
volatile bool mqtt_dl_disponivel = false;
#define Tamanho_pacote 20
byte mqtt_dl_payload[Tamanho_pacote];

//=======================================================================
//                     3 - Variáveis e Mapeamento de Hardware
//=======================================================================

// ============= Pinagem na placa PK-LoRa (RFM95 com ESP32)
#define SCK_PIN    5
#define MISO_PIN  19
#define MOSI_PIN  27
#define NSS_PIN   18
#define RST_PIN   14
#define DIO0_PIN  26

// ============= Camada Física LoRa
#define FREQUENCY_IN_HZ       915E6
#define txPower               17
#define spreadingFactor       7
#define signalBandwidth       125E3
#define codingRateDenominator 8

// LEDs de sinalização
#define LED_VERMELHO_PIN 15
#define LED_VERDE_PIN     4

// Variáveis de medição de enlace
int RSSI_dBm_UL;
int RSSI_UL;
float SNR_UL;
int SNR_UL_inteiro;

// ============== Estrutura do Pacote
byte PacoteDL[Tamanho_pacote];
byte PacoteUL[Tamanho_pacote];

// ============== Parâmetros de Rede (Mesh)
int ID_gateway = 100;      // Identificador deste Gateway
#define HOP_LIMIT_PADRAO 3 // Quantidade inicial de saltos para Downlinks
byte seq_packet_gw = 0;    // Gerador sequencial de Packet_ID do Gateway

// Cache Circular de Duplicatas (Anti-Looping)
#define CACHE_SIZE 16
struct PacketHistory {
  byte src;
  byte id;
};
PacketHistory cacheDuplicatas[CACHE_SIZE];
byte cacheIndex = 0;

// Proclamação de funções do arquivo 1_PHY.ino
void mqtt_callback(MQTTClient *client, char topic[], char bytes[], int length);
void conectar_mqtt();
void Phy_mqtt_receive_DL();
void Phy_radio_receive_UL();

//=======================================================================
//                     4 - Setup de Inicialização
//=======================================================================
void setup() {
  Serial.begin(115200);
  delay(50);

  pinMode(LED_VERMELHO_PIN, OUTPUT);
  pinMode(LED_VERDE_PIN, OUTPUT);
  digitalWrite(LED_VERMELHO_PIN, LOW);
  digitalWrite(LED_VERDE_PIN, LOW);

  // --- Conexão Wi-Fi Multi ---
  wifiMulti.addAP("Silva 2.4G", "Silv@1234");

  Serial.print("Conectando ao Wi-Fi");
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // ID único para o MQTT
  CLIENT_ID = "esp32_gw_mesh_" + String(WiFi.macAddress());
  CLIENT_ID.replace(":", "");

  // --- Configuração MQTT ---
  mqttClient.begin(MQTT_BROKER, MQTT_PORT, wifiClient);
  mqttClient.onMessageAdvanced(mqtt_callback);
  conectar_mqtt();

  // --- Inicialização LoRa / SPI ---
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, NSS_PIN);
  LoRa.setPins(NSS_PIN, RST_PIN, DIO0_PIN);

  if (!LoRa.begin(FREQUENCY_IN_HZ)) {
    Serial.println("Erro ao iniciar LoRa no Gateway!");
    while (true) { delay(1000); }
  }

  LoRa.setTxPower(txPower);
  LoRa.setSpreadingFactor(spreadingFactor);
  LoRa.setSignalBandwidth(signalBandwidth);
  LoRa.setCodingRate4(codingRateDenominator);

  // Inicializa cache de duplicatas
  for (int i = 0; i < CACHE_SIZE; i++) {
    cacheDuplicatas[i].src = 0xFF;
    cacheDuplicatas[i].id  = 0xFF;
  }

  Serial.println("--- Gateway LoRa Mesh (MQTT) Inicializado com Sucesso ---");
  digitalWrite(LED_VERDE_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_VERDE_PIN, LOW);
}

//=======================================================================
//                     5 - Loop Principal
//=======================================================================
void loop() {
  // Monitoramento Wi-Fi
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado! Reconectando...");
    delay(500);
  }

  // Monitoramento e Keep-Alive MQTT
  if (!mqttClient.connected()) {
    conectar_mqtt();
  }

  // Processa ACKs de QoS 1 e leitura de tópicos
  mqttClient.loop();

  // Processa Downlink (MQTT -> LoRa)
  Phy_mqtt_receive_DL();

  // Processa Uplink (LoRa -> MQTT)
  Phy_radio_receive_UL();
}