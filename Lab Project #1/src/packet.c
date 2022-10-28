#include "packet.h"

int createCtrlPacket(char* filename, int start, unsigned char* packet){
        char size[255];
        struct stat file;
        stat(filename, &file);
        sprintf(size,"%02lx", file.st_size);
        int l = strlen(size)/2;
        int fileSize = file.st_size;

        unsigned char startCtrlPacket[MAX_PACKET_SIZE];

        if(start)
            startCtrlPacket[0] = 2; // Control camp - start
        else
            startCtrlPacket[0] = 3; // Control camp - end
        startCtrlPacket[1] = 0; // File Size
        startCtrlPacket[2] = l; // V camp size
        for(int i = l-1, j = 0; i > -1; i-- , j++){
            startCtrlPacket[3+j] = fileSize >> (8*i); // V camp 
        }
        startCtrlPacket[3+l] = 1; // File Name
        startCtrlPacket[4+l] = strlen(filename); // File name size
        for(int i = 0; i < strlen(filename); i++){
            startCtrlPacket[5+l+i] = filename[i]; //Storing file name 
        }

        for(int i = 0; i < 5+l+strlen(filename); i++){
            packet[i] = startCtrlPacket[i];
            //printf("\nStart Control Packet: %02lx\n", packet[i]);
        }
        

        int bufSize = 5+l+strlen(filename);

        return bufSize;

}

int createDataPacket(unsigned char* bytes, unsigned char* packet, int nSequence, int nBytes){

    int l1 = nBytes%BUFSIZE;
	int l2 = nBytes/BUFSIZE;

    packet[0] = 1;
	packet[1] = nSequence%255;
    packet[2] = l2;
    packet[3] = l1;

    for(int i=0; i<nBytes; i++){
        packet[i+4] = bytes[i];
    }

	return (nBytes+4); //tamanho do data packet

}
