#include "config.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

static void trim(char *s) {
  if (!s) {
    return;
  }
  size_t n = strlen(s);
  while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ')) {
    s[--n] = 0;
  }
}

static int apply_kv(SpooferConfig *cfg, const char *key, char *value) {
  trim(value);

  if (strcmp(key, "iface") == 0) {
    strncpy(cfg->iface, value, sizeof(cfg->iface) - 1);
  } else if (strcmp(key, "channel") == 0) {
    cfg->channel = atoi(value);
  } else if (strcmp(key, "lat") == 0) {
    cfg->latitude = static_cast<float>(atof(value));
  } else if (strcmp(key, "lon") == 0) {
    cfg->longitude = static_cast<float>(atof(value));
  } else if (strcmp(key, "lat_b") == 0 || strcmp(key, "latB") == 0) {
    cfg->latitude_b = static_cast<float>(atof(value));
  } else if (strcmp(key, "lon_b") == 0 || strcmp(key, "lonB") == 0) {
    cfg->longitude_b = static_cast<float>(atof(value));
  } else if (strcmp(key, "speed_ms") == 0 || strcmp(key, "speed") == 0) {
    cfg->speed_ms = static_cast<float>(atof(value));
  } else if (strcmp(key, "loop") == 0) {
    cfg->mission_loop = atoi(value);
  } else if (strcmp(key, "drones") == 0 || strcmp(key, "n") == 0) {
    cfg->num_drones = atoi(value);
  } else if (strcmp(key, "region") == 0) {
    strncpy(cfg->region, value, sizeof(cfg->region) - 1);
  } else if (strcmp(key, "waypoints") == 0) {
    strncpy(cfg->waypoints, value, sizeof(cfg->waypoints) - 1);
  } else if (strcmp(key, "alt_m") == 0 || strcmp(key, "alt") == 0) {
    cfg->alt_m = static_cast<float>(atof(value));
  } else if (strcmp(key, "alt_b_m") == 0 || strcmp(key, "altB") == 0) {
    cfg->alt_b_m = static_cast<float>(atof(value));
  } else if (strcmp(key, "pilot_follow") == 0) {
    cfg->pilot_follow = atoi(value);
    cfg->pilot_follow_set = 1;
  } else if (strcmp(key, "pilot_offset_m") == 0 || strcmp(key, "pilot_offset") == 0) {
    cfg->pilot_offset_m = static_cast<float>(atof(value));
  } else if (key[0] != '#') {
    fprintf(stderr, "config: unknown key '%s'\n", key);
  }

  return 0;
}

int load_spoofer_config(const char *path, SpooferConfig *cfg) {
  FILE *fp = fopen(path, "r");
  if (!fp) {
    fprintf(stderr, "config: cannot open %s\n", path);
    return -1;
  }

  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    if (line[0] == '#' || line[0] == '\n') {
      continue;
    }

    char *eq = strchr(line, '=');
    if (!eq) {
      continue;
    }

    *eq = 0;
    char *key = line;
    char *value = eq + 1;
    trim(key);
    apply_kv(cfg, key, value);
  }

  fclose(fp);
  cfg->loaded = 1;
  return 0;
}

int config_has_mission(const SpooferConfig *cfg) {
  if (!cfg) {
    return 0;
  }
  if (config_has_waypoints(cfg)) {
    return 0;
  }
  const float dlat = cfg->latitude_b - cfg->latitude;
  const float dlon = cfg->longitude_b - cfg->longitude;
  return (cfg->latitude_b != 0.0f || cfg->longitude_b != 0.0f) &&
         (dlat * dlat + dlon * dlon > 1e-10f);
}

int config_has_waypoints(const SpooferConfig *cfg) {
  return cfg && cfg->waypoints[0] != '\0';
}
