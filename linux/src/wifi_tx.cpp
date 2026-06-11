#include "wifi_tx.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <pcap/pcap.h>

namespace {

pcap_t *pcap_handle = nullptr;
int current_channel = 6;

uint16_t channel_to_mhz(int channel) {
  if (channel >= 1 && channel <= 13) {
    return static_cast<uint16_t>(2407 + 5 * channel);
  }
  if (channel >= 36 && channel <= 165) {
    return static_cast<uint16_t>(5000 + 5 * channel);
  }
  return static_cast<uint16_t>(2407 + 5 * 6);
}

int build_radiotap(uint8_t *out, int channel) {
  const uint16_t freq = channel_to_mhz(channel);
  const uint8_t header[] = {
      0x00, 0x00,                   // version, pad
      0x0e, 0x00,                   // radiotap length = 14
      0x0c, 0x00, 0x00, 0x00,       // present: RATE | CHANNEL
      0x02,                         // RATE (1 Mbps)
      0x00,                         // 1 byte padding for alignment
      static_cast<uint8_t>(freq & 0xff),
      static_cast<uint8_t>((freq >> 8) & 0xff),
      0xa0, 0x00,                   // channel flags: 2 GHz + CCK
  };
  memcpy(out, header, sizeof(header));
  return static_cast<int>(sizeof(header));
}

}  // namespace

int wifi_tx_init(const char *iface, int channel) {
  char errbuf[PCAP_ERRBUF_SIZE] = {0};

  if (pcap_handle) {
    wifi_tx_shutdown();
  }

  pcap_handle = pcap_create(iface, errbuf);
  if (!pcap_handle) {
    fprintf(stderr, "pcap_create(%s): %s\n", iface, errbuf);
    return -1;
  }

  if (pcap_set_snaplen(pcap_handle, 4096) != 0 ||
      pcap_set_promisc(pcap_handle, 1) != 0 ||
      pcap_set_timeout(pcap_handle, 1) != 0 ||
      pcap_set_immediate_mode(pcap_handle, 1) != 0 ||
      pcap_activate(pcap_handle) != 0) {
    fprintf(stderr, "pcap_activate(%s): %s\n", iface, pcap_geterr(pcap_handle));
    pcap_close(pcap_handle);
    pcap_handle = nullptr;
    return -1;
  }

  current_channel = channel;
  fprintf(stdout, "WiFi TX ready on %s (channel %d, %u MHz)\n", iface, channel,
          channel_to_mhz(channel));
  return 0;
}

void wifi_tx_shutdown(void) {
  if (pcap_handle) {
    pcap_close(pcap_handle);
    pcap_handle = nullptr;
  }
}

int wifi_tx_set_channel(int channel) {
  current_channel = channel;
  return 0;
}

int wifi_tx_inject(const uint8_t *frame80211, int frame_len) {
  if (!pcap_handle || !frame80211 || frame_len <= 0) {
    return -1;
  }

  uint8_t packet[512 + 32];
  const int rt_len = build_radiotap(packet, current_channel);
  if (rt_len + frame_len > static_cast<int>(sizeof(packet))) {
    return -1;
  }

  memcpy(packet + rt_len, frame80211, frame_len);
  const int total_len = rt_len + frame_len;

  if (pcap_inject(pcap_handle, packet, total_len) != total_len) {
    fprintf(stderr, "pcap_inject failed: %s\n", pcap_geterr(pcap_handle));
    return -1;
  }

  return 0;
}
