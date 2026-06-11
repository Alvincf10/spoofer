#include "spoofer.h"

#include <cstring>

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
  squitter.set_self_id(const_cast<char *>(profile->self_id_desc));
  squitter.init(&utm_parameters);
}

void Spoofer::syncClock() {
  const time_t now = time(nullptr);
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
  utm_data.csecs = static_cast<int>((millis() % 1000) / 10);
}

void Spoofer::setPilotFollow(bool enable, float offset_m) {
  pilot_follow = enable;
  pilot_offset_m = constrain(offset_m, 5.0f, 200.0f);
}

void Spoofer::setDefaultAlt(float alt_m) {
  default_alt_m = constrain(alt_m, 1.0f, static_cast<float>(max_height));
}

float Spoofer::effectiveWpAlt(int index) const {
  if (index < 0 || index >= wp_count) {
    return default_alt_m;
  }
  if (wp_alt[index] >= 0.0f) {
    return wp_alt[index];
  }
  return default_alt_m;
}

void Spoofer::updatePilotFollow() {
  float back_x = 0.0f;
  float back_y = 0.0f;
  const float speed = static_cast<float>(std::hypot(speed_m_x, speed_m_y));

  if (speed > 0.5f) {
    back_x = -speed_m_x / speed;
    back_y = -speed_m_y / speed;
  } else if (flight_mode == FlightMode::MISSION) {
    const float dir_sign = mission_forward ? 1.0f : -1.0f;
    back_x = -mission_dir_x * dir_sign;
    back_y = -mission_dir_y * dir_sign;
  } else if (flight_mode == FlightMode::WAYPOINT) {
    back_x = -mission_dir_x;
    back_y = -mission_dir_y;
  } else {
    back_x = -1.0f;
    back_y = 0.0f;
  }

  const float jitter = static_cast<float>(rand() % 21 - 10) / 10.0f;
  const float offset = pilot_offset_m + jitter;

  utm_data.operator_latitude_d =
      utm_data.latitude_d + static_cast<double>(back_y * offset) / m_deg_lat;
  utm_data.operator_longitude_d =
      utm_data.longitude_d + static_cast<double>(back_x * offset) / m_deg_long;
  utm_data.operator_alt_m = utm_data.base_alt_m;
}

void Spoofer::updatePilot() {
  if (pilot_follow) {
    updatePilotFollow();
    return;
  }

  pilot_x += static_cast<float>(rand() % 21 - 10) / 200.0f;
  pilot_y += static_cast<float>(rand() % 21 - 10) / 200.0f;
  pilot_x = constrain(pilot_x, -40.0f, 40.0f);
  pilot_y = constrain(pilot_y, -40.0f, 40.0f);

  utm_data.operator_latitude_d =
      utm_data.base_latitude + static_cast<double>(pilot_y) / m_deg_lat;
  utm_data.operator_longitude_d =
      utm_data.base_longitude + static_cast<double>(pilot_x) / m_deg_long;
  utm_data.operator_alt_m = utm_data.base_alt_m;
}

void Spoofer::updateLegAltitude(float time_elapsed_secs) {
  float target_z = leg_alt_start_m;

  if (flight_mode == FlightMode::MISSION) {
    const float t = (mission_length_m > 0.0f) ? (mission_progress_m / mission_length_m) : 0.0f;
    target_z = mission_alt_a_m + (mission_alt_b_m - mission_alt_a_m) * t;
  } else if (flight_mode == FlightMode::WAYPOINT) {
    const float t = (mission_length_m > 0.0f) ? (mission_progress_m / mission_length_m) : 0.0f;
    target_z = leg_alt_start_m + (leg_alt_end_m - leg_alt_start_m) * t;
  }

  const float max_step = 3.0f * time_elapsed_secs;
  const float prev_z = z;
  const float dz = target_z - z;
  z += constrain(dz, -max_step, max_step);
  z = constrain(z, 1.0f, static_cast<float>(max_height));
  last_climb_rate =
      (time_elapsed_secs > 0.0f) ? ((z - prev_z) / time_elapsed_secs) : 0.0f;
}

void Spoofer::recomputeMissionVectors() {
  utm_utils.calc_m_per_deg(lat_a, &m_deg_lat, &m_deg_long);

  const double east_m = (lon_b - lon_a) * m_deg_long;
  const double north_m = (lat_b - lat_a) * m_deg_lat;
  mission_length_m = static_cast<float>(std::hypot(east_m, north_m));

  if (mission_length_m < 1.0f) {
    mission_length_m = 1.0f;
  }

  mission_dir_x = static_cast<float>(east_m / mission_length_m);
  mission_dir_y = static_cast<float>(north_m / mission_length_m);
  mission_perp_x = -mission_dir_y;
  mission_perp_y = mission_dir_x;

  lane_offset_m = static_cast<float>((drone_index % 4) - 1.5f) * 25.0f;
  mission_progress_m = -static_cast<float>(drone_index) * 40.0f;
  mission_forward = true;
}

