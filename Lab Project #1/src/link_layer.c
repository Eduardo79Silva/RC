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

    printf("Opening connection on port: %s \n", connectionParameters.serialPort);

    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        return -1;
    }

    // Save current port settingsS
    if (tcgetattr(fd, &oldtio) == -1)
    {
        printf("Error saving port settings\n");
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
        printf("Error setting port settings\n");
        perror("tcsetattr");
        return -1;
    }

    printf("New termios structure set\n");

    // Start either transmitter or receiver

    if (connectionParameters.role == LlTx) {
        
        unsigned char buf[5] = {0};
        unsigned char c;
        STATE state = START;

        buf[0] = FLAG; // FLAG
        buf[1] = A_TX; // A
        buf[2] = C_SET; // C
        buf[3] = BCC(buf[1], buf[2]); // BCC
        buf[4] = FLAG; // FLAG

        while(alarmCount < connectionParameters.nRetransmissions){
            if(!alarmEnabled){
                state = START;
                if (write(fd,buf, 5) == -1){
                    printf("ERROR: Failed to write\n");
                    return -1;
                }
                startAlarm(connectionParameters.timeout);
            }

            int frameBytes = read(fd,&c,1);

            if(frameBytes > -1){
                stateMachine(&c, &state, C_UA);
            }

            if(state == STOP_ST){
                printf("UA RECEIVED\n");
                disableAlarm();
                break;
            }

        }

    } else if(connectionParameters.role == LlRx) {

        unsigned char buf[5] = {0};
        unsigned char c;
        STATE state = START;

        buf[0] = FLAG; // FLAG
        buf[1] = A_TX; // A
        buf[2] = C_UA; // C
        buf[3] = BCC(buf[1], buf[2]); // BCC
        buf[4] = FLAG; // FLAG

        //NÃO PRECISA DE UM ALARM COUNT POIS SE O TRANSMITER NÃO RECEBER O UA ELE DISPARA O ALARME

        while(state != STOP_ST){
            int frameBytes = read(fd,&c,1);
            if(frameBytes > -1){
                stateMachine(&c,&state,C_SET);
            }
        }

        int frameBytes = write(fd,buf,5);
        printf("UA response sent, %d bytes written\n", frameBytes);


    }

    
    return 1;
}




////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////

int llwrite(const unsigned char *buf, int bufSize){

    unsigned char frame[MAX_PAYLOAD_SIZE+6] = {0};
    unsigned char BCC2 = BCC2creator(buf,bufSize);
    unsigned char BCC = 0x00, parcels[5] = {0};
    int STOP = 0, ctrlRX = (!NS << 7) | C_RR0;
    int idx = 4;

    frame[0] = FLAG;
    frame[1] = A_TX;
    frame[2] = (NS << 6);
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
            printf("\nControl Packet sent NS=%d\n", NS);
            startAlarm(timeout);
        }
        
        int result = read(fd, parcels, 5);

        if(result != -1 && parcels != 0){

            if(parcels[2] != (ctrlRX) || (parcels[3] != (parcels[1]^parcels[2]))){
                    printf("\nRR not correct: 0x%02x%02x%02x%02x%02x\n", parcels[0], parcels[1], parcels[2], parcels[3], parcels[4]);
                    alarmEnabled = FALSE;
                    continue;
            }
            
            else{
                printf("\nRR correctly received: 0x%02x%02x%02x%02x%02x\n", parcels[0], parcels[1], parcels[2], parcels[3], parcels[4]);
                alarmEnabled = FALSE;
                STOP = 1;
            }
        }

        if(alarmCount >= nRetransmissions){
            printf("\n%d\n", nRetransmissions);
            printf("\nllwrite error: Exceeded number of tries when sending frame\n");
            STOP = 1;
            close(fd);
            return -1;
        }
        
    }

    printf("\nllwrite: Frame sent successfully\n");


    if(NS){
        NS = 0;
    }
    else {NS = 1;}

    return 0;
}


