#!/usr/bin/env bash
# Print the wireless interface name for a connected Alfa / RTL8814AU adapter.
set -euo pipefail

if ! lsusb 2>/dev/null | grep -qiE '0bda:8813|alfa'; then
  echo "No RTL8814AU (0bda:8813) USB device found." >&2
  exit 1
fi

IFACE="$(iw dev 2>/dev/null | awk '/Interface/ {print $2; exit}')"

if [[ -z "${IFACE}" ]]; then
  for cand in /sys/class/net/wl* /sys/class/net/wlan*; do
    [[ -e "${cand}" ]] || continue
    IFACE="$(basename "${cand}")"
    break
  done
fi

if [[ -z "${IFACE}" ]]; then
  echo "RTL8814AU detected on USB but no wireless interface found. Load driver first." >&2
  exit 1
fi

echo "${IFACE}"
