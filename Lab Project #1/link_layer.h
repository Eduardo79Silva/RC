#pragma once



struct linkLayer {
    char port[20]; /*Dispositivo /dev/ttySx, x = 0, 1*/
    int baudRate; /*Velocidade de transmissão*/
    unsigned int sequenceNumber; /*Número de sequência da trama: 0, 1*/
    unsigned int timeout; /*Valor do temporizador: 1 s*/
    unsigned int numTransmissions; /*Número de tentativas em caso de
    falha*/
    char frame[MAX_SIZE]; /*Trama*/
}

/**
 * Função para establecer a conexão entre dois dispositivos
 * @param porta Nome da porta
 * @param flagRole Flag usada para saber se é o transmiter ou receiver
 * @return Retorna o valor do file descriptor ou -1 em caso de erro
 */

int llopen(int porta, int flagRole);