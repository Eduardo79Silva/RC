// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include "macros.h"
#include "string.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer link;
    unsigned char buf[] = {0};

    strncpy(link.serialPort, serialPort, strlen(serialPort) + 1);

    if (strcmp("rx", role) == 0)
    {
        link.role = LlRx;
    }
    else if (strcmp("tx", role) == 0)
    {
        link.role = LlTx;
    }
    else
    {
        return;
    }

    link.nRetransmissions = nTries;
    link.timeout = timeout;
    link.baudRate = baudRate;

    if (llopen(link) < 0)
    {
        printf("\nError: Open failed!\n");
        return;
    }

    if (llclose(0) < 0)
    {
        printf("\nError: Close failed!\n");
        return;
    }
}
