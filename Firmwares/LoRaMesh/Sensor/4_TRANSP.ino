/*
  Camada de Transporte (TRANSP) - Controle Fim a Fim
  Arquivo: 4_TRANSP.ino
*/

// ================= CAMADA DE TRANSPORTE DL (RECEBIMENTO) =================
void Transp_radio_receive_DL() { 
  // Incrementa o contador de pacotes de Downlink entregues a este nó
  contador_pkt_DL = contador_pkt_DL + 1;

  Serial.print("[TRANSP] Downlink fim a fim recebido com sucesso. Total DL: ");
  Serial.println(contador_pkt_DL);

  // Entrega o pacote para a Camada de Aplicação processar o comando
  App_radio_receive_DL();
}

// ================= CAMADA DE TRANSPORTE UL (ORIGINAÇÃO) =================
void Transp_radio_send_UL() { 
  // 1. Aloca no pacote de UL o contador acumulado de DL recebidos (Bytes 12 e 13)
  PacoteUL[12] = (contador_pkt_DL / 256);
  PacoteUL[13] = (contador_pkt_DL % 256);
  
  // 2. Incrementa e aloca o contador de UL gerados por este nó (Bytes 14 e 15)
  contador_pkt_UL = contador_pkt_UL + 1;
  PacoteUL[14] = (contador_pkt_UL / 256);
  PacoteUL[15] = (contador_pkt_UL % 256);

  Serial.print("[TRANSP] Preparando métricas de transporte. Total UL: ");
  Serial.println(contador_pkt_UL);

  // Encaminha para a Camada de Rede (NET) envelopar com cabeçalho de malha
  Net_radio_send_UL();
}