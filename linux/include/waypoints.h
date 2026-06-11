#pragma once

#define MAX_WAYPOINTS 64

#define WP_NO_ALT (-1.0f)

struct WaypointRoute {
  float lat[MAX_WAYPOINTS];
  float lon[MAX_WAYPOINTS];
  float speed_mps[MAX_WAYPOINTS];  // cruise speed for leg starting at this point; 0 = default
  float alt_m[MAX_WAYPOINTS];      // target AGL at point; WP_NO_ALT = default
  int count = 0;
  float default_speed_ms = 12.0f;
  float default_alt_m = 50.0f;
};

int load_waypoint_route(const char *path, WaypointRoute *route);
