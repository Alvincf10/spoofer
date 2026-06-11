#pragma once

struct SpooferConfig {
  char iface[32] = {0};
  int channel = 6;
  float latitude = -6.2088f;
  float longitude = 106.8456f;
  float latitude_b = 0.0f;
  float longitude_b = 0.0f;
  float speed_ms = 12.0f;
  int mission_loop = 1;
  int num_drones = 16;
  char region[16] = "generic";
  char waypoints[256] = {0};
  float alt_m = 50.0f;
  float alt_b_m = 0.0f;
  int pilot_follow = 0;
  int pilot_follow_set = 0;
  float pilot_offset_m = 30.0f;
  int loaded = 0;
};

int load_spoofer_config(const char *path, SpooferConfig *cfg);
int config_has_mission(const SpooferConfig *cfg);
int config_has_waypoints(const SpooferConfig *cfg);
