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
volatile int STOP = FALSE;

////////////////////////////////////////////////
// STATE MACHINE
////////////////////////////////////////////////

int stateMachine(char *buffer, int length, u_int16_t ctrl)
{
    int currentByte = 0;

    int state = 0; // 0 = Start; 1 = FLAG; 2 = A; 3 = C; 4 = BCC; 5 = STOP

    while (currentByte < length)
    {
        switch (state)
        {
        case 0:
            if (buffer[currentByte] == FLAG)
            {
                state = 1;
            }
            currentByte++;
            break;
        case 1:
            if (buffer[currentByte] == A)
                state = 2;
            else if (buffer[currentByte] != FLAG)
                state = 0;
            
            currentByte++;
            break;
        case 2:
            if (buffer[currentByte] == ctrl)
                state = 3;
            else if (buffer[currentByte] == FLAG)
                state = 1;
            else
                state = 0;

            currentByte++;
            break;

        case 3:
            if (buffer[currentByte] == buffer[currentByte - 1] ^ buffer[currentByte - 2])
                state = 4;
            else if (buffer[currentByte] == FLAG)
                state = 1;
            else
                state = 0;

            currentByte++;
            break;

        case 4:
            if (buffer[currentByte] == FLAG)
                return TRUE;
            else
                state = 0;

            currentByte++;
            break;

        default:
            break;
        }
    }
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

        // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 1;  // Blocking read until 5 chars received

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
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

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
    // Loop for input
    unsigned char buf[256 + 1] = {0}; // +1: Save space for the final '\0' char

    while (STOP == FALSE)
    {
        // Returns after 5 chars have been input
        int bytes = read(fd, buf, 256);
        buf[bytes] = '\0'; // Set end of string to '\0', so we can printf

        printf(":%s:%d\n", buf, bytes);
        if (buf[0] == 'z')
            STOP = TRUE;
    }

    // The while() cycle should be changed in order to respect the specifications
    // of the protocol indicated in the Lab guide

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
        exit(-1);
    }

    close(fd);

    return 1;
}
