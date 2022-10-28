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

        if(alarmCount >= connectionParameters.nRetransmissions){
            printf("\n#     UA NOT RECEIVED\n");
            close(fd);
            return -1;
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

    //---Variables---

    unsigned char frame[MAX_PAYLOAD_SIZE+6] = {0};
    unsigned char BCC2 = BCC2creator(buf,bufSize);
    unsigned char responses[5] = {0};
    unsigned char newBuffer[bufSize+1];
    int STOP = 0, ctrlRX = SHIFT(!NS, 7) | C_RR0;
    int idx = 4;

    //---Frame creation---

    frame[0] = FLAG;
    frame[1] = A_TX;
    frame[2] = SHIFT(NS, 6);
    frame[3] = BCC(frame[1],frame[2]);
   

    //Estamos a criar um novo array que vai conter a informação toda do buf+bcc2 para fazermos bytestuffing de tudo
    for(int i = 0; i<=bufSize; i++){
        newBuffer[i] = buf[i];
    }
    newBuffer[bufSize] = BCC2;

    idx = byte_stuffing(&newBuffer, bufSize,  &frame, idx);

    frame[idx++] = FLAG;

    //---Frame sending---

    while(!STOP){

        if(!alarmEnabled){
            write(fd, frame, idx);
            printf("\n#     Control Packet sent NS=%d\n", NS);
            startAlarm(timeout);
        }
        
        int result = read(fd, responses, 5);

        if(result != -1 && responses != 0){

            if(responses[2] != (ctrlRX) || (responses[3] != BCC(responses[1],responses[2]))){
                    printf("\n#     REJ: 0x%02x%02x%02x%02x%02x\n", responses[0], responses[1], responses[2], responses[3], responses[4]);
                    alarmEnabled = FALSE;
                    continue;
            }
            
            else{
                printf("\n#     RR: 0x%02x%02x%02x%02x%02x\n", responses[0], responses[1], responses[2], responses[3], responses[4]);
                alarmEnabled = FALSE;
                STOP = 1;
            }
        }

        if(alarmCount >= nRetransmissions){
            printf("\n#     llwrite error: Exceeded number of tries when sending frame\n");
            STOP = 1;
            close(fd);
            return -1;
        }
        
    }

    printf("\n#     llwrite: Frame sent successfully\n");

    //---Change Sender Number---

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
int llread(unsigned char *packet, int *packetSize)
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
    
        //-----STATE MACHINE-----
        
        STOP = dataStateMachine(&frame, &st, &cmdFrame, &reading, &sizeInfo);

    }

    responseFrame[0] = FLAG;
    responseFrame[1] = A_TX;
    
    responseFrame[4] = FLAG;


    if(BCC(frame[1],frame[2]) != frame[3] || frame[2] != control){
        printf("\n#     Wrong I Frame - Protocol error\n");
        responseFrame[2] = SHIFT(NR, 7) | 0x01;
        responseFrame[3] = BCC(responseFrame[1], responseFrame[2]);
        write(fd, responseFrame, 5);
        printf("\n#     -------------");
        printf("\n#     |    REJ    |");
        printf("\n#     -------------");
        printf("\n#     REJ Size: %d\nREJ: 0x", 5);

        for(int i=0; i<5; i++){
            printf("#   %02X ", responseFrame[i]);
        }

        printf("\n\n");

        return -1;
    }


    //Destuffing

    idx = byte_destuffing(&frame, sizeInfo, packet, idx);

    //--------------------BCC2--------------------

    int len = 0; 

    if(packet[4]==1){  
        len = packet[7]+10+BUFSIZE*packet[6]; 
        for(int i=4; i<len-2; i++){
            BCC2 = BCC(BCC2, packet[i]);
        }
    }
    
    else{
        len = len + packet[6]+ 7; 
        len = len + packet[len+1] + 4; 

        for(int i=4; i<len-2; i++){
            BCC2 = BCC(BCC2, packet[i]);
        }
    }

    if(packet[len-2] == BCC2){
        if(packet[4]==0x01){
            if(frame[5] == lastNum){
                responseFrame[2] = SHIFT(NR, 7) | C_RR0;
                responseFrame[3] = (responseFrame[1], responseFrame[2]);
                write(fd, responseFrame, 5);
                printf("\n#     Duplicated I Frame received.\n");
                return -1;
            }   
            else{
                lastNum = frame[5];
            }
        }
        responseFrame[2] = SHIFT(NR, 7) | C_RR0;
        responseFrame[3] = BCC(responseFrame[1], responseFrame[2]);
        write(fd, responseFrame, 5);
        printf("\n#     Received I Frame.\n");
    }
    else {
        responseFrame[2] = SHIFT(NR, 7) | 0x01;
        responseFrame[3] = BCC(responseFrame[1] , responseFrame[2]);
        write(fd, responseFrame, 5);
        printf("\n#     Didn't receive I frame\n");

        return -1;
    }

    (*packetSize) = len;

    //--------------------Nr Change--------------------


    if(NR){
        NR = 0;
    }
    else {NR = 1;}


    //--------------------Reset Variables--------------------

    idx = 0;
    
    for(int i=4; i<(*packetSize)-2; i++){
        aux[idx++] = packet[i];
    }


    for(int i = 0; i < (*packetSize); i++){
        packet[i] = 0;
    }

    (*packetSize) = len - 6;


    for(int i=0; i<(*packetSize); i++){
        packet[i] = aux[i];
    }



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
        unsigned char STOP = 0;
        //unsigned char UA = 0;

        cmdFrame[0] = FLAG;
        cmdFrame[1] = A_TX;
        cmdFrame[2] = DISC;
        cmdFrame[3] = BCC(cmdFrame[1],cmdFrame[2]);
        cmdFrame[4] = FLAG;
        cmdFrame[5] = '\0';


        while(!STOP){
            int result = read(fd, responses, 5);
            
            responses[5] = '\0';

            if(result==-1){
                continue;
            }


            else if(strcasecmp(cmdFrame, responses) == 0){
                printf("\n#     Received DISC.\n");
                
                cmdFrame[1] = 1;
                cmdFrame[3] = BCC(cmdFrame[1],cmdFrame[2]);

                while(alarmCount < nRetransmissions){

                    if(alarmEnabled == 0){
                        printf("\n#     Sent DISC, %d bytes written\n", 5);
                        write(fd, cmdFrame, 5);
                        startAlarm(timeout);
                    }
                    
                    int result = read(fd, responses, 5);
                    if(result >=0 && responses[0]==FLAG && responses != 0 ){
                        if(responses[2] != 0x07 || (responses[3] != BCC(responses[1],responses[2]))){
                            printf("\n#     Wrong UA: 0x%02x%02x%02x%02x%02x\n", responses[0], responses[1], responses[2], responses[3], responses[4]);
                            alarmEnabled = FALSE;
                            continue;
                        }
                        
                        else{   
                            printf("\n#     Good UA: 0x%02x%02x%02x%02x%02x\n", responses[0], responses[1], responses[2], responses[3], responses[4]);
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

        unsigned char cmdFrame[6] = {0};
        unsigned char responses[6] = {0};

        cmdFrame[0] = FLAG;
        cmdFrame[1] = A_TX;
        cmdFrame[2] = DISC;
        cmdFrame[3] = BCC(cmdFrame[1], cmdFrame[2]);
        cmdFrame[4] = FLAG;
        cmdFrame[5] = '\0';

        while(alarmCount < nRetransmissions){

            if(!alarmEnabled){
                
                int frameBytes = write(fd, cmdFrame, 5);
                printf("\n#     Sent DISC, %d bytes written\n", frameBytes);
                startAlarm(timeout);
            }
            
            int result = read(fd, responses, 5);

            cmdFrame[1] = 1;
            cmdFrame[3] = BCC(cmdFrame[1],cmdFrame[2]);
            responses[5] = '\0';

            int ans = closeStateMachine(&cmdFrame, &responses, result, fd);

            if(ans == -1){
                continue;
            }
            else if(ans == 1){
                return 1;
            }

            
        }

        if(alarmCount >= nRetransmissions){
            printf("\n#     Alarm Triggered! DISC not sent\n");
            close(fd);
            return -1;
        }


    }

    printf("\n#################################################################\n");

    if(showStatistics){
        printf("\n###########################__STATS__#############################\n\n");

        printf("\n#    Number of Packets: %d\n#    Size of data packets: %d\n#    Total time: %f\n#    Average time/packet: %f\n", lastNum, 200, runTime, runTime/200.0);

        printf("\n#################################################################\n");
    }

    return 1;
}
