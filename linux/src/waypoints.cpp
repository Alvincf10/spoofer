#include "waypoints.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

static int add_point(WaypointRoute *route, float lat, float lon, float speed_mps, float alt_m) {
  if (!route || route->count >= MAX_WAYPOINTS) {
    return -1;
  }

  const int i = route->count;
  route->lat[i] = lat;
  route->lon[i] = lon;
  route->speed_mps[i] = speed_mps;
  route->alt_m[i] = alt_m;
  route->count++;
  return 0;
}

static int parse_waypoint_line(const char *line, float *lat, float *lon, float *speed_mps,
                               float *alt_m) {
  *speed_mps = 0.0f;
  *alt_m = WP_NO_ALT;

  char buf[512];
  strncpy(buf, line, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  for (char *p = buf; *p; p++) {
    if (*p == ',') {
      *p = ' ';
    }
  }

  double la = 0.0;
  double lo = 0.0;
  double sp = 0.0;
  double al = 0.0;
  const int n = sscanf(buf, "%lf %lf %lf %lf", &la, &lo, &sp, &al);
  if (n >= 2) {
    *lat = static_cast<float>(la);
    *lon = static_cast<float>(lo);
    if (n >= 3) {
      *speed_mps = static_cast<float>(sp);
    }
    if (n >= 4) {
      *alt_m = static_cast<float>(al);
    }
    return 0;
  }

  const char *latp = strstr(line, "lat=\"");
  const char *lonp = strstr(line, "lon=\"");
  if (latp && lonp) {
    *lat = static_cast<float>(atof(latp + 5));
    *lon = static_cast<float>(atof(lonp + 5));
    return 0;
  }

  return -1;
}

int load_waypoint_route(const char *path, WaypointRoute *route) {
  if (!path || !route) {
    return -1;
  }

  route->count = 0;

  FILE *fp = fopen(path, "r");
  if (!fp) {
    fprintf(stderr, "waypoints: cannot open %s\n", path);
    return -1;
  }

  char line[512];
  while (fgets(line, sizeof(line), fp)) {
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
      continue;
    }

    float lat = 0.0f;
    float lon = 0.0f;
    float speed = 0.0f;
    float alt = WP_NO_ALT;
    if (parse_waypoint_line(line, &lat, &lon, &speed, &alt) != 0) {
      continue;
    }

    if (add_point(route, lat, lon, speed, alt) != 0) {
      fprintf(stderr, "waypoints: max %d points\n", MAX_WAYPOINTS);
      break;
    }
  }

  fclose(fp);

  if (route->count < 2) {
    fprintf(stderr, "waypoints: need at least 2 points in %s\n", path);
    route->count = 0;
    return -1;
  }

  return 0;
}
