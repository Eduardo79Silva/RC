// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include "macros.h"
#include "string.h"

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

    if (llclose(0) < 0){ // If llclose fails
        printf("\nError: Close failed!\n");
        return;
    }
}
