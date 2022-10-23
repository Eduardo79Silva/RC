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

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

struct termios oldtio;
struct termios newtio;
int fd = 0;

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

    if (connectionParameters.role == LlRx) {
        if (!receiverStart(fd,connectionParameters.nRetransmissions)){
            printf("ERROR RECEIVING DATA");
            return -1;
        } 
    } else {
        if (!senderStart(fd, connectionParameters.nRetransmissions)){
            printf("Error sending data\n");
            return -1;
        }
    }

    
    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////

int llwrite(const unsigned char *buf, int bufSize)
{
    // Create string to send

    printf("Enter a string : \n");
    gets(buf);

    printf("\nYou entered: %s\n", buf);

    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.

    int bytes = write(fd, buf, 256);
    printf("%d bytes written\n", strlen(buf));

    // Wait until all bytes have been written to the serial port
    sleep(1);

    return 0;
}


////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////

int llread(unsigned char *packet)
{
    
   
    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////

int llclose(int showStatistics)
{
    // Close serial port (Given by the professor)
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        return(-1);
    }

    close(fd);

    return 1;
}
