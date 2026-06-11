#pragma once

#include <cstdint>

int wifi_tx_init(const char *iface, int channel);
void wifi_tx_shutdown(void);
int wifi_tx_set_channel(int channel);
int wifi_tx_inject(const uint8_t *frame80211, int frame_len);
