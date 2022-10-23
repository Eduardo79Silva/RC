#pragma once

#define A_RX 0x01
#define A_TX 0x03
#define C_SET 0x03
#define C_DISC 0x0B
#define C_UA 0x07
#define C_RR 0x05
#define CREJ 0x01
#define FLAG 0x7E
#define ESC 0x7D
#define BCC(x, y) (x^y)

#define FALSE 0
#define TRUE 1

#define BUFSIZE 256

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
