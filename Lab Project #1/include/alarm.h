#pragma once

#include "macros.h"

extern int alarmEnabled;
extern int alarmCount;

void disableAlarm();

void alarmHandler(int signal);

int startAlarm(unsigned int seconds);