////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet, int *sizeOfPacket)
{   
    
    printf("\n------------------------------LLREAD------------------------------\n\n");

    unsigned char frame[600]={0}, responseFrame[5]={0}, BCC2=0x00, aux[400] = {0}, flagCount = 0, STOP = FALSE; 
    int control = (!NR) << 6, index = 0, sizeInfo = 0;

    
    unsigned char buf[1] = {0}; 

    STATE st = START;
    unsigned char reading = TRUE;
    
    
    while (!STOP)
    { 
        if(reading){
            int frameBytes = read(fd, buf, 1); 
            if(frameBytes==-1 || frameBytes == 0) continue; 
            
        }
    
        
        switch (st)
        {
        case START:
            if(buf[0] == FLAG){
                st = FLAG_RCV;
                frame[sizeInfo++] = buf[0];
            }
            break;

        case FLAG_RCV:
            if(buf[0] != FLAG){
            
                st = A_RCV;
                frame[sizeInfo++] = buf[0];
            }
            else{
                memset(frame, 0, 600);
                st = FLAG_RCV;
                sizeInfo = 0;
                frame[sizeInfo++] = buf[0];
            }
            break;

        case A_RCV:
             if(buf[0] != FLAG){
                frame[sizeInfo++] = buf[0];
            }
            else if(buf[0] == FLAG){
            
                STOP = TRUE;
                frame[sizeInfo++] = buf[0];
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

    for(int i = 0; i < sizeInfo; i++){
        printf("%02lx \n", frame[i]);
    }

    if((frame[1]^frame[2]) != frame[3] || frame[2] != control){
        printf("\nInfoFrame not received correctly. Protocol error. Sending REJ.\n");
        responseFrame[2] = (NR << 7) | 0x01;
        responseFrame[3] = responseFrame[1] ^ responseFrame[2];
        write(fd, responseFrame, 5);

        printf("\n-----REJ-----\n");
        printf("\nSize of REJ: %d\nREJ: 0x", 5);

        for(int i=0; i<5; i++){
            printf("%02X ", responseFrame[i]);
        }

        printf("\n\n");

        return -1;
    }


    //Destuffing

    for(int i=0; i<sizeInfo; i++){
        if(frame[i] == 0x7D && frame[i+1]==0x5e){
            packet[index++] = FLAG;
            i++;
        }

        else if(frame[i] == 0x7D && frame[i+1]==0x5d){
            packet[index++] = 0x7D;
            i++;
        }

        else {packet[index++] = frame[i];}
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
                printf("\nInfoFrame received correctly. Repeated Frame. Sending RR.\n");
                responseFrame[2] = (NR << 7) | C_RR0;
                responseFrame[3] = responseFrame[1] ^ responseFrame[2];
                write(fd, responseFrame, 5);
                return -1;
            }   
            else{
                lastNum = frame[5];
            }
        }
        printf("\nInfoFrame received correctly. Sending RR.\n");
        responseFrame[2] = (NR << 7) | C_RR0;
        responseFrame[3] = responseFrame[1] ^ responseFrame[2];
        write(fd, responseFrame, 5);
    }
    
    else {
        printf("\nInfoFrame not received correctly. Error in data packet. Sending REJ.\n");
        responseFrame[2] = (NR << 7) | 0x01;
        responseFrame[3] = responseFrame[1] ^ responseFrame[2];
        write(fd, responseFrame, 5);

        return -1;
    }

    (*sizeOfPacket) = size;

    //--------------------Reset Variables--------------------

    index = 0;
    
    for(int i=4; i<(*sizeOfPacket)-2; i++){
        aux[index++] = packet[i];
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

    return 1;
}


////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics, LinkLayer connectionParameters, float runTime)
{       
    alarmCount = 0;

    printf("\n------------------------------LLCLOSE------------------------------\n\n");

    if(connectionParameters.role == LlRx){

        unsigned char buf[6] = {0}, parcels[6] = {0};
        unsigned char STOP = 0, UA = 0;

        buf[0] = FLAG;
        buf[1] = A_TX;
        buf[2] = DISC;
        buf[3] = buf[1]^buf[2];
        buf[4] = FLAG;
        buf[5] = '\0';


        while(!STOP){
            int result = read(fd, parcels, 5);
            
            parcels[5] = '\0';

            if(result==-1){
                continue;
            }


            else if(strcasecmp(buf, parcels) == 0){
                printf("\nDISC message received. Responding now.\n");
                
                buf[1] = 0x01;
                buf[3] = buf[1]^buf[2];

                while(alarmCount < nRetransmissions){

                    if(!alarmEnabled){
                        printf("\nDISC message sent, %d bytes written\n", 5);
                        write(fd, buf, 5);
                        startAlarm(timeout);
                    }
                    
                    int result = read(fd, parcels, 5);
                    if(result != -1 && parcels != 0 && parcels[0]==FLAG){
                        if(parcels[2] != 0x07 || (parcels[3] != (parcels[1]^parcels[2]))){
                            printf("\nUA not correct: 0x%02x%02x%02x%02x%02x\n", parcels[0], parcels[1], parcels[2], parcels[3], parcels[4]);
                            alarmEnabled = FALSE;
                            continue;
                        }
                        
                        else{   
                            printf("\nUA correctly received: 0x%02x%02x%02x%02x%02x\n", parcels[0], parcels[1], parcels[2], parcels[3], parcels[4]);
                            alarmEnabled = FALSE;
                            close(fd);
                            break;
                        }
                    }

                }

                if(alarmCount >= nRetransmissions){
                    printf("\nAlarm limit reached, DISC message not sent\n");
                    return -1;
                }
                
                STOP = TRUE;
            }
        
        }

    }

    else{
        alarmCount = 0;

        unsigned char buf[6] = {0}, parcels[6] = {0};

        buf[0] = FLAG;
        buf[1] = A_TX;
        buf[2] = DISC;
        buf[3] = buf[1]^buf[2];
        buf[4] = FLAG;
        buf[5] = '\0';

        while(alarmCount < nRetransmissions){

            if(!alarmEnabled){
                
                int frameBytes = write(fd, buf, 5);
                printf("\nDISC message sent, %d bytes written\n", frameBytes);
                startAlarm(timeout);
            }
            
            int result = read(fd, parcels, 5);

            buf[1] = 0x01;
            buf[3] = buf[1]^buf[2];
            parcels[5] = '\0';

            if(result != -1 && parcels != 0 && parcels[0]==FLAG){
                if(strcasecmp(buf, parcels) != 0){
                    printf("\nDISC not correct: 0x%02x%02x%02x%02x%02x\n", parcels[0], parcels[1], parcels[2], parcels[3], parcels[4]);
                    alarmEnabled = FALSE;
                    continue;
                }
                
                else{   
                    printf("\nDISC correctly received: 0x%02x%02x%02x%02x%02x\n", parcels[0], parcels[1], parcels[2], parcels[3], parcels[4]);
                    alarmEnabled = FALSE;
                    
                    buf[1] = 0x01;
                    buf[2] = 0x07;
                    buf[3] = buf[1]^buf[2];

                    int frameBytes = write(fd, buf, 5);

                    close(fd);

                    printf("\nUA message sent, %d bytes written.\n\nI'm shutting off now, bye bye!\n", frameBytes);
                    return 1;

                }
            }

        }

        if(alarmCount >= nRetransmissions){
            printf("\nAlarm limit reached, DISC message not sent\n");
            close(fd);
            return -1;
        }


    }

    if(showStatistics){
        printf("\n------------------------------STATISTICS------------------------------\n\n");

        printf("\nNumber of packets sent: %d\nSize of data packets in information frame: %d\nTotal run time: %f\nAverage time per packet: %f\n", lastNum, 200, runTime, runTime/200.0);

    }

    return 1;
}