void Spoofer::beginLeg(int leg_index) {
  if (leg_index < 0 || leg_index >= wp_count - 1) {
    return;
  }

  wp_leg = leg_index;
  lat_a = wp_lat[wp_leg];
  lon_a = wp_lon[wp_leg];
  lat_b = wp_lat[wp_leg + 1];
  lon_b = wp_lon[wp_leg + 1];

  cruise_speed_mps = wp_speed[wp_leg] > 0.0f ? wp_speed[wp_leg] : default_speed_mps;
  cruise_speed_mps = constrain(cruise_speed_mps, 1.0f, static_cast<float>(max_speed));

  leg_alt_start_m = effectiveWpAlt(wp_leg);
  leg_alt_end_m = effectiveWpAlt(wp_leg + 1);

  recomputeMissionVectors();
}

void Spoofer::setWaypointRoute(const WaypointRoute *route, bool loop) {
  if (!route || route->count < 2 || route->count > kMaxWaypoints) {
    return;
  }

  flight_mode = FlightMode::WAYPOINT;
  wp_count = route->count;
  default_speed_mps = route->default_speed_ms;
  default_alt_m = route->default_alt_m;

  for (int i = 0; i < wp_count; i++) {
    wp_lat[i] = route->lat[i];
    wp_lon[i] = route->lon[i];
    wp_speed[i] = route->speed_mps[i];
    wp_alt[i] = route->alt_m[i];
  }

  mission_loop = loop;
  beginLeg(0);

  utm_data.base_valid = 1;
  utm_data.base_latitude = lat_a;
  utm_data.base_longitude = lon_a;
  utm_data.base_alt_m = static_cast<float>(rand() % 1000) / 10.0f;
  z = leg_alt_start_m;

  updatePilot();
}

void Spoofer::setMission(float lat_start, float lon_start, float lat_end, float lon_end,
                         float speed_mps, bool loop, float alt_a_m, float alt_b_m) {
  flight_mode = FlightMode::MISSION;
  wp_count = 0;
  lat_a = lat_start;
  lon_a = lon_start;
  lat_b = lat_end;
  lon_b = lon_end;
  default_speed_mps = speed_mps;
  cruise_speed_mps = constrain(speed_mps, 1.0f, static_cast<float>(max_speed));
  mission_loop = loop;

  mission_alt_a_m = alt_a_m > 0.0f ? alt_a_m : default_alt_m;
  mission_alt_b_m = alt_b_m > 0.0f ? alt_b_m : mission_alt_a_m;
  leg_alt_start_m = mission_alt_a_m;
  leg_alt_end_m = mission_alt_b_m;

  recomputeMissionVectors();

  utm_data.base_valid = 1;
  utm_data.base_latitude = lat_a;
  utm_data.base_longitude = lon_a;
  utm_data.base_alt_m = static_cast<float>(rand() % 1000) / 10.0f;
  z = mission_alt_a_m;

  updatePilot();
}

void Spoofer::init(int index, RidRegion region) {
  drone_index = index;
  rid_region = region;
  flight_mode = FlightMode::PATROL;
  default_alt_m = 50.0f;
  default_speed_mps = 12.0f;



  pilot_x = static_cast<float>(rand() % 100 - 50) / 10.0f;
  pilot_y = static_cast<float>(rand() % 100 - 50) / 10.0f;

  applyProfile(rid_profile_for_index(drone_index));
  memset(&utm_data, 0, sizeof(utm_data));
  utm_data.flight_status = ODID_STATUS_AIRBORNE;
  syncClock();
}

void Spoofer::updateLocation(float latitude, float longitude) {
  flight_mode = FlightMode::PATROL;

  const double lat_d = utm_data.latitude_d = utm_data.base_latitude =
      latitude + static_cast<float>(rand() % 10 - 5) / 10000.0f;

  utm_data.longitude_d = utm_data.base_longitude =
      longitude + static_cast<float>(rand() % 10 - 5) / 10000.0f;

  utm_data.base_valid = 1;
  utm_data.base_alt_m = static_cast<float>(rand() % 1000) / 10.0f;

  utm_utils.calc_m_per_deg(lat_d, &m_deg_lat, &m_deg_long);
  z = default_alt_m;
  updatePilot();
}

