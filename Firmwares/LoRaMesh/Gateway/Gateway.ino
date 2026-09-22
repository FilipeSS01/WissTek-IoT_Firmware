/*
  MoT LoRa Mesh - Gateway (Inundação Gerenciada)
  Última versão: Adaptado para ESP32 + RFM95 (Kit PK-LoRa)
  Arquivo: 1_Gateway_LoRa_V0_6.ino
*/

//=======================================================================
//                     1 - Bibliotecas
//=======================================================================
#include <SPI.h>
#include <LoRa.h>

//=======================================================================
//                     2 - Variáveis e Mapeamento
//=======================================================================

// ============= Pinagem na placa PK-LoRa (RFM95 com ESP32)
#define SCK_PIN    5
#define MISO_PIN  19
#define MOSI_PIN  27
#define NSS_PIN   18
#define RST_PIN   14
#define DIO0_PIN  26

// ============= CAMADA FÍSICA
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

// ============== ESTRUTURA DO PACOTE (20 BYTES)
#define Tamanho_pacote 20
byte PacoteDL[Tamanho_pacote];
byte PacoteUL[Tamanho_pacote];

// ============== PARÂMETROS DE REDE (MESH)
int ID_gateway = 100;      // Identificador deste Gateway
#define HOP_LIMIT_PADRAO 3 // Quantidade inicial de saltos para Downlinks
byte seq_packet_gw = 0;    // Gerador sequencial de Packet_ID do Gateway

// Cache Circular de Duplicatas (Anti-Looping para Uplinks recebidos)
#define CACHE_SIZE 16
struct PacketHistory {
  byte src;
  byte id;
};
PacketHistory cacheDuplicatas[CACHE_SIZE];
byte cacheIndex = 0;

//=======================================================================
//                     3 - Setup de inicialização
//=======================================================================
void setup() {
  Serial.begin(115200);

  pinMode(LED_VERMELHO_PIN, OUTPUT);
  pinMode(LED_VERDE_PIN, OUTPUT);
  digitalWrite(LED_VERMELHO_PIN, LOW);
  digitalWrite(LED_VERDE_PIN, LOW);

  // Inicialização SPI e LoRa
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, NSS_PIN);
  LoRa.setPins(NSS_PIN, RST_PIN, DIO0_PIN);

  if (!LoRa.begin(FREQUENCY_IN_HZ)) {
    Serial.println("Erro ao iniciar LoRa no Gateway");
  }
  
  LoRa.setTxPower(txPower); 
  LoRa.setSpreadingFactor(spreadingFactor); 
  LoRa.setSignalBandwidth(signalBandwidth); 
  LoRa.setCodingRate4(codingRateDenominator); 

  // Inicializa o cache de duplicatas com valores nulos
  for (int i = 0; i < CACHE_SIZE; i++) {
    cacheDuplicatas[i].src = 0xFF;
    cacheDuplicatas[i].id  = 0xFF;
  }

  Serial.println("--- Gateway LoRa Mesh Inicializado com Sucesso ---");
  digitalWrite(LED_VERDE_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_VERDE_PIN, LOW);
}

//=======================================================================
//                     4 - Loop de repetição
//=======================================================================
void loop() {
  Phy_serial_receive_DL(); // Escuta comandos do Python via Serial
  Phy_radio_receive_UL();  // Escuta dados dos sensores via LoRa
}