#include "spoofer.h"

#include <Arduino.h>

void Spoofer::init() {
  memset(&clock_tm, 0, sizeof(struct tm));
  clock_tm.tm_hour = 10;
  clock_tm.tm_mday = 16;
  clock_tm.tm_mon = 11;
  clock_tm.tm_year = 122;
  tv.tv_sec = time_2 = mktime(&clock_tm);
  settimeofday(&tv, &utc);

  memset(&utm_parameters, 0, sizeof(utm_parameters));
  strcpy(utm_parameters.UAS_operator, getID().c_str());
  utm_parameters.region = 1;
  utm_parameters.EU_category = 1;
  utm_parameters.EU_class = 5;
  squitter.init(&utm_parameters);
  memset(&utm_data, 0, sizeof(utm_data));
}

void Spoofer::updateLocation(float latitude, float longitude) {
  double lat_d = utm_data.latitude_d = utm_data.base_latitude =
      latitude + static_cast<float>(rand() % 10 - 5) / 10000.0f;

  utm_data.longitude_d = utm_data.base_longitude =
      longitude + static_cast<float>(rand() % 10 - 5) / 10000.0f;

  utm_data.base_valid = 1;
  utm_data.base_alt_m = static_cast<float>(rand() % 1000) / 10.0f;

  utm_utils.calc_m_per_deg(lat_d, &m_deg_lat, &m_deg_long);
}

void Spoofer::update() {
  if ((millis() - last_update) < 200) {
    return;
  }

  const double time_elapsed_secs = static_cast<double>(millis() - last_update) / 1000.0;
  last_update = millis();

  utm_data.satellites = rand() % 8 + 8;

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

  const double absolute_speed = std::hypot(speed_m_x, speed_m_y);
  utm_data.speed_kn = static_cast<int>(speed_ms2kn * absolute_speed);

  const double heading_rads = atan2(speed_m_y, speed_m_x);
  utm_data.heading = static_cast<int>(heading_rads * angle_rad2deg) % 360;

  x += speed_m_x * static_cast<float>(time_elapsed_secs);
  y += speed_m_y * static_cast<float>(time_elapsed_secs);

  const float climbrate =
      static_cast<float>(rand() % static_cast<int>(2 * max_climbrate) - max_climbrate) / 1000.0f;
  z = constrain(z + climbrate, 1.0f, static_cast<float>(max_height));
  utm_data.alt_msl_m = utm_data.base_alt_m + z;
  utm_data.alt_agl_m = z;

  utm_data.latitude_d = utm_data.base_latitude + (y / static_cast<float>(m_deg_lat));
  utm_data.longitude_d = utm_data.base_longitude + (x / static_cast<float>(m_deg_long));

  squitter.transmit(&utm_data);
}

std::string Spoofer::getID() {
  static const char characters[] =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::string id;
  id.reserve(16);
  for (int i = 0; i < 16; i++) {
    id.push_back(characters[rand() % (sizeof(characters) - 1)]);
  }
  return id;
}
