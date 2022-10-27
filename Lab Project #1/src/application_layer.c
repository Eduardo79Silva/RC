// Application layer protocol implementation

#include "application_layer.h"



void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer link; // Link layer struct instance
    //unsigned char buf[] = {0};

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

    if (llopen(link) < 0) // If llopen fails
    {
        printf("\nError: Open failed!\n");
        return;
    }


    if(link.role == LlTx){

        //CREATING THE START CONTROL PACKET 
        char size[255];
        struct stat file;
        stat(filename, &file);
        sprintf(size,"%02lx", file.st_size);
        int l = strlen(size)/2;
        int fileSize = file.st_size;
        unsigned char packet[MAX_PACKET_SIZE], bytes[200], fileEnd = 0;
        int sizePacket = 0;
        int nBytes = 200, curByte=0, index=0, nSequence = 0;
        FILE *fileptr;
        
        int bufSize = getCtrlPacket(filename, 1, &packet);
        printf("Buffer size: %d\n", bufSize);

        llwrite(packet, bufSize);

        // while(!fileEnd){

        //     //comeco por ler a file stream
        //     //se deixar de haver coisas para ler corro este codigo
        //     if(!fread(&curByte, (size_t)1, (size_t) 1, fileptr)){
        //         fileEnd = 1;
        //         sizePacket = getDataPacket(bytes, &packet, nSequence++, index);

        //         if(llwrite(packet, sizePacket) == -1){
        //             return;
        //         }
        //     }

        //     //se o valor de index for igual a nBytes, significa que o ja passamos por nByte elementos
        //     else if(nBytes == index) {
        //         sizePacket = getDataPacket(bytes, &packet, nSequence++, index);

        //         if(llwrite(packet, sizePacket) == -1){
        //             return;
        //         }

        //         memset(bytes,0,sizeof(bytes));
        //         memset(packet,0,sizeof(packet));
        //         index = 0;
        //     }

        //     bytes[index++] = curByte;
        // }

        // fclose(fileptr);




    }


    if (llclose(0) < 0){ // If llclose fails
        printf("\nError: Close failed!\n");
        return;
    }
}



