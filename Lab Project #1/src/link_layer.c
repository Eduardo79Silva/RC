// Link layer protocol implementation

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "link_layer.h"
#include "macros.h"
#include "receiver.h"
#include "sender.h"
#include "utils.h"

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

struct termios oldtio;
struct termios newtio;
int nRetransmissions = 0;

int NS = 0;
int NR = 1;

extern int alarmEnabled;

//Delete
int timeout, fd, lastNum = -1;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

int llopen(LinkLayer connectionParameters)
{

    nRetransmissions = connectionParameters.nRetransmissions;
    timeout = connectionParameters.timeout;

    // Open serial port device for reading and writing (Given by the professor)
    printf("\n#########################__OPENING__#############################\n\n");

    printf("#   Opening connection on port: %s \n", connectionParameters.serialPort);

    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        return -1;
    }

    // Save current port settingsS
    if (tcgetattr(fd, &oldtio) == -1)
    {
        printf("#   Error saving port settings\n");
        perror("tcgetattr");
        return -1;
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 5;  // Blocking read until 5 chars received
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        printf("#   Error setting port settings\n");
        perror("tcsetattr");
        return -1;
    }

    printf("#   New termios structure set\n");

    // Start either transmitter or receiver

    if (connectionParameters.role == LlTx) {
        
        unsigned char cmdFrame[5] = {0};
        unsigned char c;
        STATE state = START;

        cmdFrame[0] = FLAG; // FLAG
        cmdFrame[1] = A_TX; // A
        cmdFrame[2] = C_SET; // C
        cmdFrame[3] = BCC(cmdFrame[1], cmdFrame[2]); // BCC
        cmdFrame[4] = FLAG; // FLAG

        while(alarmCount < connectionParameters.nRetransmissions){
            if(!alarmEnabled){
                state = START;
                if (write(fd,cmdFrame, 5) == -1){
                    printf("#       ERROR: Failed to write\n");
                    return -1;
                }
                startAlarm(connectionParameters.timeout);
            }

            int frameBytes = read(fd,&c,1);

            if(frameBytes > -1){
                stateMachine(&c, &state, C_UA);
            }

            if(state == STOP_ST){
                printf("#   UA RECEIVED\n");
                disableAlarm();
                break;
            }

        }

    } else if(connectionParameters.role == LlRx) {

        unsigned char cmdFrame[5] = {0};
        unsigned char c;
        STATE state = START;

        cmdFrame[0] = FLAG; // FLAG
        cmdFrame[1] = A_TX; // A
        cmdFrame[2] = C_UA; // C
        cmdFrame[3] = BCC(cmdFrame[1], cmdFrame[2]); // BCC
        cmdFrame[4] = FLAG; // FLAG

        //NÃO PRECISA DE UM ALARM COUNT POIS SE O TRANSMITER NÃO RECEBER O UA ELE DISPARA O ALARME

        while(state != STOP_ST){
            int frameBytes = read(fd,&c,1);
            if(frameBytes > -1){
                stateMachine(&c,&state,C_SET);
            }
        }

        int frameBytes = write(fd,cmdFrame,5);
        printf("#   UA response sent, %d bytes written\n", frameBytes);


    }

    printf("\n#################################################################\n\n");
    
    return 1;
}




////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////

