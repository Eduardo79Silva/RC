#include "macros.h"
#include "state_machine.h"
#include "alarm.h"
#include "link_layer.h"

int localFD;

int STOP = 0;


unsigned char buf[BUFSIZE] = {0};

void sendUA(unsigned char *res){

    res[0] = FLAG; // FLAG
    res[1] = A_TX; // A
    res[2] = C_UA; // C
    res[3] = BCC(res[1], res[2]); // BCC
    res[4] = FLAG; // FLAG

    int bytes = write(localFD, res, 5);
    printf("UA response sent, %d bytes written\n", bytes);
    return;
}


void receiverStart(int fd, int nRetransmissions)
{
    //Save file descriptor
    localFD = fd;

    while (!STOP && alarmCount < nRetransmissions){
        
        int bytes_ = read(localFD, buf, 1);
        if (buf != 0 && bytes_ > -1) //If there is data to read in the buffer
        {
            printf("Received %02x \n", buf[0]);

            
            STATE ans = stateMachine(buf, LlRx); //Check if the received data is a SET flag
            if (ans == FLAG_RCV)
            {
                printf("SET received\n");
                sendUA(&buf); //Send UA response to the transmitter
                STOP = TRUE;
            }
        }
    }
}

