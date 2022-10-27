#pragma once

#define A_RX 0x01
#define A_TX 0x03
#define C_SET 0x03
#define C_DISC 0x0B
#define C_UA 0x07
#define C_RR0 0x05
#define C_RR1 0x85
#define C_REJ0 0x01
#define C_REJ1 0x81
#define FLAG 0x7E
#define ESC 0x7D
#define DISC 0x0B
#define BCC(x, y) (x^y)

#define FALSE 0
#define TRUE 1

#define BUFSIZE 256
#define MAX_PACKET_SIZE 256

typedef enum
{   
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    STOP_ST,
    INVALID
} STATE;
