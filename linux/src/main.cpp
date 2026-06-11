#include <Arduino.h>

#include <cmath>
#include <cstdlib>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <vector>

#include "config.h"
#include "drone_profiles.h"
#include "spoofer.h"
#include "waypoints.h"
#include "wifi_tx.h"

namespace {

volatile sig_atomic_t keep_running = 1;

void handle_signal(int) { keep_running = 0; }

RidRegion parse_region(const char *value) {
  if (!value) {
    return RID_REGION_GENERIC;
  }
  if (strcasecmp(value, "us") == 0 || strcasecmp(value, "faa") == 0) {
    return RID_REGION_US_FAA;
  }
  if (strcasecmp(value, "eu") == 0) {
    return RID_REGION_EU;
  }
  return RID_REGION_GENERIC;
}

void usage(const char *prog) {
  fprintf(stderr,
          "Remote ID Spoofer — Alfa RTL8814AU / Linux\n"
          "\n"
          "Usage: %s -i <iface> [options]\n"
          "   or: sudo ./scripts/alfa_spoof.sh\n"
          "\n"
          "Patrol mode (default): random flight around -lat/-lon center\n"
          "Mission mode: autonomous A -> B when -latB/-lonB or lat_b/lon_b in config\n"
          "Waypoint mode: multi-leg route from -waypoints FILE or waypoints= in config\n"
          "\n"
          "Options:\n"
          "  -config FILE   Config file (rids.conf.example)\n"
          "  -i IFACE       Monitor interface\n"
          "  -c CHANNEL     WiFi channel (default 6)\n"
          "  -lat LAT       Start / center latitude (point A)\n"
          "  -lon LON       Start / center longitude (point A)\n"
          "  -latB LAT      Mission destination latitude (point B)\n"
          "  -lonB LON      Mission destination longitude (point B)\n"
          "  -speed MS      Cruise speed m/s (default 12)\n"
          "  -loop 0|1      Ping-pong A<->B or loop route (default 1)\n"
          "  -waypoints F   Waypoint file (.wpt or .gpx)\n"
          "  -alt M         Default / mission-A altitude AGL (m)\n"
          "  -altB M        Mission destination altitude AGL (m)\n"
          "  -pilot_follow 0|1  Operator trails behind drone (auto on for mission/wp)\n"
          "  -pilot_offset M    Meters behind drone (default 30)\n"
          "  -n COUNT       Drones 1-16\n"
          "  -region REG    us|eu|generic\n"
          "  -h             Help\n",
          prog);
}

}  // namespace

