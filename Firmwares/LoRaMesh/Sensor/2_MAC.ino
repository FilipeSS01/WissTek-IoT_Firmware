/*
  Camada de Enlace e Controle de Acesso ao Meio (MAC)
  Arquivo: 2_MAC.ino
*/

// ================= CAMADA MAC RX DL (RECEPÇÃO) =================
void Mac_radio_receive(byte* pacoteRecebido, float snrRecebido) {
  // Entrega o pacote e o SNR medido para a Camada de Rede (NET)
  // A Camada de Rede verificará o cache de duplicatas e o destino lógico (ID)
  Net_radio_receive(pacoteRecebido, snrRecebido);
}

// ================= CAMADA MAC TX UL (TRANSMISSÃO E CONTENÇÃO) =================
void Mac_radio_send(byte* pacoteParaEnviar, float snrRecebido, bool ehRetransmissao) {
  
  // 1. Atraso Estocástico ponderado por SNR (aplicado apenas em repasses da malha)
  if (ehRetransmissao) {
    // Mapeamento de SNR (intervalo operacional típico entre -20 dB e +10 dB):
    // SNR baixo (-20 dB) -> nó periférico -> espera menos (30 ms) -> ganha prioridade
    // SNR alto  (+10 dB) -> nó próximo   -> espera mais (350 ms) -> aguarda na fila
    float snrLimitado = constrain(snrRecebido, -20.0, 10.0);
    long delay_snr = map((long)snrLimitado, -20, 10, 30, 350);
    
    // Jitter pseudoaleatório para evitar colisões entre nós equidistantes
    long jitter = random(10, 100);
    
    Serial.print("[MAC] Contenção: Backoff SNR (");
    Serial.print(delay_snr);
    Serial.print(" ms) + Jitter (");
    Serial.print(jitter);
    Serial.println(" ms)");
    
    delay(delay_snr + jitter);
  }

  // 2. Verificação de Canal Livre via CAD (Channel Activity Detection)
  byte tentativas = 0;
  while (Phy_canal_ocupado_CAD() && tentativas < 5) {
    Serial.println("[MAC] Canal ocupado (preâmbulo LoRa detectado). Aplicando recuo...");
    delay(random(20, 70)); // Janela aleatória de recuo rápido
    tentativas++;
  }

  // 3. Despacho para a Camada Física (PHY)
  Phy_radio_send_raw(pacoteParaEnviar);
}