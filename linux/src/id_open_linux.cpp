/*
 * Linux / USB WiFi adapter backend (RTL8814AU and other inject-capable NICs).
 */

#include <Arduino.h>

#include "id_open.h"
#include "wifi_tx.h"

static Stream *Debug_Serial = nullptr;

void construct2() { return; }

void init2(char *ssid, int ssid_length, uint8_t *WiFi_mac_addr, uint8_t wifi_channel) {
  char text[128];
  text[0] = text[63] = 0;

#if DIAGNOSTICS
  Debug_Serial = &Serial;
#endif

  (void)ssid;
  (void)ssid_length;
  wifi_tx_set_channel(wifi_channel);

  if (Debug_Serial) {
    sprintf(text, "linux mac: %02x:%02x:%02x:%02x:%02x:%02x channel: %u\r\n",
            WiFi_mac_addr[0], WiFi_mac_addr[1], WiFi_mac_addr[2], WiFi_mac_addr[3],
            WiFi_mac_addr[4], WiFi_mac_addr[5], wifi_channel);
    Debug_Serial->print(text);
  }
}

uint8_t *capability() {
  static uint8_t capa[2] = {0x21, 0x04};
  return capa;
}

int tag_rates(uint8_t *beacon_frame, int beacon_offset) {
  beacon_frame[beacon_offset++] = 0x01;
  beacon_frame[beacon_offset++] = 0x08;
  beacon_frame[beacon_offset++] = 0x8b;
  beacon_frame[beacon_offset++] = 0x96;
  beacon_frame[beacon_offset++] = 0x82;
  beacon_frame[beacon_offset++] = 0x84;
  beacon_frame[beacon_offset++] = 0x0c;
  beacon_frame[beacon_offset++] = 0x18;
  beacon_frame[beacon_offset++] = 0x30;
  beacon_frame[beacon_offset++] = 0x60;
  return beacon_offset;
}

int tag_ext_rates(uint8_t *beacon_frame, int beacon_offset) {
  beacon_frame[beacon_offset++] = 0x32;
  beacon_frame[beacon_offset++] = 0x04;
  beacon_frame[beacon_offset++] = 0x6c;
  beacon_frame[beacon_offset++] = 0x12;
  beacon_frame[beacon_offset++] = 0x24;
  beacon_frame[beacon_offset++] = 0x48;
  return beacon_offset;
}

int misc_tags(uint8_t *beacon_frame, int beacon_offset) {
  (void)beacon_frame;
  return beacon_offset;
}

int transmit_wifi2(uint8_t *buffer, int length) {
#if ID_OD_WIFI
  if (length) {
    return wifi_tx_inject(buffer, length);
  }
#endif
  return 0;
}

int transmit_ble2(uint8_t *ble_message, int length) {
  (void)ble_message;
  (void)length;
  return 0;
}
