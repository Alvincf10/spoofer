#include <Arduino.h>

#include <cstdlib>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <vector>

#include "spoofer.h"
#include "wifi_tx.h"

namespace {

volatile sig_atomic_t keep_running = 1;

void handle_signal(int) { keep_running = 0; }

void usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s -i <monitor_iface> [options]\n"
          "\n"
          "Required:\n"
          "  -i IFACE       Monitor-mode interface (e.g. wlan0mon)\n"
          "\n"
          "Options:\n"
          "  -c CHANNEL     WiFi channel (default: 6)\n"
          "  -lat LAT       Center latitude (default: -6.2088)\n"
          "  -lon LON       Center longitude (default: 106.8456)\n"
          "  -n COUNT       Number of drones, 1-16 (default: 16)\n"
          "  -h             Show this help\n"
          "\n"
          "Example (Alfa RTL8814AU on Linux):\n"
          "  sudo ./scripts/setup_monitor.sh wlan0 6\n"
          "  sudo ./rids-spoofer -i wlan0mon -c 6 -lat -6.2 -lon 106.8 -n 16\n",
          prog);
}

}  // namespace

int main(int argc, char **argv) {
  const char *iface = nullptr;
  int channel = 6;
  float latitude = -6.2088f;
  float longitude = 106.8456f;
  int num_drones = 16;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      iface = argv[++i];
    } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
      channel = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-lat") == 0 && i + 1 < argc) {
      latitude = static_cast<float>(atof(argv[++i]));
    } else if (strcmp(argv[i], "-lon") == 0 && i + 1 < argc) {
      longitude = static_cast<float>(atof(argv[++i]));
    } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
      num_drones = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
      return 0;
    } else {
      fprintf(stderr, "Unknown argument: %s\n", argv[i]);
      usage(argv[0]);
      return 1;
    }
  }

  if (!iface) {
    usage(argv[0]);
    return 1;
  }

  if (num_drones < 1 || num_drones > 16) {
    fprintf(stderr, "Drone count must be between 1 and 16\n");
    return 1;
  }

  if (geteuid() != 0) {
    fprintf(stderr, "Warning: not running as root. Packet injection usually requires sudo.\n");
  }

  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);
  srand(static_cast<unsigned>(time(nullptr)));

  if (wifi_tx_init(iface, channel) != 0) {
    return 1;
  }

  std::vector<Spoofer> spoofers(static_cast<size_t>(num_drones));
  for (int i = 0; i < num_drones; i++) {
    spoofers[static_cast<size_t>(i)].init();
    spoofers[static_cast<size_t>(i)].updateLocation(latitude, longitude);
  }

  fprintf(stdout,
          "Broadcasting %d Remote ID drone(s) on %s channel %d around %.6f, %.6f\n",
          num_drones, iface, channel, latitude, longitude);
  fprintf(stdout, "Press Ctrl+C to stop.\n");

  while (keep_running) {
    for (int i = 0; i < num_drones; i++) {
      spoofers[static_cast<size_t>(i)].update();
      usleep(static_cast<useconds_t>(200000 / num_drones));
    }
  }

  fprintf(stdout, "\nStopping...\n");
  wifi_tx_shutdown();
  return 0;
}
