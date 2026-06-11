#pragma once

#include <cmath>
#include <cstdint>
#include <ctime>
#include <string>
#include <sys/time.h>

#include <Arduino.h>
#include "id_open.h"
#include "utm.h"

#ifndef SPOOFER_H
#define SPOOFER_H

class Spoofer {
 private:
  ID_OpenDrone squitter;
  UTM_Utilities utm_utils;

  struct UTM_parameters utm_parameters;
  struct UTM_data utm_data;

  const double angle_rad2deg = 180.0 / M_PI;
  const double speed_ms2kn = 1.9438452;
  const double max_accel = 10 * 1000;
  const double max_speed = 25;
  const double max_climbrate = 10 * 1000;
  const double max_height = 200;

  float speed_m_x = 0.0f, speed_m_y = 0.0f;
  double m_deg_lat = 0.0, m_deg_long = 0.0;
  float x = 0.0f, y = 0.0f, z = 0.0f;

  time_t time_2;
  struct tm clock_tm;
  struct timeval tv = {0, 0};
  struct timezone utc = {0, 0};
  uint32_t last_update = 0;

  std::string getID();

 public:
  void init();
  void updateLocation(float latitude, float longitude);
  void update();
};

#endif
