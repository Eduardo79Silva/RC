// Application layer protocol implementation

#include "application_layer.h"



void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer link; // Link layer struct instance
     int stats = 1;

    strncpy(link.serialPort, serialPort, strlen(serialPort) + 1); // Copy serial port name to link layer struct

    if (strcmp("rx", role) == 0) // If role is receiver
    {
        link.role = LlRx;
    }
    else if (strcmp("tx", role) == 0) // If role is transmitter
    {
        link.role = LlTx;
    }
    else // If role is invalid
    {
        return;
    }

    link.nRetransmissions = nTries; // Copy number of retransmissions to link layer struct
    link.timeout = timeout; // Copy timeout to link layer struct
    link.baudRate = baudRate; // Copy baud rate to link layer struct

    clock_t start, final;
    
    start = clock();

    if (llopen(link) < 0) // If llopen fails
    {
        printf("\nError: Open failed!\n");
        return;
    }


    if(link.role == LlTx){

        //CREATING THE START CONTROL PACKET 
        char size[255];
        int bufSize = 0;
        struct stat file;
        stat(filename, &file);
        sprintf(size,"%02lx", file.st_size);
        int l = strlen(size)/2;
        int fileSize = file.st_size;
        unsigned char packet[MAX_PACKET_SIZE];
        unsigned char bytes[200];
        unsigned char finished = 0;
        int sizePacket = 0;
        int nBytes = 200, currentByte=0, pos=0, id = 0;
        FILE *fileptr;
        
        bufSize = getCtrlPacket(filename, 1, &packet);
        printf("Buffer size: %d\n", 5+l+strlen(filename));

        llwrite(packet, bufSize);
        printf("Start Control Packet sent!\n");

        fileptr = fopen(filename, "rb");

        while(!finished){
            if(!fread(&currentByte,(size_t)1 , (size_t)1, fileptr)){
                finished = 1;
                sizePacket = getDataPacket(bytes,&packet,id++, pos);

                if(llwrite(packet, sizePacket) == -1){
                    return;
                }
            }
            else if(nBytes == pos){
                sizePacket = getDataPacket(bytes,&packet,id++, pos);

                if(llwrite(packet,sizePacket) == -1)
                    return;
                
                pos = 0;
                memset(bytes,0,sizeof(bytes));
                memset(packet,0,sizeof(packet));
            }
            bytes[pos++] = currentByte;
        }

        fclose(fileptr);

        sizePacket = getCtrlPacket(filename,0,&packet);

        if(llwrite(packet, sizePacket) == -1){
            return;
        }

}
else{
        FILE *fileptr;
        char readBytes = 1;
        
        

        while(readBytes){
        
            unsigned char packet[600] = {0};
            int sizeOfPacket = 0, index = 0;
            
            if(llread(&packet, &sizeOfPacket)==-1){
                continue;
            }

           
            
            if(packet[0] == A_TX){
                printf("\nClosed penguin\n");
                fclose(fileptr);
                readBytes = 0;
            }
            else if(packet[0]==0x02){
                printf("\nOpened penguin\n");
                fileptr = fopen(filename, "wb");   
            }
            else{
                for(int i=4; i<sizeOfPacket; i++){
                    fputc(packet[i], fileptr);
                }
            }
        }
    }
    final = clock();
    float timeElapsed = ((float)final - start)/CLOCKS_PER_SEC;

    llclose(&stats, link, timeElapsed); // Close link layer
}



