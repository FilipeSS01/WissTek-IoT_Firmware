/*
  Camada Física, Roteamento Mesh e Transporte MQTT do Gateway
  Arquivo: 1_PHY.ino
*/

// ================= REGISTRADORES SX1276 PARA CAD =================
#define REG_OP_MODE       0x01
#define MODE_LORA         0x80
#define MODE_STANDBY      0x01
#define MODE_CAD          0x07
#define REG_IRQ_FLAGS     0x12
#define IRQ_CAD_DONE_MASK 0x04
#define IRQ_CAD_DETECTED  0x01

// Configuração de barramento SPI para o SX1276
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

// ================= ESCUTA DE CANAL (CAD) POR HARDWARE =================
bool Phy_canal_ocupado_CAD() {
  rfm95_writeRegister(REG_OP_MODE, MODE_LORA | MODE_STANDBY);
  rfm95_writeRegister(REG_IRQ_FLAGS, 0xFF);                 // Limpa interrupções
  rfm95_writeRegister(REG_OP_MODE, MODE_LORA | MODE_CAD); // Dispara ciclo CAD

  unsigned long timeout = millis() + 50;
  while (!(rfm95_readRegister(REG_IRQ_FLAGS) & IRQ_CAD_DONE_MASK)) {
    if (millis() > timeout) break;
    yield();
  }

  byte flags = rfm95_readRegister(REG_IRQ_FLAGS);
  rfm95_writeRegister(REG_IRQ_FLAGS, 0xFF);                 // Limpa flags
  rfm95_writeRegister(REG_OP_MODE, MODE_LORA | MODE_STANDBY);

  return (flags & IRQ_CAD_DETECTED); // Retorna true se houver preâmbulo no ar
}

// ================= FUNÇÕES DO CACHE DE DUPLICATAS =================
bool Gateway_pacote_duplicado(byte src, byte id) {
  for (int i = 0; i < CACHE_SIZE; i++) {
    if (cacheDuplicatas[i].src == src && cacheDuplicatas[i].id == id) {
      return true;
    }
  }
  return false;
}

void Gateway_salvar_cache(byte src, byte id) {
  cacheDuplicatas[cacheIndex].src = src;
  cacheDuplicatas[cacheIndex].id  = id;
  cacheIndex = (cacheIndex + 1) % CACHE_SIZE;
}

// ================= DOWNLINK (MQTT -> LORA) =================
void Phy_mqtt_receive_DL() {
  if (mqtt_dl_disponivel) {
    mqtt_dl_disponivel = false;

    // Copia o payload recebido via MQTT para o buffer de Downlink
    for (byte i = 0; i < Tamanho_pacote; i++) {
      PacoteDL[i] = mqtt_dl_payload[i];
    }

    // Injeta cabeçalho Mesh no Downlink (Lógica original preservada)
    seq_packet_gw++;
    PacoteDL[3] = HOP_LIMIT_PADRAO; // Byte 3: Salto inicial
    PacoteDL[4] = seq_packet_gw;    // Byte 4: Identificador sequencial
    PacoteDL[9] = ID_gateway;       // Byte 9: Origem do Gateway

    // Salva no próprio cache para evitar reprocessamento de eco
    Gateway_salvar_cache(ID_gateway, seq_packet_gw);

    // Transmite via LoRa com CAD
    Phy_radio_send_DL();
  }
}

void Phy_radio_send_DL() {
  byte tentativas = 0;
  while (Phy_canal_ocupado_CAD() && tentativas < 5) {
    delay(random(15, 50));
    tentativas++;
  }

  LoRa.beginPacket();
  for (int i = 0; i < Tamanho_pacote; i++) {
    LoRa.write(PacoteDL[i]);
  }
  LoRa.endPacket();

  digitalWrite(LED_VERMELHO_PIN, HIGH);  
  delay(50);                            
  digitalWrite(LED_VERMELHO_PIN, LOW);   
}

// ================= UPLINK (LORA -> MQTT) =================
void Phy_radio_receive_UL() {
  uint8_t packetSize = LoRa.parsePacket();
  
  if (packetSize > 0) {
    if (packetSize >= Tamanho_pacote) {
      for (int i = 0; i < Tamanho_pacote; i++) {
        PacoteUL[i] = LoRa.read();
      }
      
      RSSI_dBm_UL = LoRa.packetRssi();
      SNR_UL = LoRa.packetSnr();

      byte hop_limit   = PacoteUL[3];
      byte packet_id   = PacoteUL[4];
      byte destination = PacoteUL[8];
      byte source      = PacoteUL[9];

      // 1. Filtro Anti-Loop: Se o Gateway já recebeu esse Uplink, descarta
      if (Gateway_pacote_duplicado(source, packet_id)) {
        return; 
      }
      Gateway_salvar_cache(source, packet_id);

      // 2. Destino: O pacote é para este Gateway (ou broadcast geral)?
      if (destination == ID_gateway || destination == 0xFF) {
        Phy_mqtt_send_UL();
      }
    }
  }
}

void Phy_mqtt_send_UL() {
  if (RSSI_dBm_UL > -10.5) {
    RSSI_UL = 127;
  } else if (RSSI_dBm_UL <= -10.5 && RSSI_dBm_UL >= -74) {
    RSSI_UL = ((RSSI_dBm_UL + 74) * 2);
  } else {
    RSSI_UL = (((RSSI_dBm_UL + 74) * 2) + 256);
  }

  PacoteUL[5] = RSSI_UL;
  SNR_UL_inteiro = (int)(SNR_UL * 100); 
  PacoteUL[6] = (SNR_UL_inteiro / 256);
  PacoteUL[7] = (SNR_UL_inteiro % 256);

  // Publica o pacote binário de 20 bytes via MQTT com QoS 1
  if (mqttClient.connected()) {
    bool ok = mqttClient.publish(TOPIC_UL, (char*)PacoteUL, Tamanho_pacote, false, MQTT_QOS);
    if (ok) {
      Serial.println("[PHY] Pacote UL publicado via MQTT (QoS1).");
    } else {
      Serial.println("[PHY] Falha ao publicar pacote UL via MQTT.");
    }
  } else {
    Serial.println("[PHY] MQTT desconectado – pacote UL descartado.");
  }

  digitalWrite(LED_VERDE_PIN, HIGH);
  delay(50);
  digitalWrite(LED_VERDE_PIN, LOW);
}

// ================= GERENCIAMENTO DE CONEXÃO E CALLBACK MQTT =================
void mqtt_callback(MQTTClient *client, char topic[], char bytes[], int length) {
  if (strcmp(topic, TOPIC_DL) == 0) {
    if (length >= Tamanho_pacote) {
      for (int i = 0; i < Tamanho_pacote; i++) {
        mqtt_dl_payload[i] = (byte)bytes[i];
      }
      mqtt_dl_disponivel = true; // Sinaliza para consumo no loop
    }
  }
}

void conectar_mqtt() {
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Conectando ao broker...");
    if (mqttClient.connect(CLIENT_ID.c_str())) {
      Serial.println(" conectado!");
      mqttClient.subscribe(TOPIC_DL, MQTT_QOS);
      Serial.print("[MQTT] Inscrito (QoS");
      Serial.print(MQTT_QOS);
      Serial.print(") em: ");
      Serial.println(TOPIC_DL);
    } else {
      Serial.print(" falhou (erro=");
      Serial.print((int)mqttClient.lastError());
      Serial.println("). Tentando novamente em 3s...");
      delay(3000);
    }
  }
}