#include "spoofer.h"

void Spoofer::applyProfile(const DroneProfile *profile) {
  char serial[ID_SIZE];
  char operator_id[ID_SIZE];

  if (!profile) {
    return;
  }

  active_profile = profile;
  memset(&utm_parameters, 0, sizeof(utm_parameters));

  rid_generate_serial(profile, serial, sizeof(serial));
  rid_generate_operator_id(rid_region, profile, &utm_utils, operator_id, sizeof(operator_id));

  strncpy(utm_parameters.UAV_id, serial, sizeof(utm_parameters.UAV_id) - 1);
  strncpy(utm_parameters.UAS_operator, operator_id, sizeof(utm_parameters.UAS_operator) - 1);
  strncpy(utm_parameters.flight_desc, profile->self_id_desc, sizeof(utm_parameters.flight_desc) - 1);

  utm_parameters.UA_type = profile->ua_type;
  utm_parameters.ID_type = profile->id_type;
  utm_parameters.ID_type2 = ODID_IDTYPE_CAA_REGISTRATION_ID;
  utm_parameters.region = profile->classification_region;
  utm_parameters.EU_category = profile->eu_category;
  utm_parameters.EU_class = profile->eu_class;
  utm_parameters.operator_loc_type = ODID_OPERATOR_LOCATION_TYPE_LIVE_GNSS;

  uint8_t mac[6];
  rid_generate_mac(profile, drone_index, mac);
  squitter.set_wifi_mac(mac);
  squitter.set_self_id((char *) profile->self_id_desc);
  squitter.init(&utm_parameters);
}

void Spoofer::syncClock() {
  time_t now = time(nullptr);
  struct tm *gmt = gmtime(&now);

  if (!gmt) {
    return;
  }

  utm_data.years = 1900 + gmt->tm_year;
  utm_data.months = 1 + gmt->tm_mon;
  utm_data.days = gmt->tm_mday;
  utm_data.hours = gmt->tm_hour;
  utm_data.minutes = gmt->tm_min;
  utm_data.seconds = gmt->tm_sec;
  utm_data.csecs = (millis() % 1000) / 10;
}

void Spoofer::updatePilot() {
  pilot_x += float(rand() % 21 - 10) / 200.0f;
  pilot_y += float(rand() % 21 - 10) / 200.0f;
  pilot_x = constrain(pilot_x, -40.0f, 40.0f);
  pilot_y = constrain(pilot_y, -40.0f, 40.0f);

  utm_data.operator_latitude_d =
      utm_data.base_latitude + (double) pilot_y / m_deg_lat;
  utm_data.operator_longitude_d =
      utm_data.base_longitude + (double) pilot_x / m_deg_long;
  utm_data.operator_alt_m = utm_data.base_alt_m;
}

void Spoofer::init(int index, RidRegion region) {
  drone_index = index;
  rid_region = region;

  memset(&clock_tm, 0, sizeof(struct tm));
  clock_tm.tm_hour = 10;
  clock_tm.tm_mday = 16;
  clock_tm.tm_mon = 11;
  clock_tm.tm_year = 122;
  tv.tv_sec = time_2 = mktime(&clock_tm);
  settimeofday(&tv, &utc);

  pilot_x = float(rand() % 100 - 50) / 10.0f;
  pilot_y = float(rand() % 100 - 50) / 10.0f;

  applyProfile(rid_profile_for_index(drone_index));
  memset(&utm_data, 0, sizeof(utm_data));
  utm_data.flight_status = ODID_STATUS_AIRBORNE;
  syncClock();
}

void Spoofer::updateLocation(float latitude, float longitude) {
  double lat_d = utm_data.latitude_d = utm_data.base_latitude =
      latitude + (float) (rand() % 10 - 5) / 10000.0f;

  utm_data.longitude_d = utm_data.base_longitude =
      longitude + (float) (rand() % 10 - 5) / 10000.0f;

  utm_data.base_valid = 1;
  utm_data.base_alt_m = (float) (rand() % 1000) / 10.0f;

  utm_utils.calc_m_per_deg(lat_d, &m_deg_lat, &m_deg_long);
  updatePilot();
}

void Spoofer::update() {
  if ((millis() - last_update) < 1000) {
    return;
  }

  double time_elapsed_secs = double(millis() - last_update) / 1000.0;
  last_update = millis();
  syncClock();

  utm_data.satellites = rand() % 8 + 8;

  speed_m_x += float(rand() % int(2 * max_accel) - max_accel) / 1000.0f - 0.05f * x;
  speed_m_y += float(rand() % int(2 * max_accel) - max_accel) / 1000.0f - 0.05f * y;
  speed_m_x = constrain(speed_m_x, (float) -max_speed, (float) max_speed);
  speed_m_y = constrain(speed_m_y, (float) -max_speed, (float) max_speed);

  double absolute_speed = sqrt(speed_m_x * speed_m_x + speed_m_y * speed_m_y);
  utm_data.speed_kn = (int) (speed_ms2kn * absolute_speed);

  double heading_rads = atan2(speed_m_y, speed_m_x);
  utm_data.heading = int(heading_rads * angle_rad2deg) % 360;

  x += speed_m_x * (float) time_elapsed_secs;
  y += speed_m_y * (float) time_elapsed_secs;

  float climbrate =
      float(rand() % int(2 * max_climbrate) - max_climbrate) / 1000.0f;
  last_climb_rate = climbrate;
  z = constrain(z + climbrate, 1.0f, (float) max_height);
  utm_data.alt_msl_m = utm_data.base_alt_m + z;
  utm_data.alt_agl_m = z;
  utm_data.alt_baro_m = utm_data.alt_msl_m - 0.8f;
  utm_data.speed_vertical_mps = last_climb_rate;
  utm_data.flight_status = ODID_STATUS_AIRBORNE;

  utm_data.latitude_d = utm_data.base_latitude + (y / (float) m_deg_lat);
  utm_data.longitude_d = utm_data.base_longitude + (x / (float) m_deg_long);

  updatePilot();
  squitter.transmit(&utm_data);
}
