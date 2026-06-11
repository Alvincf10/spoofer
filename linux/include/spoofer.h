#pragma once

#include <cmath>
#include <cstdint>
#include <ctime>
#include <sys/time.h>

#include <Arduino.h>
#include "drone_profiles.h"
#include "id_open.h"
#include "utm.h"
#include "waypoints.h"

#ifndef SPOOFER_H
#define SPOOFER_H

enum class FlightMode {
  PATROL,
  MISSION,
  WAYPOINT,
};

class Spoofer {
 private:
  ID_OpenDrone squitter;
  UTM_Utilities utm_utils;

  struct UTM_parameters utm_parameters;
  struct UTM_data utm_data;

  const DroneProfile *active_profile = nullptr;
  RidRegion rid_region = RID_REGION_US_FAA;
  int drone_index = 0;
  FlightMode flight_mode = FlightMode::PATROL;

  const double angle_rad2deg = 180.0 / M_PI;
  const double speed_ms2kn = 1.9438452;
  const double max_accel = 10 * 1000;
  const double max_speed = 25;
  const double max_climbrate = 10 * 1000;
  const double max_height = 200;

  float speed_m_x = 0.0f, speed_m_y = 0.0f;
  double m_deg_lat = 0.0, m_deg_long = 0.0;
  float x = 0.0f, y = 0.0f, z = 0.0f;
  float pilot_x = 0.0f, pilot_y = 0.0f;
  float last_climb_rate = 0.0f;

  double lat_a = 0.0, lon_a = 0.0, lat_b = 0.0, lon_b = 0.0;
  float mission_progress_m = 0.0f;
  float mission_length_m = 1.0f;
  float mission_dir_x = 1.0f, mission_dir_y = 0.0f;
  float mission_perp_x = 0.0f, mission_perp_y = 1.0f;
  float lane_offset_m = 0.0f;
  float cruise_speed_mps = 12.0f;
  float default_speed_mps = 12.0f;
  float default_alt_m = 50.0f;
  float leg_alt_start_m = 50.0f;
  float leg_alt_end_m = 50.0f;
  float mission_alt_a_m = 0.0f;
  float mission_alt_b_m = 0.0f;
  bool mission_loop = true;
  bool mission_forward = true;
  bool pilot_follow = false;
  float pilot_offset_m = 30.0f;

  static constexpr int kMaxWaypoints = 64;
  float wp_lat[kMaxWaypoints];
  float wp_lon[kMaxWaypoints];
  float wp_speed[kMaxWaypoints];
  float wp_alt[kMaxWaypoints];
  int wp_count = 0;
  int wp_leg = 0;

  time_t time_2;
  struct tm clock_tm;
  struct timeval tv = {0, 0};
  struct timezone utc = {0, 0};
  uint32_t last_update = 0;

  void applyProfile(const DroneProfile *profile);
  void syncClock();
  void updatePilot();
  void updatePilotFollow();
  void updateLegAltitude(float time_elapsed_secs);
  float effectiveWpAlt(int index) const;
  void recomputeMissionVectors();
  void updatePatrol(float time_elapsed_secs);
  void updateMission(float time_elapsed_secs);
  void updateWaypoint(float time_elapsed_secs);
  void beginLeg(int leg_index);

 public:
  void init(int index = 0, RidRegion region = RID_REGION_US_FAA);
  void updateLocation(float latitude, float longitude);
  void setPilotFollow(bool enable, float offset_m = 30.0f);
  void setDefaultAlt(float alt_m);
  void setMission(float lat_start, float lon_start, float lat_end, float lon_end,
                  float speed_mps = 12.0f, bool loop = true, float alt_a_m = 0.0f,
                  float alt_b_m = 0.0f);
  void setWaypointRoute(const WaypointRoute *route, bool loop = true);
  void update();
  const DroneProfile *profile() const { return active_profile; }
  FlightMode mode() const { return flight_mode; }
};

#endif
