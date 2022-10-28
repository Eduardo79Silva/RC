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
int fd = 0;

int nRetransmissions = 0;
int timeout = 0;

int NS = 0;
int NR = 1;

extern int alarmEnabled;

//Delete
int nTries, timeout, fd, lastFrameNumber = -1;

////////////////////////////////////////////////
// FRAME BUILDER
////////////////////////////////////////////////

void frame(u_int16_t byteOne, u_int16_t byteTwo, u_int16_t byteThree, u_int16_t byteFour, u_int16_t byteFive, char buffer[]){ // Builds a frame

    buffer[0] = byteOne;
    buffer[1] = byteTwo;
    buffer[2] = byteThree;
    buffer[3] = byteFour;
    buffer[4] = byteFive;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

int llopen(LinkLayer connectionParameters)
{

    nRetransmissions = connectionParameters.nRetransmissions;
    timeout = connectionParameters.timeout;

    // Open serial port device for reading and writing (Given by the professor)

    printf("Opening connection %s \n", connectionParameters.serialPort);

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

            int bytes = read(fd,&c,1);

            if(bytes > -1){
                printf("RECEIVED: %x\n", c);
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
            int bytes = read(fd,&c,1);
            if(bytes > -1){
                printf("RECEIVED: %x\n", c);
                stateMachine(&c,&state,C_SET);
            }
        }

        int bytes = write(fd,buf,5);
        printf("UA response sent, %d bytes written\n", bytes);


    }

    
    return 1;
}










////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////

