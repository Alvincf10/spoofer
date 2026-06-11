// ESP8266 RemoteID spoofer

#include "frontend.h"
#include "spoofer.h"

// Operator ID format: RID_REGION_US_FAA, RID_REGION_EU, or RID_REGION_GENERIC
#ifndef SPOOFER_RID_REGION
#define SPOOFER_RID_REGION RID_REGION_US_FAA
#endif

// for some reason I can't get vector to work so... this hacky thing for now...
int num_spoofers = 0;
Spoofer spoofers[16];

void setup() {
  Serial.begin(115200);
  
  // start the frontend and don't exit until either timer elapse or user ends it
  // the timer here is set at 2 minutes
  Frontend frontend(120000);
  while (!frontend.do_spoof) {
    frontend.handleClient();
  }

  // instantiate the spoofers and change locations
  Serial.println("Starting Spoofers");
  num_spoofers = frontend.num_drones;
  for (int i = 0; i < num_spoofers; i++) {
    spoofers[i].init(i, (RidRegion) SPOOFER_RID_REGION);
    spoofers[i].updateLocation(frontend.latitude, frontend.longitude);
  }
}

void loop() {
  // do the spoofing
  for (int i = 0; i < num_spoofers; i++) {
    spoofers[i].update();
    delay(1000 / num_spoofers);
  }
}
