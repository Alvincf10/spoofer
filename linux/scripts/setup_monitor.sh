#!/usr/bin/env bash
set -euo pipefail

IFACE="${1:-}"
CHANNEL="${2:-6}"

if [[ -z "${IFACE}" ]]; then
  echo "Usage: sudo $0 <wireless_iface> [channel]"
  echo "Example: sudo $0 wlan0 6"
  exit 1
fi

if [[ "${EUID}" -ne 0 ]]; then
  echo "Run as root (sudo)."
  exit 1
fi

MON_IFACE="${IFACE}mon"

echo "[1/4] Stopping NetworkManager on ${IFACE} (if managed)..."
nmcli dev set "${IFACE}" managed no 2>/dev/null || true

echo "[2/4] Creating monitor interface ${MON_IFACE} on channel ${CHANNEL}..."
ip link set "${IFACE}" down
iw dev "${IFACE}" set type monitor
ip link set "${IFACE}" up
iw dev "${IFACE}" set channel "${CHANNEL}" HT20

echo "[3/4] Verifying injection support..."
if ! iw phy "$(iw dev "${IFACE}" info | awk '/wiphy/ {print $2}')" info | grep -q "Supported interface modes:.*\* monitor"; then
  echo "Warning: monitor mode may not be supported by this driver."
fi

echo "[4/4] Ready. Use monitor interface: ${IFACE}"
echo
echo "Run spoofer:"
echo "  sudo ./rids-spoofer -i ${IFACE} -c ${CHANNEL} -lat <lat> -lon <lon> -n 16"
echo
echo "Restore managed mode later:"
echo "  sudo ip link set ${IFACE} down"
echo "  sudo iw dev ${IFACE} set type managed"
echo "  sudo ip link set ${IFACE} up"
echo "  sudo nmcli dev set ${IFACE} managed yes"