int llwrite(const unsigned char *buf, int bufSize){

    unsigned char frame[MAX_PAYLOAD_SIZE+6] = {0};
    unsigned char BCC2 = BCC2creator(buf,bufSize);
    unsigned char BCC = 0x00, infoFrame[600] = {0}, parcels[5] = {0};
    int index = 4, STOP = 0, controlReceiver = (!NS << 7) | 0x05;
    int idx = 4;
    // printf("Printing buffer\n");
    // for(int i = 0; i < bufSize; i++){
    //     printf("%x \n", buf[i]);
    // }

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
    //printf("%x \n", BCC2);


    for(int i=0; i<=bufSize; i++){
        if(newBuffer[i]==0x7E){
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


    
    //byteStuffing(&newBuffer, newSize);

    // for(int i =0 ; i <= newSize; i++){
    //     frame[idx++] = newBuffer[i];
    // }    
    
    frame[idx++] = FLAG;
    // printf("Printing frame\n"); 
    // for(int i = 0; i < idx; i++){
    //     printf("%02lx \n", frame[i]);
    // }
    
   // frame = (unsigned char *) realloc(frame, (newSize+4)* sizeof(unsigned char));


    int rejected = FALSE;
    int rejectedFlag = 0;
    unsigned char c;
    STATE state = START;
    unsigned int ctrl_camp = NULL;

    while(!STOP){
        if(!alarmEnabled){
            write(fd, frame, idx);
            printf("\nControl Packet sent NS=%d\n", NS);
            startAlarm(timeout);
        }
        
        int result = read(fd, parcels, 5);

        if(result != -1 && parcels != 0){
            /*alarmEnabled = FALSE;
            return 1;*/

            if(parcels[2] != (controlReceiver) || (parcels[3] != (parcels[1]^parcels[2]))){
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
        
        // if(STOP == 1){
        //     break;
        // }
    }

    printf("\nllwrite: Frame sent successfully\n");


    if(NS){
        NS = 0;
    }
    else {NS = 1;}

    return 0;

    // while(alarmCount < nRetransmissions){
    //     if(!alarmEnabled){
    //         if (write(fd,buf, 5+newSize) == -1){
    //             printf("ERROR: Failed to write\n");
    //             return -1;
    //         }
    //         startAlarm(timeout);
    //     }


    //     int bytes = read(fd,&c,1);
    //     if (bytes == 0){
    //         continue;
    //     }

    //     if(bytes > -1){
    //        // printf("RECEIVED: %x\n", c);
    //         if(c == C_RR0 || c == C_RR1 || c == DISC || c == C_REJ0 || c == C_REJ1){
    //             if(readCtrlMessage(&c, &state, ctrl_camp) == -1){
    //                 ctrl_camp = c;
    //                 rejectedFlag = -1;
    //             }
    //         }else{
    //             if(readCtrlMessage(&c, &state, ctrl_camp) == -1){
    //                 rejectedFlag = -1;
    //             }
    //         }
    //     }



    //     if(state == STOP_ST){
    //         //printf("RECEIVED READY\n");
    //         if(c == C_RR0 && NS == 0x40){
    //             NS = 0X00;
    //         }
    //         else if(c == C_RR1 && NS == 0x00){
    //             NS = 0x40;
    //         }
            
    //         if(rejectedFlag == -1){
    //             rejected = TRUE;
    //         }
    //         else{
    //             disableAlarm();
    //             break;
    //         }
    //     }

    //     if(rejected == TRUE){
    //         if (write(fd,buf, 5+newSize) == -1){
    //             printf("ERROR: Failed to write\n");
    //             return -1;
    //         }
    //         rejected = FALSE;
    //     }


    //}

    return (newSize+5);



}


////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet, int *sizeOfPacket)
{   
    /*codigo testado e a funcionar como pretendido*/
    
    printf("\n------------------------------LLREAD------------------------------\n\n");

    unsigned char infoFrame[600]={0}, supFrame[5]={0}, BCC2=0x00, aux[400] = {0}, flagCount = 0, STOP = FALSE; 
    int control = (!NR) << 6, index = 0, sizeInfo = 0;

    
    unsigned char buf[1] = {0}; // +1: Save space for the final '\0' char

    STATE st = START;
    unsigned char readByte = TRUE;
    
    
    // Loop for input
    while (!STOP)
    { 
        if(readByte){
            int bytes = read(fd, buf, 1); //ler byte a byte
            if(bytes==-1 || bytes == 0) continue; // se der erro a leitura ou se tiver lido 0 bytes continuo para a próxima iteraçao
            
        }
    
        
        switch (st)
        {
        case START:
            //state0 porque a primeira coisa que quero receber é a FLAG. quando tenho confirmaçao que recebi a flag passo ao proximo estado.
            if(buf[0] == 0x7E){
                st = FLAG_RCV;
                infoFrame[sizeInfo++] = buf[0];
            }
            //se nao receber a flag significa que estou a ler um byte aleatorio que eu nao quero
            break;

        case FLAG_RCV:
            //no STATE1 eu quero receber qualquer coisa exceto a flag, porque pode dar-se o caso que eu leio 2 FLAGS seguidas (a de fim de uma trama e a de inicio de outra)
            // ou seja, EU NAO QUERO EM QUALQUER CASO LER 0X7E DUAS VEZES SEGUIDINHAS porque isso significa que esta a ler duas tramas diferentes
            //por isso este estado serve só para receber um byte logo a seguir a flag, que seja diferente de 0x7E, assim que isso acontecer passo para STATE2
            if(buf[0] != 0x7E){
            
                st = A_RCV;
                infoFrame[sizeInfo++] = buf[0];
            }
            //Se eu ler duas FLAGS seguidas sei que a flag que acabei de ler é o inico de uma nova tram de info por isso posso cortar o primeiro passo de receber a flag e continuar no STATE1 em que a proxima coisa que vem depois de uma flag é qualquer numero EXCETO a flag
            else{
                memset(infoFrame, 0, 600);
                st = FLAG_RCV;
                sizeInfo = 0;
                infoFrame[sizeInfo++] = buf[0];
            }
            break;

        case A_RCV:
            //no state2 eu ja garanti que estou a ler uma unica trama de informaçao e nao o cu e a cabeça de duas diferentes, por isso continuo no STATE2 até recebr uma flag (ou seja ate acabar a trama de info)
            if(buf[0] != 0x7E){
                infoFrame[sizeInfo++] = buf[0];
            }
            //quando receber a flag significa que a trama de info acabou e por isso posso sair da stateMachine e comecou a verificar se aquilo que recebi nao foi corrompido ou algo do genero
            else if(buf[0] == 0x7E){
            
                STOP = TRUE;
                infoFrame[sizeInfo++] = buf[0];
                readByte = FALSE;
            }
            break;
        
        default:
            break;
        }
    }

    //1º ler o pipe
    //2º fazer de-stuff aos bytes lidos
    //3º verificar que os BCCs estao certos
    //4º enviar a mensagem de confirmacao de receçao, positiva se correu tudo bem, negativa se BCC ou assim esta mal
    
    //fazer as contas para confirmar o valor max do buffer
    
    supFrame[0] = 0x7E;
    supFrame[1] = 0x03;
    
    supFrame[4] = 0x7E;

    for(int i = 0; i < sizeInfo; i++){
        printf("%02lx \n", infoFrame[i]);
    }

    if((infoFrame[1]^infoFrame[2]) != infoFrame[3] || infoFrame[2] != control){
        printf("\nInfoFrame not received correctly. Protocol error. Sending REJ.\n");
        supFrame[2] = (NR << 7) | 0x01;
        supFrame[3] = supFrame[1] ^ supFrame[2];
        write(fd, supFrame, 5);

        printf("\n-----REJ-----\n");
        printf("\nSize of REJ: %d\nREJ: 0x", 5);

        for(int i=0; i<5; i++){
            printf("%02X ", supFrame[i]);
        }

        printf("\n\n");

        return -1;
    }

    /*printf("\n-----llread 422 infoFrame-----\n");
    printf("\nSize of P: %d\nInfoFrame: 0x", sizeInfo);
    for(int i=0; i<sizeInfo; i++){
        printf("%02X ", infoFrame[i]);
    }
    printf("\n\n");*/

    //Destuffing

    for(int i=0; i<sizeInfo; i++){
        if(infoFrame[i] == 0x7D && infoFrame[i+1]==0x5e){
            packet[index++] = 0x7E;
            i++;
        }

        else if(infoFrame[i] == 0x7D && infoFrame[i+1]==0x5d){
            packet[index++] = 0x7D;
            i++;
        }

        else {packet[index++] = infoFrame[i];}
    }

    //--------------------BCC2--------------------

    int size = 0; //tamanho da secçao de dados

    if(packet[4]==0x01){
        size = 256*packet[6]+packet[7]+4 +6; //+4 para contar com os bytes de controlo, numero de seq e tamanho
        for(int i=4; i<size-2; i++){
            BCC2 = BCC2 ^ packet[i];
        }
    }
    
    else{
        size += packet[6]+ 3 + 4; //+3 para contar com os bytes de C, T1 e L1 // +4 para contar com os bytes FLAG, A, C, BCC
        size += packet[size+1] + 2 +2; //+2 para contar com T2 e L2 //+2 para contar com BCC2 e FLAG

        for(int i=4; i<size-2; i++){
            BCC2 = BCC2 ^ packet[i];
        }
    }


    //printf("%02X\n", BCC2);

    if(packet[size-2] == BCC2){

        if(packet[4]==0x01){
            if(infoFrame[5] == lastFrameNumber){
                printf("\nInfoFrame received correctly. Repeated Frame. Sending RR.\n");
                supFrame[2] = (NR << 7) | 0x05;
                supFrame[3] = supFrame[1] ^ supFrame[2];
                write(fd, supFrame, 5);
                return -1;
            }   
            else{
                lastFrameNumber = infoFrame[5];
            }
        }
        printf("\nInfoFrame received correctly. Sending RR.\n");
        supFrame[2] = (NR << 7) | 0x05;
        supFrame[3] = supFrame[1] ^ supFrame[2];
        write(fd, supFrame, 5);
    }
    
    else {
        printf("\nInfoFrame not received correctly. Error in data packet. Sending REJ.\n");
        supFrame[2] = (NR << 7) | 0x01;
        supFrame[3] = supFrame[1] ^ supFrame[2];
        write(fd, supFrame, 5);

        /*printf("\n-----REJ-----\n");
        printf("\nSize of REJ: %d\nREJ: 0x", 5);

        for(int i=0; i<5; i++){
            printf("%02X ", supFrame[i]);
        }

        printf("\n\n");*/

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

        buf[0] = 0x7E;
        buf[1] = 0x03;
        buf[2] = 0x0B;
        buf[3] = buf[1]^buf[2];
        buf[4] = 0x7E;
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
                    if(result != -1 && parcels != 0 && parcels[0]==0x7E){
                        //se o UA estiver errado 
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

        buf[0] = 0x7E;
        buf[1] = 0x03;
        buf[2] = 0x0B;
        buf[3] = buf[1]^buf[2];
        buf[4] = 0x7E;
        buf[5] = '\0'; //assim posso usar o strcmp

        while(alarmCount < nRetransmissions){

            if(!alarmEnabled){
                
                int bytes = write(fd, buf, 5);
                printf("\nDISC message sent, %d bytes written\n", bytes);
                startAlarm(timeout);
            }

            //sleep(2);
            
            int result = read(fd, parcels, 5);

            buf[1] = 0x01;
            buf[3] = buf[1]^buf[2];
            parcels[5] = '\0';

            if(result != -1 && parcels != 0 && parcels[0]==0x7E){
                //se o DISC estiver errado 
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

                    int bytes = write(fd, buf, 5);

                    close(fd);

                    printf("\nUA message sent, %d bytes written.\n\nI'm shutting off now, bye bye!\n", bytes);
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

        printf("\nNumber of packets sent: %d\nSize of data packets in information frame: %d\nTotal run time: %f\nAverage time per packet: %f\n", lastFrameNumber, 200, runTime, runTime/200.0);

    }

    return 1;
}
