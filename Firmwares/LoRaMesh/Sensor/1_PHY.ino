/*
  Camada Física (PHY) - LoRa RFM95
  Arquivo: 1_PHY.ino (Nó Sensor)
*/

// ================= REGISTRADORES SX1276 PARA CAD =================
#define REG_OP_MODE       0x01
#define MODE_LORA         0x80
#define MODE_STANDBY      0x01
#define MODE_CAD          0x07
#define REG_IRQ_FLAGS     0x12
#define IRQ_CAD_DONE_MASK 0x04
#define IRQ_CAD_DETECTED  0x01

// Configuração do barramento SPI para comunicação com o SX1276
SPISettings rfm95_spiSettings(8E6, MSBFIRST, SPI_MODE0);

// ================= LEITURA E ESCRITA DIRETA VIA SPI =================
uint8_t rfm95_readRegister(uint8_t address) {
  digitalWrite(NSS_PIN, LOW);
  SPI.beginTransaction(rfm95_spiSettings);
  SPI.transfer(address & 0x7F);        // MSB = 0 para Leitura
  uint8_t response = SPI.transfer(0x00);
  SPI.endTransaction();
  digitalWrite(NSS_PIN, HIGH);
  return response;
}

void rfm95_writeRegister(uint8_t address, uint8_t value) {
  digitalWrite(NSS_PIN, LOW);
  SPI.beginTransaction(rfm95_spiSettings);
  SPI.transfer(address | 0x80);        // MSB = 1 para Escrita
  SPI.transfer(value);
  SPI.endTransaction();
  digitalWrite(NSS_PIN, HIGH);
}

// ================= FUNÇÃO DE DETECÇÃO DE ATIVIDADE (CAD) =================
// Retorna true se houver preâmbulo LoRa no ar (canal ocupado)
bool Phy_canal_ocupado_CAD() {
  // 1. Coloca o rádio em Standby para configurar
  rfm95_writeRegister(REG_OP_MODE, MODE_LORA | MODE_STANDBY);
  
  // 2. Limpa todas as flags de interrupção anteriores
  rfm95_writeRegister(REG_IRQ_FLAGS, 0xFF);
  
  // 3. Dispara o modo CAD
  rfm95_writeRegister(REG_OP_MODE, MODE_LORA | MODE_CAD);

  // 4. Aguarda a conclusão do ciclo de CAD (timeout de segurança de 50ms)
  unsigned long timeout = millis() + 50;
  while (!(rfm95_readRegister(REG_IRQ_FLAGS) & IRQ_CAD_DONE_MASK)) {
    if (millis() > timeout) {
      break;
    }
    yield();
  }

  // 5. Lê o registrador de flags para verificar se detectou portadora
  byte flags = rfm95_readRegister(REG_IRQ_FLAGS);
  
  // 6. Limpa as flags e volta para Standby
  rfm95_writeRegister(REG_IRQ_FLAGS, 0xFF);
  rfm95_writeRegister(REG_OP_MODE, MODE_LORA | MODE_STANDBY);

  // Retorna verdadeiro se o bit CadDetected estiver em 1
  return (flags & IRQ_CAD_DETECTED);
}

// ================= CAMADA FÍSICA RX DL (RECEPÇÃO) =================
void Phy_radio_receive_DL() {
  uint8_t packetSize = LoRa.parsePacket();

  if (packetSize) {
    if (packetSize >= Tamanho_pacote) {
      // Lê os 20 bytes do buffer de rádio para a memória RAM
      for (int i = 0; i < Tamanho_pacote; i++) {
        PacoteDL[i] = LoRa.read();
      }

      // Pisca o LED Verde indicando pacote físico capturado
      digitalWrite(LED_VERDE_PIN, HIGH);
      delay(50);
      digitalWrite(LED_VERDE_PIN, LOW);

      // Medições do enlace físico
      RSSI_dBm_DL = LoRa.packetRssi();
      SNR_DL = LoRa.packetSnr();

      Serial.print("[PHY] Pacote RX recebido. RSSI: ");
      Serial.print(RSSI_dBm_DL);
      Serial.print(" dBm | SNR: ");
      Serial.print(SNR_DL);
      Serial.println(" dB");

      // Entrega o pacote e a SNR medida para a Camada MAC
      Mac_radio_receive(PacoteDL, SNR_DL);
    }
  }
}

// ================= CAMADA FÍSICA TX UL (TRANSMISSÃO BRUTA) =================
// Capaz de transmitir tanto PacoteUL próprio quanto repasses da malha
void Phy_radio_send_raw(byte* buffer) {

  // Adequação da leitura de RSSI para o Byte 0
  if (RSSI_dBm_DL > -10.5) {
    RSSI_DL = 127;
  } else if (RSSI_dBm_DL <= -10.5 && RSSI_dBm_DL >= -74) {
    RSSI_DL = ((RSSI_dBm_DL + 74) * 2);
  } else {
    RSSI_DL = (((RSSI_dBm_DL + 74) * 2) + 256);
  }

  // Preenchimento das métricas físicas nos bytes 0, 1 e 2
  buffer[0] = RSSI_DL;
  SNR_DL_inteiro = (int)(SNR_DL * 100);
  buffer[1] = (SNR_DL_inteiro / 256);
  buffer[2] = (SNR_DL_inteiro % 256);

  // Transmissão via hardware RFM95
  LoRa.beginPacket();
  for (int i = 0; i < Tamanho_pacote; i++) {
    LoRa.write(buffer[i]);
  }
  LoRa.endPacket();

  // Pisca o LED Vermelho indicando irradiação de RF concluída
  digitalWrite(LED_VERMELHO_PIN, HIGH);
  delay(80);
  digitalWrite(LED_VERMELHO_PIN, LOW);

  Serial.println("[PHY] Pacote transmitido no ar via RFM95.");
}