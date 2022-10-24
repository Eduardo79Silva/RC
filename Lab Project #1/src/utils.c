#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "macros.h"
#include "utils.h"


int BCC2stuffing(){}

unsigned char BCC2creator(unsigned char *buf,int bufSize){
    unsigned char BCC2 = 0x00;
    BCC2 = buf[0];

    for(int i = 1; i <= bufSize-1; i++){
        BCC2 ^= buf[i];
    }
    return BCC2;

}
