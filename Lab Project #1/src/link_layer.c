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

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
struct termios oldtio;
struct termios newtio;
int fd = 0;

////////////////////////////////////////////////
// FRAME BUILDER
////////////////////////////////////////////////

void frame(u_int16_t byteOne, u_int16_t byteTwo, u_int16_t byteThree, u_int16_t byteFour, u_int16_t byteFive, char buffer[])
{
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
    printf("Opening connection %s \n", connectionParameters.serialPort);

    unsigned char SET = {FLAG, A_TX, C_SET, A_TX ^ C_SET, FLAG};

    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        return (-1);
    }

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        printf("Error 1");
        perror("tcgetattr");
        return (-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 5;  // Blocking read until 5 chars received



    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        printf("Error 2");
        perror("tcsetattr");
        return (-1);
    }

    printf("New termios structure set\n");

    if (connectionParameters.role == LlRx)
    {
        receiverStart(fd);
    }
    else
    {
        senderStart(fd);
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
    // Read string from serial port
    int bytes = read(fd, packet, 256);
    printf("%d bytes read : %s\n", bytes, packet);
    
   
    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        return(-1);
    }

    close(fd);

    return 1;
}
