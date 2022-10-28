#pragma once
#include "application_layer.h"

int byte_stuffing(unsigned char *newBuf, int bufSize, unsigned char *frame, int idx);

int byte_destuffing(unsigned char *frame, int sizeInfo, unsigned char *packet, int idx);