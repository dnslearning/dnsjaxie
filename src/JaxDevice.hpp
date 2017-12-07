
#pragma once

typedef struct JaxDevice JaxDevice;

struct JaxDevice {
  int time;
  int id;
  bool study;
  int blockads;
  int lastActivity;
};