int llwrite(const unsigned char *buf, int bufSize){

    printf("\n###########################__WRITING__#############################\n\n");

    unsigned char frame[MAX_PAYLOAD_SIZE+6] = {0};
    unsigned char BCC2 = BCC2creator(buf,bufSize);
    unsigned char BCC = 0x00, responses[5] = {0};
    int STOP = 0, ctrlRX = SHIFT(!NS, 7) | C_RR0;
    int idx = 4;

    frame[0] = FLAG;
    frame[1] = A_TX;
    frame[2] = SHIFT(NS, 6);
    frame[3] = BCC(frame[1],frame[2]);
   
    int newSize = bufSize;


    //Estamos a criar um novo array que vai conter a informação toda do buf+bcc2 para fazermos bytestuffing de tudo
    unsigned char newBuffer[bufSize+1];

    

    for(int i = 0; i<=bufSize; i++){
        newBuffer[i] = buf[i];
    }
    newBuffer[bufSize] = BCC2;


    for(int i=0; i<=bufSize; i++){
        if(newBuffer[i]==FLAG){
            frame[idx++]=0x7D;
            frame[idx++]=0x5e;
            continue;
        }
        else if(newBuffer[i]==0x7D){
            frame[idx++]=0x7D;
            frame[idx++]=0x5D;
            continue;
        }

        frame[idx++]=newBuffer[i];
    }

    frame[idx++] = FLAG;

    while(!STOP){
        if(!alarmEnabled){
            write(fd, frame, idx);
            printf("\n#     Control Packet sent NS=%d\n", NS);
            startAlarm(timeout);
        }
        
        int result = read(fd, responses, 5);

        if(result != -1 && responses != 0){

            if(responses[2] != (ctrlRX) || (responses[3] != (responses[1]^responses[2]))){
                    printf("\n#     RR not correct: 0x%02x%02x%02x%02x%02x\n", responses[0], responses[1], responses[2], responses[3], responses[4]);
                    alarmEnabled = FALSE;
                    continue;
            }
            
            else{
                printf("\n#     RR correctly received: 0x%02x%02x%02x%02x%02x\n", responses[0], responses[1], responses[2], responses[3], responses[4]);
                alarmEnabled = FALSE;
                STOP = 1;
            }
        }

        if(alarmCount >= nRetransmissions){
            printf("\n%d\n", nRetransmissions);
            printf("\n#     llwrite error: Exceeded number of tries when sending frame\n");
            STOP = 1;
            close(fd);
            return -1;
        }
        
    }

    printf("\n#     llwrite: Frame sent successfully\n");


    if(NS){
        NS = 0;
    }
    else {NS = 1;}

    printf("\n#################################################################\n\n");

    return 0;
}


