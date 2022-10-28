#include "byte_stuffing.h"


int byte_stuffing(unsigned char *newBuf, int bufSize, unsigned char *frame, int idx){

    for(int i=0; i<=bufSize; i++){
        if(newBuf[i]==FLAG){
            frame[idx++]=ESC;
            frame[idx++]=0x5e;
            continue;
        }
        else if(newBuf[i]==ESC){
            frame[idx++]=ESC;
            frame[idx++]=0x5D;
            continue;
        }

        frame[idx++]=newBuf[i];
    }
    return idx;
}

int byte_destuffing(unsigned char *frame, int sizeInfo, unsigned char *packet, int idx){

    for(int i=0; i<sizeInfo; i++){
        if(frame[i] == ESC && frame[i+1]==0x5e){
            packet[idx++] = FLAG;
            i++;
        }

        else if(frame[i] == ESC && frame[i+1]==0x5d){
            packet[idx++] = ESC;
            i++;
        }

        else {packet[idx++] = frame[i];}
    }

    return idx;
}