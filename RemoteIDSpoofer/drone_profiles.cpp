#include "drone_profiles.h"

#if defined(LINUX_RIDS)
#include <cstdlib>
#include <cstring>
#else
#include <Arduino.h>
#include <string.h>
#endif

static const char SERIAL_CHARSET[] = "0123456789ABCDEFGHJKLMNPQRSTUVWXYZ";

const DroneProfile DRONE_PROFILES[] = {
    {"DJI", "Mini 3 Pro", 2, 1, "1581F", 20, 1, 1, 1, "Recreational", "DEU", {0x34, 0xD2, 0x62}},
    {"DJI", "Mavic 3", 2, 1, "1ZNBJ", 20, 1, 2, 1, "Recreational", "DEU", {0x34, 0xD2, 0x62}},
    {"DJI", "Air 3", 2, 1, "4KUXT", 20, 1, 2, 1, "Recreational", "FRA", {0x34, 0xD2, 0x62}},
    {"DJI", "Avata 2", 2, 1, "1581F", 20, 1, 1, 1, "Recreational", "GBR", {0x34, 0xD2, 0x62}},
    {"Autel", "EVO Lite+", 2, 1, "AU7YC", 20, 1, 2, 1, "Commercial", "DEU", {0x8C, 0x85, 0x80}},
    {"Autel", "EVO II Pro", 2, 1, "AU8TN", 20, 1, 3, 1, "Commercial", "NLD", {0x8C, 0x85, 0x80}},
    {"Skydio", "2+", 2, 1, "SDX00", 20, 1, 2, 1, "Commercial", "USA", {0xFC, 0xF1, 0x36}},
    {"Parrot", "Anafi USA", 2, 1, "PI040", 20, 1, 2, 1, "Recreational", "FRA", {0x90, 0x3A, 0xE6}},
};

const size_t DRONE_PROFILE_COUNT =
    sizeof(DRONE_PROFILES) / sizeof(DRONE_PROFILES[0]);

const DroneProfile *rid_profile_for_index(int drone_index) {
  if (DRONE_PROFILE_COUNT == 0) {
    return nullptr;
  }
  return &DRONE_PROFILES[((drone_index % (int)DRONE_PROFILE_COUNT) + (int)DRONE_PROFILE_COUNT) %
                         (int)DRONE_PROFILE_COUNT];
}

static void random_serial_suffix(char *out, int count) {
  for (int i = 0; i < count; ++i) {
    out[i] = SERIAL_CHARSET[rand() % (sizeof(SERIAL_CHARSET) - 1)];
  }
  out[count] = 0;
}

void rid_generate_serial(const DroneProfile *profile, char *out, size_t out_len) {
  if (!profile || !out || out_len < 2) {
    return;
  }

  memset(out, 0, out_len);
  strncpy(out, profile->serial_prefix, out_len - 1);

  int prefix_len = strlen(out);
  int total = profile->serial_total_len;
  if (total > (int)out_len - 1) {
    total = (int)out_len - 1;
  }
  if (total > prefix_len) {
    random_serial_suffix(out + prefix_len, total - prefix_len);
  }
}

static void generate_faa_operator_id(char *out, size_t out_len) {
  static const char FAA_CHARSET[] = "0123456789ABCDEFGHJKLMNPQRSTUVWXYZ";

  if (out_len < 13) {
    return;
  }

  out[0] = 'F';
  out[1] = 'A';
  for (int i = 2; i < 12; ++i) {
    out[i] = FAA_CHARSET[rand() % (sizeof(FAA_CHARSET) - 1)];
  }
  out[12] = 0;
}

static void generate_eu_operator_id(const DroneProfile *profile, UTM_Utilities *utils,
                                    char *out, size_t out_len) {
  static const char EU_BODY[] = "0123456789abcdefghijklmnopqrstuvwxyz";
  char check_input[20];
  const char *prefix = (profile && profile->eu_op_prefix) ? profile->eu_op_prefix : "DEU";

  if (!utils || out_len < 17) {
    return;
  }

  out[0] = prefix[0];
  out[1] = prefix[1];
  out[2] = prefix[2];

  int j = 0;
  for (int i = 3; i < 15; ++i) {
    out[i] = EU_BODY[rand() % (sizeof(EU_BODY) - 1)];
    check_input[j++] = out[i];
  }

  check_input[j++] = 'X';
  check_input[j++] = '9';
  check_input[j++] = 'k';
  check_input[j] = 0;

  out[15] = utils->luhn36_check(check_input);
  out[16] = 0;
}

static void generate_generic_operator_id(char *out, size_t out_len) {
  static const char CHARSET[] =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  if (out_len < 17) {
    return;
  }

  for (int i = 0; i < 16; ++i) {
    out[i] = CHARSET[rand() % (sizeof(CHARSET) - 1)];
  }
  out[16] = 0;
}

void rid_generate_operator_id(RidRegion region, const DroneProfile *profile,
                              UTM_Utilities *utils, char *out, size_t out_len) {
  switch (region) {
    case RID_REGION_US_FAA:
      generate_faa_operator_id(out, out_len);
      break;
    case RID_REGION_EU:
      generate_eu_operator_id(profile, utils, out, out_len);
      break;
    default:
      generate_generic_operator_id(out, out_len);
      break;
  }
}

void rid_generate_mac(const DroneProfile *profile, int drone_index, uint8_t mac[6]) {
  if (!profile || !mac) {
    return;
  }

  mac[0] = profile->mac_oui[0] & 0xFC;
  mac[1] = profile->mac_oui[1];
  mac[2] = profile->mac_oui[2];
  mac[3] = static_cast<uint8_t>((drone_index * 37 + rand()) & 0xFF);
  mac[4] = static_cast<uint8_t>(rand() & 0xFF);
  mac[5] = static_cast<uint8_t>(rand() & 0xFF);
}

const char *rid_region_name(RidRegion region) {
  switch (region) {
    case RID_REGION_US_FAA:
      return "US/FAA";
    case RID_REGION_EU:
      return "EU";
    default:
      return "generic";
  }
}
