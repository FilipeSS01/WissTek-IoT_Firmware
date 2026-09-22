/*
  MoT LoRa Mesh - Versão Inundação Gerenciada
  Adaptado para ESP32 + RFM95 (Kit PK-LoRa)
  Arquivo: 0_Sensor_LoRa_V0_6.ino
*/

//=======================================================================
//                     1 - Bibliotecas
//=======================================================================
#include <SPI.h>   // SPI para interligar ESP32 com o RFM95
#include <LoRa.h>  // Driver do rádio RFM95

//=======================================================================
//                     2 - Variáveis e Mapeamento
//=======================================================================

// ============= Pinagem na placa PK-LoRa (RFM95 com ESP32)
#define SCK_PIN 5
#define MISO_PIN 19
#define MOSI_PIN 27
#define NSS_PIN 18
#define RST_PIN 14
#define DIO0_PIN 26

// ============= CAMADA FÍSICA
#define FREQUENCY_IN_HZ 915E6    // Frequência LoRa (915 MHz)
#define txPower 17               // Potência TX em dBm
#define spreadingFactor 7        // SF7
#define signalBandwidth 125E3    // Largura de banda 125 kHz
#define codingRateDenominator 8  // CR 4/8

// Variáveis de medição de sinal do rádio
int RSSI_dBm_DL;
int RSSI_DL;
float SNR_DL;
int SNR_DL_inteiro;

// ============== CAMADA MAC
#define Tamanho_pacote 20
byte PacoteDL[Tamanho_pacote];
byte PacoteUL[Tamanho_pacote];

// ============= CAMADA DE REDE (PARÂMETROS MESH)
int ID_sensor = 1;          // Identificador exclusivo deste nó sensor
int ID_gateway = 100;       // ID padrão do Gateway de destino
#define HOP_LIMIT_PADRAO 3  // Quantidade inicial de saltos (decrementada a cada repasse)
byte seq_packet_local = 0;  // Contador incremental para identificação única de pacotes locais

// Estrutura do Cache Circular de Duplicatas (Anti-Looping na Memória RAM)
#define CACHE_SIZE 16
struct PacketHistory {
  byte src;  // Source_ID
  byte id;   // Packet_ID
};
PacketHistory cacheDuplicatas[CACHE_SIZE];
byte cacheIndex = 0;

// ============== CAMADA DE TRANSPORTE
int contador_pkt_DL = 0;
int contador_pkt_UL = 0;

// ============= CAMADA DE APLICAÇÃO
#define LED_VERMELHO_PIN 15
#define LED_AMARELO_PIN 2
#define LED_VERDE_PIN 4
#define LDR_PIN 36    // ADC1_CH0 - Sensor LDR (PIN VP)
#define BOTAO_PIN 39  // Botão (PIN VN)
int luminosidade;

//=======================================================================
//                     3 - Setup de inicialização
//=======================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("--- Iniciando Nó Sensor Mesh (Inundação Gerenciada) ---");

  // Configuração dos pinos de I/O
  pinMode(LED_VERMELHO_PIN, OUTPUT);
  pinMode(LED_AMARELO_PIN, OUTPUT);
  pinMode(LED_VERDE_PIN, OUTPUT);
  digitalWrite(LED_VERMELHO_PIN, LOW);
  digitalWrite(LED_AMARELO_PIN, LOW);
  digitalWrite(LED_VERDE_PIN, LOW);

  pinMode(BOTAO_PIN, INPUT);

  // Inicialização do barramento SPI e pinos de controle do RFM95
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, NSS_PIN);
  LoRa.setPins(NSS_PIN, RST_PIN, DIO0_PIN);

  if (!LoRa.begin(FREQUENCY_IN_HZ)) {
    Serial.println("Erro ao iniciar módulo RFM95!");
  }

  LoRa.setTxPower(txPower);
  LoRa.setSpreadingFactor(spreadingFactor);
  LoRa.setSignalBandwidth(signalBandwidth);
  LoRa.setCodingRate4(codingRateDenominator);

  // Inicializa a tabela de duplicatas com valores vazios
  for (int i = 0; i < CACHE_SIZE; i++) {
    cacheDuplicatas[i].src = 0xFF;
    cacheDuplicatas[i].id = 0xFF;
  }

  Serial.println("Módulo LoRa configurado e pronto para operação Mesh.");

  // Pisca o LED Verde indicando boot concluído
  digitalWrite(LED_VERDE_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_VERDE_PIN, LOW);
}

//=======================================================================
//                     4 - Loop principal
//=======================================================================
void loop() {
  Phy_radio_receive_DL();
}