void Spoofer::updatePatrol(float time_elapsed_secs) {
  speed_m_x += static_cast<float>(rand() % static_cast<int>(2 * max_accel) - max_accel) /
                   1000.0f -
               0.05f * x;
  speed_m_y += static_cast<float>(rand() % static_cast<int>(2 * max_accel) - max_accel) /
                   1000.0f -
               0.05f * y;
  speed_m_x = constrain(speed_m_x, static_cast<float>(-max_speed),
                        static_cast<float>(max_speed));
  speed_m_y = constrain(speed_m_y, static_cast<float>(-max_speed),
                        static_cast<float>(max_speed));

  x += speed_m_x * time_elapsed_secs;
  y += speed_m_y * time_elapsed_secs;

  const float climbrate =
      static_cast<float>(rand() % static_cast<int>(2 * max_climbrate) - max_climbrate) / 1000.0f;
  last_climb_rate = climbrate;
  z = constrain(z + climbrate, 1.0f, static_cast<float>(max_height));

  utm_data.latitude_d = utm_data.base_latitude + (y / static_cast<float>(m_deg_lat));
  utm_data.longitude_d = utm_data.base_longitude + (x / static_cast<float>(m_deg_long));
}

void Spoofer::updateMission(float time_elapsed_secs) {
  const float dir_sign = mission_forward ? 1.0f : -1.0f;
  mission_progress_m += dir_sign * cruise_speed_mps * time_elapsed_secs;

  if (mission_forward && mission_progress_m >= mission_length_m) {
    if (mission_loop) {
      mission_forward = false;
      mission_progress_m = mission_length_m;
    } else {
      mission_progress_m = mission_length_m;
    }
  } else if (!mission_forward && mission_progress_m <= 0.0f) {
    if (mission_loop) {
      mission_forward = true;
      mission_progress_m = 0.0f;
    } else {
      mission_progress_m = 0.0f;
    }
  }

  mission_progress_m = constrain(mission_progress_m, 0.0f, mission_length_m);

  const float along = mission_progress_m;
  const float px = mission_dir_x * along + mission_perp_x * lane_offset_m;
  const float py = mission_dir_y * along + mission_perp_y * lane_offset_m;

  utm_data.latitude_d = lat_a + py / m_deg_lat;
  utm_data.longitude_d = lon_a + px / m_deg_long;

  speed_m_x = mission_dir_x * cruise_speed_mps * dir_sign;
  speed_m_y = mission_dir_y * cruise_speed_mps * dir_sign;

  updateLegAltitude(time_elapsed_secs);
}

void Spoofer::updateWaypoint(float time_elapsed_secs) {
  mission_progress_m += cruise_speed_mps * time_elapsed_secs;

  if (mission_progress_m >= mission_length_m) {
    const int next_leg = wp_leg + 1;
    if (next_leg >= wp_count - 1) {
      if (mission_loop) {
        beginLeg(0);
      } else {
        mission_progress_m = mission_length_m;
      }
    } else {
      beginLeg(next_leg);
    }
  }

  mission_progress_m = constrain(mission_progress_m, 0.0f, mission_length_m);

  const float along = mission_progress_m;
  const float px = mission_dir_x * along + mission_perp_x * lane_offset_m;
  const float py = mission_dir_y * along + mission_perp_y * lane_offset_m;

  utm_data.latitude_d = lat_a + py / m_deg_lat;
  utm_data.longitude_d = lon_a + px / m_deg_long;

  speed_m_x = mission_dir_x * cruise_speed_mps;
  speed_m_y = mission_dir_y * cruise_speed_mps;

  updateLegAltitude(time_elapsed_secs);
}

void Spoofer::update() {
  if ((millis() - last_update) < 1000) {
    return;
  }

  const float time_elapsed_secs =
      static_cast<float>(static_cast<double>(millis() - last_update) / 1000.0);
  last_update = millis();
  syncClock();

  utm_data.satellites = rand() % 8 + 8;

  if (flight_mode == FlightMode::WAYPOINT) {
    updateWaypoint(time_elapsed_secs);
  } else if (flight_mode == FlightMode::MISSION) {
    updateMission(time_elapsed_secs);
  } else {
    updatePatrol(time_elapsed_secs);
  }

  const double absolute_speed = std::hypot(speed_m_x, speed_m_y);
  utm_data.speed_kn = static_cast<int>(speed_ms2kn * absolute_speed);

  const double heading_rads = atan2(speed_m_y, speed_m_x);
  utm_data.heading = static_cast<int>(heading_rads * angle_rad2deg);
  if (utm_data.heading < 0) {
    utm_data.heading += 360;
  }

  utm_data.alt_msl_m = utm_data.base_alt_m + z;
  utm_data.alt_agl_m = z;
  utm_data.alt_baro_m = utm_data.alt_msl_m - 0.8f;
  utm_data.speed_vertical_mps = last_climb_rate;
  utm_data.flight_status = ODID_STATUS_AIRBORNE;

  updatePilot();
  squitter.transmit(&utm_data);
}