////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet, int *sizeOfPacket)
{   
    
    printf("\n###########################__READING__#############################\n\n");

    unsigned char frame[600]={0}, responseFrame[5]={0}, BCC2=0x00, aux[400] = {0}, flagCount = 0, STOP = FALSE; 
    int control = SHIFT(!NR, 6), idx = 0, sizeInfo = 0;

    
    unsigned char cmdFrame[1] = {0}; 

    STATE st = START;
    unsigned char reading = TRUE;
    
    
    while (!STOP)
    { 
        if(reading){
            int frameBytes = read(fd, cmdFrame, 1); 
            if(frameBytes==-1 || frameBytes == 0) continue; 
            
        }
    
        
        switch (st)
        {
        case START:
            if(cmdFrame[0] == FLAG){
                st = FLAG_RCV;
                frame[sizeInfo++] = cmdFrame[0];
            }
            break;

        case FLAG_RCV:
            if(cmdFrame[0] != FLAG){
            
                st = A_RCV;
                frame[sizeInfo++] = cmdFrame[0];
            }
            else{
                memset(frame, 0, 600);
                st = FLAG_RCV;
                sizeInfo = 0;
                frame[sizeInfo++] = cmdFrame[0];
            }
            break;

        case A_RCV:
             if(cmdFrame[0] != FLAG){
                frame[sizeInfo++] = cmdFrame[0];
            }
            else if(cmdFrame[0] == FLAG){
            
                STOP = TRUE;
                frame[sizeInfo++] = cmdFrame[0];
                reading = FALSE;
            }
            break;
        
        default:
            break;
        }
    }

    responseFrame[0] = FLAG;
    responseFrame[1] = A_TX;
    
    responseFrame[4] = FLAG;


    if((frame[1]^frame[2]) != frame[3] || frame[2] != control){
        printf("\n#     InfoFrame not received correctly. Protocol error. Sending REJ.\n");
        responseFrame[2] = SHIFT(NR, 7) | 0x01;
        responseFrame[3] = responseFrame[1] ^ responseFrame[2];
        write(fd, responseFrame, 5);
        printf("\n#     -------------");
        printf("\n#     |    REJ    |");
        printf("\n#     -------------");
        printf("\n#     Size of REJ: %d\nREJ: 0x", 5);

        for(int i=0; i<5; i++){
            printf("#   %02X ", responseFrame[i]);
        }

        printf("\n\n");

        return -1;
    }


    //Destuffing

    for(int i=0; i<sizeInfo; i++){
        if(frame[i] == 0x7D && frame[i+1]==0x5e){
            packet[idx++] = FLAG;
            i++;
        }

        else if(frame[i] == 0x7D && frame[i+1]==0x5d){
            packet[idx++] = 0x7D;
            i++;
        }

        else {packet[idx++] = frame[i];}
    }

    //--------------------BCC2--------------------

    int size = 0; 

    if(packet[4]==0x01){
        size = 256*packet[6]+packet[7]+4 +6; 
        for(int i=4; i<size-2; i++){
            BCC2 = BCC2 ^ packet[i];
        }
    }
    
    else{
        size += packet[6]+ 3 + 4; 
        size += packet[size+1] + 2 +2; 

        for(int i=4; i<size-2; i++){
            BCC2 = BCC2 ^ packet[i];
        }
    }

    if(packet[size-2] == BCC2){

        if(packet[4]==0x01){
            if(frame[5] == lastNum){
                printf("\n#     InfoFrame received correctly. Repeated Frame. Sending RR.\n");
                responseFrame[2] = SHIFT(NR, 7) | C_RR0;
                responseFrame[3] = responseFrame[1] ^ responseFrame[2];
                write(fd, responseFrame, 5);
                return -1;
            }   
            else{
                lastNum = frame[5];
            }
        }
        printf("\n#     InfoFrame received correctly. Sending RR.\n");
        responseFrame[2] = SHIFT(NR, 7) | C_RR0;
        responseFrame[3] = responseFrame[1] ^ responseFrame[2];
        write(fd, responseFrame, 5);
    }
    
    else {
        printf("\n#     InfoFrame not received correctly. Error in data packet. Sending REJ.\n");
        responseFrame[2] = SHIFT(NR, 7) | 0x01;
        responseFrame[3] = responseFrame[1] ^ responseFrame[2];
        write(fd, responseFrame, 5);

        return -1;
    }

    (*sizeOfPacket) = size;

    //--------------------Reset Variables--------------------

    idx = 0;
    
    for(int i=4; i<(*sizeOfPacket)-2; i++){
        aux[idx++] = packet[i];
    }

    
    (*sizeOfPacket) = size - 6;

    memset(packet,0,sizeof(packet));

    for(int i=0; i<(*sizeOfPacket); i++){
        packet[i] = aux[i];
    }

    //--------------------Nr Change--------------------


    if(NR){
        NR = 0;
    }
    else {NR = 1;}

    printf("\n#################################################################\n\n");

    return 1;
}


