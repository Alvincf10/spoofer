#ifndef DRONE_PROFILES_H
#define DRONE_PROFILES_H

#include <stddef.h>
#include <stdint.h>

#include "utm.h"

enum RidRegion {
  RID_REGION_US_FAA = 0,
  RID_REGION_EU = 1,
  RID_REGION_GENERIC = 2,
};

struct DroneProfile {
  const char *manufacturer;
  const char *model;
  uint8_t ua_type;
  uint8_t id_type;
  const char *serial_prefix;
  uint8_t serial_total_len;
  uint8_t eu_category;
  uint8_t eu_class;
  uint8_t classification_region;
  const char *self_id_desc;
  const char *eu_op_prefix;
  uint8_t mac_oui[3];
};

void rid_generate_mac(const DroneProfile *profile, int drone_index, uint8_t mac[6]);

extern const DroneProfile DRONE_PROFILES[];
extern const size_t DRONE_PROFILE_COUNT;

const DroneProfile *rid_profile_for_index(int drone_index);
void rid_generate_serial(const DroneProfile *profile, char *out, size_t out_len);
void rid_generate_operator_id(RidRegion region, const DroneProfile *profile,
                              UTM_Utilities *utils, char *out, size_t out_len);
const char *rid_region_name(RidRegion region);

#endif
