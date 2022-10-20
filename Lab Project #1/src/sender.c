#include "macros.h"

int sendSET(int fd, unsigned char *msg)
{
    msg[0] = 0x7E; // FLAG
    msg[1] = 0x03; // A
    msg[2] = 0x03; // C
    msg[3] = 0x00; // BCC
    msg[4] = 0x7E; // FLAG

    int bytes = write(fd, msg, 5);
    printf("SET flag sent, %d bytes written\n", bytes);
    return bytes;
}

void senderStart(int fd)
{
    unsigned char buf[BUFSIZE] = {0};

    sendSET(fd, &buf);
    
    
}