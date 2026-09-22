/*
  Camada de Aplicação (APP) - Nó Sensor
  Arquivo: 5_APP.ino
*/

// ================= CAMADA DE APLICAÇÃO DL (PROCESSAMENTO DE COMANDO) =================
void App_radio_receive_DL() {
  Serial.print("[APP] Executando comando de Downlink. Byte 16: ");
  Serial.println(PacoteDL[16]);

  // Acionamento do LED amarelo conforme o comando contido no byte 16
  if (PacoteDL[16] == 1) {
    digitalWrite(LED_AMARELO_PIN, HIGH);
    Serial.println("[APP] LED Amarelo LIGADO");
  } else {
    digitalWrite(LED_AMARELO_PIN, LOW);
    Serial.println("[APP] LED Amarelo DESLIGADO");
  }

  // Desencadeia o envio da resposta de telemetria (Uplink) de volta ao Gateway
  App_radio_send_UL();
}

// ================= CAMADA DE APLICAÇÃO UL (LEITURA E EMPACOTAMENTO) =================
void App_radio_send_UL() {
  // Limpa apenas o espaço de payload de aplicação (bytes 16 a 19)
  for (int i = 16; i < Tamanho_pacote; i++) {
    PacoteUL[i] = 0;
  }

  // Leitura do sensor LDR (ADC de 12 bits do ESP32: 0 a 4095)
  luminosidade = analogRead(LDR_PIN);
  
  // Aloca as informações de telemetria no PacoteUL
  PacoteUL[16] = PacoteDL[16];          // Confirma/ecoa o comando recebido
  PacoteUL[17] = 1;                     // Código do hardware (PK-LoRa v31 = 1)
  PacoteUL[18] = (luminosidade / 256);  // Byte mais significativo (MSB)
  PacoteUL[19] = (luminosidade % 256);  // Byte menos significativo (LSB)

  Serial.print("[APP] Luminosidade lida: ");
  Serial.print(luminosidade);
  Serial.println(" (ADC 12 bits). Despachando para Transporte...");

  // Encaminha para a Camada de Transporte alocar contadores fim a fim
  Transp_radio_send_UL();
}