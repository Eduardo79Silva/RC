// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include "macros.h"
#include "string.h"
#include <stdio.h>


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer link; // Link layer struct instance
    unsigned char buf[] = {0};

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
        
        unsigned char startCtrlPacket[MAX_PAYLOAD_SIZE];
        int fileSize = ftell(filename);
        int l = 0;

        while(fileSize > 1){
            fileSize = fileSize / 10;
            l++;
        }

        char size[l];

        itoa(fileSize,size,l);

        startCtrlPacket[0] = 2; // Control camp - start
        startCtrlPacket[1] = 0; // File Size
        startCtrlPacket[2] = l; // V camp size
        for(int i = 0; i < l; i++){
            startCtrlPacket[3+i] = size[i]; // V camp 
        }
        startCtrlPacket[3+l] = 1; // File Name
        startCtrlPacket[4+l] = strlen(filename); // File name size
        for(int i = 0; i < strlen(filename); i++){
            startCtrlPacket[5+l+i] = filename[i]; //Storing file name 
        }

        int bufSize = 5+l+strlen(filename);

        llwrite(startCtrlPacket, bufSize);




    }


    if (llclose(0) < 0){ // If llclose fails
        printf("\nError: Close failed!\n");
        return;
    }
}
