/*
  Camada de Rede (NET) - Roteamento por Inundação Gerenciada
  Arquivo: 3_NET.ino
*/

// ================= GESTÃO DE DUPLICATAS NA MEMÓRIA RAM =================

// Verifica se a tupla (Source_ID, Packet_ID) já está registrada no cache
bool Net_pacote_duplicado(byte src, byte id) {
  for (int i = 0; i < CACHE_SIZE; i++) {
    if (cacheDuplicatas[i].src == src && cacheDuplicatas[i].id == id) {
      return true;
    }
  }
  return false;
}

// Armazena a nova tupla no cache
void Net_salvar_cache(byte src, byte id) {
  cacheDuplicatas[cacheIndex].src = src;
  cacheDuplicatas[cacheIndex].id  = id;
  cacheIndex = (cacheIndex + 1) % CACHE_SIZE;
}

// ================= CAMADA DE REDE RX DL (RECEBIMENTO E REPASSE) =================
void Net_radio_receive(byte* pacote, float snr) {
  // Extrai os campos do cabeçalho da malha
  byte hop_limit   = pacote[3];
  byte packet_id   = pacote[4];
  byte destination = pacote[8];
  byte source      = pacote[9];

  // 1. Barreira Anti-Loop: descarta se já viu este pacote
  if (Net_pacote_duplicado(source, packet_id)) {
    Serial.println("[NET] Pacote duplicado detectado. Descartando...");
    return;
  }

  // Registra no cache de duplicatas
  Net_salvar_cache(source, packet_id);

  Serial.print("[NET] Pacote inédito. Origem: ");
  Serial.print(source);
  Serial.print(" | Destino: ");
  Serial.print(destination);
  Serial.print(" | ID: ");
  Serial.print(packet_id);
  Serial.print(" | Saltos restantes: ");
  Serial.println(hop_limit);

  // 2. Avaliação do Destino Lógico da Mensagem
  if (destination == ID_sensor || destination == 0xFF) {
    // A mensagem é para este nó (ou broadcast geral)

    /*
      Destination em broadcast (0xFF) é utilizando todo o conceito de broadcast e não a junção do unicast com broadcast.
      Ou seja, aqui eu garanto que o nó quando receber uma mensagem com destino 0xFF, vai saber que é exatamente o gateway que está enviando.
      
      Da mesma forma ocorre quando o destination é direcionando para o sensor atual (ID_sensor).
      Ou seja, o nó vai saber que é mensagem do gateway e não de outro sensor da malha, pois não existe comunicação direcionada entre sensores. 
    */
    ID_gateway = source;

    Serial.println("[NET] Mensagem endereçada a este nó. Encaminhando para Camada de Transporte...");
    Transp_radio_receive_DL();
  } 
  else {
    // 3. Encaminhamento por Inundação (Repetidor Mesh)
    // O pacote não é para este nó: se houver saldo de saltos, ajuda a propagar
    if (hop_limit > 1) {
      pacote[3] = hop_limit - 1; // Decrementa a vida útil do pacote

      Serial.println("[NET] Não sou o destinatário. Reencaminhando pacote para a malha...");
      
      // Solicita transmissão com atraso estocástico por SNR na MAC (ehRetransmissao = true)
      Mac_radio_send(pacote, snr, true);
    } else {
      Serial.println("[NET] Limite de saltos esgotado (hop_limit atingiu 1). Pacote descartado.");
    }
  }
}

// ================= CAMADA DE REDE TX UL =================
void Net_radio_send_UL() {
  seq_packet_local++; // Gera novo identificador sequencial para o pacote local

  // Preenche o cabeçalho de rede no PacoteUL
  PacoteUL[3] = HOP_LIMIT_PADRAO; // Byte 3: Salto inicial (ex: 3)
  PacoteUL[4] = seq_packet_local;  // Byte 4: Identificador único do pacote
  PacoteUL[8] = ID_gateway;        // Byte 8: Endereço do destinatário final
  PacoteUL[9] = ID_sensor;         // Byte 9: Endereço de origem

  // Registra o próprio pacote no cache para não processar o eco caso um vizinho retransmita
  Net_salvar_cache(ID_sensor, seq_packet_local);

  Serial.print("[NET] Criando Uplink próprio. ID Pacote: ");
  Serial.print(seq_packet_local);
  Serial.print(" | Destino Gateway: ");
  Serial.println(ID_gateway);

  // Envia diretamente para a MAC sem atraso de contenção estocástica (ehRetransmissao = false)
  Mac_radio_send(PacoteUL, 0, false);
}