////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics, LinkLayer connectionParameters, float runTime)
{       
    alarmCount = 0;

    printf("\n###########################__LLCLOSE__#############################\n\n");

    if(connectionParameters.role == LlRx){

        unsigned char cmdFrame[6] = {0}, responses[6] = {0};
        unsigned char STOP = 0, UA = 0;

        cmdFrame[0] = FLAG;
        cmdFrame[1] = A_TX;
        cmdFrame[2] = DISC;
        cmdFrame[3] = cmdFrame[1]^cmdFrame[2];
        cmdFrame[4] = FLAG;
        cmdFrame[5] = '\0';


        while(!STOP){
            int result = read(fd, responses, 5);
            
            responses[5] = '\0';

            if(result==-1){
                continue;
            }


            else if(strcasecmp(cmdFrame, responses) == 0){
                printf("\n#     DISC message received. Responding now.\n");
                
                cmdFrame[1] = 0x01;
                cmdFrame[3] = cmdFrame[1]^cmdFrame[2];

                while(alarmCount < nRetransmissions){

                    if(!alarmEnabled){
                        printf("\n#     DISC message sent, %d bytes written\n", 5);
                        write(fd, cmdFrame, 5);
                        startAlarm(timeout);
                    }
                    
                    int result = read(fd, responses, 5);
                    if(result != -1 && responses != 0 && responses[0]==FLAG){
                        if(responses[2] != 0x07 || (responses[3] != (responses[1]^responses[2]))){
                            printf("\n#     UA not correct: 0x%02x%02x%02x%02x%02x\n", responses[0], responses[1], responses[2], responses[3], responses[4]);
                            alarmEnabled = FALSE;
                            continue;
                        }
                        
                        else{   
                            printf("\n#     UA correctly received: 0x%02x%02x%02x%02x%02x\n", responses[0], responses[1], responses[2], responses[3], responses[4]);
                            alarmEnabled = FALSE;
                            close(fd);
                            break;
                        }
                    }

                }

                if(alarmCount >= nRetransmissions){
                    printf("\n#     Alarm limit reached, DISC message not sent\n");
                    return -1;
                }
                
                STOP = TRUE;
            }
        
        }

    }

    else{
        alarmCount = 0;

        unsigned char cmdFrame[6] = {0}, responses[6] = {0};

        cmdFrame[0] = FLAG;
        cmdFrame[1] = A_TX;
        cmdFrame[2] = DISC;
        cmdFrame[3] = cmdFrame[1]^cmdFrame[2];
        cmdFrame[4] = FLAG;
        cmdFrame[5] = '\0';

        while(alarmCount < nRetransmissions){

            if(!alarmEnabled){
                
                int frameBytes = write(fd, cmdFrame, 5);
                printf("\n#     DISC message sent, %d bytes written\n", frameBytes);
                startAlarm(timeout);
            }
            
            int result = read(fd, responses, 5);

            cmdFrame[1] = 0x01;
            cmdFrame[3] = cmdFrame[1]^cmdFrame[2];
            responses[5] = '\0';

            if(result != -1 && responses != 0 && responses[0]==FLAG){
                if(strcasecmp(cmdFrame, responses) != 0){
                    printf("\n#     DISC not correct: 0x%02x%02x%02x%02x%02x\n", responses[0], responses[1], responses[2], responses[3], responses[4]);
                    alarmEnabled = FALSE;
                    continue;
                }
                
                else{   
                    printf("\n#     DISC correctly received: 0x%02x%02x%02x%02x%02x\n", responses[0], responses[1], responses[2], responses[3], responses[4]);
                    alarmEnabled = FALSE;
                    
                    cmdFrame[1] = 0x01;
                    cmdFrame[2] = 0x07;
                    cmdFrame[3] = cmdFrame[1]^cmdFrame[2];

                    int frameBytes = write(fd, cmdFrame, 5);

                    close(fd);

                    printf("\n#     UA message sent, %d bytes written.\n\nWe will miss you Tux! <3\n", frameBytes);
                    return 1;

                }
            }

        }

        if(alarmCount >= nRetransmissions){
            printf("\n#     Alarm limit reached, DISC message not sent\n");
            close(fd);
            return -1;
        }


    }

    printf("\n#################################################################\n");

    if(showStatistics){
        printf("\n###########################__STATS__#############################\n\n");

        printf("\n#    Number of packets sent: %d\n#    Size of data packets in information frame: %d\n#    Total run time: %f\n#    Average time per packet: %f\n", lastNum, 200, runTime, runTime/200.0);

        printf("\n#################################################################\n");
    }

    return 1;
}
