#pragma once
#include <Arduino.h>
#include "config.h"

struct Schedule {
  bool enabled = false;
  int onHour = 18;
  int onMinute = 0;
  int offHour = 6;
  int offMinute = 0;
};

void relayInit();
void relaySet(int channel, bool on);
void relayToggle(int channel);
bool relayGet(int channel);
void relayAllOff();
void relaySaveState();
void relayLoadState();

// Scheduler
void scheduleInit();
Schedule scheduleGet(int channel);
void scheduleSet(int channel, Schedule s);
void scheduleSave();
void scheduleLoad();
void scheduleCheck(int hour, int minute);
