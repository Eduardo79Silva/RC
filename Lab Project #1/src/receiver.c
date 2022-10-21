#include "macros.h"
#include "state_machine.h"

int localFD;


volatile int STOP = 0;

unsigned char buf[BUFSIZE] = {0};

void sendUA(unsigned char *res){

    res[0] = FLAG; // FLAG
    res[1] = A_RX; // A
    res[2] = C_UA; // C
    res[3] = BCC(A_RX, C_UA); // BCC
    res[4] = FLAG; // FLAG

    int bytes = write(localFD, res, 5);
    printf("UA response sent, %d bytes written\n", bytes);
    return bytes;
}


void receiverStart(int fd)
{
    //Save file descriptor
    localFD = fd;

    while (STOP == FALSE)
    {
        int bytes_ = read(localFD, buf, 1);
        if (buf != 0 && bytes_ > -1) //If there is data to read in the buffer
        {
            printf("Received %02x \n", buf[0]);
            int ans = stateMachine(buf, BUFSIZE, C_SET); //Check if the received data is a SET flag
            if (ans == 1)
            {
                printf("SET received\n");
                sendUA(&buf); //Send UA response to the transmitter
                STOP = TRUE;
            }
        }
    }
}