int main(int argc, char **argv) {
  const char *iface = nullptr;
  const char *config_path = nullptr;
  int channel = 6;
  float latitude = -6.2088f;
  float longitude = 106.8456f;
  float latitude_b = 0.0f;
  float longitude_b = 0.0f;
  float speed_ms = 12.0f;
  int mission_loop = 1;
  int num_drones = 16;
  int mission_cli = 0;
  int alt_b_cli = 0;
  const char *waypoints_path = nullptr;
  float alt_m = 50.0f;
  float alt_b_m = 0.0f;
  int pilot_follow = -1;
  float pilot_offset_m = 30.0f;
  RidRegion region = RID_REGION_GENERIC;

  SpooferConfig cfg;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-config") == 0 && i + 1 < argc) {
      config_path = argv[++i];
    } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      iface = argv[++i];
    } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
      channel = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-lat") == 0 && i + 1 < argc) {
      latitude = static_cast<float>(atof(argv[++i]));
    } else if (strcmp(argv[i], "-lon") == 0 && i + 1 < argc) {
      longitude = static_cast<float>(atof(argv[++i]));
    } else if (strcmp(argv[i], "-latB") == 0 && i + 1 < argc) {
      latitude_b = static_cast<float>(atof(argv[++i]));
      mission_cli = 1;
    } else if (strcmp(argv[i], "-lonB") == 0 && i + 1 < argc) {
      longitude_b = static_cast<float>(atof(argv[++i]));
      mission_cli = 1;
    } else if (strcmp(argv[i], "-speed") == 0 && i + 1 < argc) {
      speed_ms = static_cast<float>(atof(argv[++i]));
    } else if (strcmp(argv[i], "-loop") == 0 && i + 1 < argc) {
      mission_loop = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
      num_drones = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-region") == 0 && i + 1 < argc) {
      region = parse_region(argv[++i]);
    } else if (strcmp(argv[i], "-waypoints") == 0 && i + 1 < argc) {
      waypoints_path = argv[++i];
    } else if (strcmp(argv[i], "-alt") == 0 && i + 1 < argc) {
      alt_m = static_cast<float>(atof(argv[++i]));
    } else if (strcmp(argv[i], "-altB") == 0 && i + 1 < argc) {
      alt_b_m = static_cast<float>(atof(argv[++i]));
      alt_b_cli = 1;
    } else if (strcmp(argv[i], "-pilot_follow") == 0 && i + 1 < argc) {
      pilot_follow = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-pilot_offset") == 0 && i + 1 < argc) {
      pilot_offset_m = static_cast<float>(atof(argv[++i]));
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
      return 0;
    } else {
      fprintf(stderr, "Unknown argument: %s\n", argv[i]);
      usage(argv[0]);
      return 1;
    }
  }

  if (config_path && load_spoofer_config(config_path, &cfg) == 0) {
    if (!iface && cfg.iface[0]) {
      iface = cfg.iface;
    }
    channel = cfg.channel;
    latitude = cfg.latitude;
    longitude = cfg.longitude;
    if (!mission_cli) {
      latitude_b = cfg.latitude_b;
      longitude_b = cfg.longitude_b;
    }
    speed_ms = cfg.speed_ms;
    mission_loop = cfg.mission_loop;
    num_drones = cfg.num_drones;
    region = parse_region(cfg.region);
    if (!waypoints_path && cfg.waypoints[0]) {
      waypoints_path = cfg.waypoints;
    }
    alt_m = cfg.alt_m;
    if (!alt_b_cli) {
      alt_b_m = cfg.alt_b_m;
    }
    if (pilot_follow < 0 && cfg.pilot_follow_set) {
      pilot_follow = cfg.pilot_follow;
    }
    pilot_offset_m = cfg.pilot_offset_m;
  }

  WaypointRoute route = {};
  route.default_speed_ms = speed_ms;
  route.default_alt_m = alt_m;
  const int waypoint_mode =
      waypoints_path && load_waypoint_route(waypoints_path, &route) == 0;

  if (!iface) {
    usage(argv[0]);
    return 1;
  }

  if (num_drones < 1 || num_drones > 16) {
    fprintf(stderr, "Drone count must be between 1 and 16\n");
    return 1;
  }

  cfg.latitude = latitude;
  cfg.longitude = longitude;
  cfg.latitude_b = latitude_b;
  cfg.longitude_b = longitude_b;
  const int mission_mode = !waypoint_mode && config_has_mission(&cfg);

  if (pilot_follow < 0) {
    pilot_follow = (mission_mode || waypoint_mode) ? 1 : 0;
  }

  if (geteuid() != 0) {
    fprintf(stderr, "Warning: not running as root. Alfa injection requires sudo.\n");
  }

  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);
  srand(static_cast<unsigned>(time(nullptr)));

  fprintf(stdout, "Alfa/Linux Remote ID Spoofer\n");
  fprintf(stdout, "Interface: %s  Channel: %d  Region: %s\n", iface, channel,
          rid_region_name(region));
  if (waypoint_mode) {
    fprintf(stdout, "Flight: WAYPOINT %s (%d legs) @ %.1f m/s %s\n", waypoints_path,
            route.count - 1, speed_ms, mission_loop ? "[loop]" : "[one-way]");
  } else if (mission_mode) {
    fprintf(stdout, "Flight: MISSION A(%.6f, %.6f) -> B(%.6f, %.6f) @ %.1f m/s %s\n",
            latitude, longitude, latitude_b, longitude_b, speed_ms,
            mission_loop ? "[loop]" : "[one-way]");
  } else {
    fprintf(stdout, "Flight: PATROL around (%.6f, %.6f)\n", latitude, longitude);
  }
  fprintf(stdout, "Transmit: beacon + WiFi NAN (1 Hz)\n");
  fprintf(stdout, "Pilot: %s", pilot_follow ? "follow" : "fixed base");
  if (pilot_follow) {
    fprintf(stdout, " (%.0f m behind)", pilot_offset_m);
  }
  fprintf(stdout, "\n\n");

  if (wifi_tx_init(iface, channel) != 0) {
    return 1;
  }

  std::vector<Spoofer> spoofers(static_cast<size_t>(num_drones));
  for (int i = 0; i < num_drones; i++) {
    spoofers[static_cast<size_t>(i)].init(i, region);
    spoofers[static_cast<size_t>(i)].setDefaultAlt(alt_m);
    spoofers[static_cast<size_t>(i)].setPilotFollow(pilot_follow != 0, pilot_offset_m);
    if (waypoint_mode) {
      spoofers[static_cast<size_t>(i)].setWaypointRoute(&route, mission_loop != 0);
    } else if (mission_mode) {
      spoofers[static_cast<size_t>(i)].setMission(latitude, longitude, latitude_b, longitude_b,
                                                  speed_ms, mission_loop != 0, alt_m, alt_b_m);
    } else {
      spoofers[static_cast<size_t>(i)].updateLocation(latitude, longitude);
    }
    if (const DroneProfile *profile = spoofers[static_cast<size_t>(i)].profile()) {
      fprintf(stdout, "  drone %2d: %s %s\n", i + 1, profile->manufacturer, profile->model);
    }
  }

  fprintf(stdout, "\nBroadcasting %d drone(s) — Ctrl+C to stop\n", num_drones);

  while (keep_running) {
    for (int i = 0; i < num_drones; i++) {
      spoofers[static_cast<size_t>(i)].update();
      usleep(static_cast<useconds_t>(1000000 / num_drones));
    }
  }

  fprintf(stdout, "\nStopping...\n");
  wifi_tx_shutdown();
  return 0;
}
