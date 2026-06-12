#!/usr/bin/env bash
# One-shot: detect Alfa, monitor mode, run rids-spoofer.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONF="${ROOT}/rids.conf"
PROFILES_JSON="${ROOT}/../RemoteIDSpoofer/drone_profiles.json"
GEN_PROFILES="${ROOT}/scripts/gen_drone_profiles.py"
SETUP_CHANNEL="6"
SKIP_SETUP=0

usage() {
  echo "Usage: sudo $0 [options]"
  echo "  -c FILE     Config file (default: linux/rids.conf)"
  echo "  --no-setup  Skip monitor-mode setup (interface already ready)"
  echo "  -h          Help"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -c) CONF="$2"; shift 2 ;;
    --no-setup) SKIP_SETUP=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage; exit 1 ;;
  esac
done

if [[ "${EUID}" -ne 0 ]]; then
  echo "Run as root: sudo $0" >&2
  exit 1
fi

if [[ ! -x "${ROOT}/rids-spoofer" ]]; then
  echo "Binary not found. Run: cd ${ROOT} && make" >&2
  exit 1
fi

read_cfg() {
  local key="$1" default="$2"
  if [[ -f "${CONF}" ]] && grep -q "^${key}=" "${CONF}"; then
    grep "^${key}=" "${CONF}" | tail -1 | cut -d= -f2-
  else
    echo "${default}"
  fi
}

IFACE="$(read_cfg iface "")"
if [[ -z "${IFACE}" ]]; then
  IFACE="$("${ROOT}/scripts/detect_alfa.sh")"
fi

SETUP_CHANNEL="$(read_cfg channel 6)"
NUM_DRONES="$(read_cfg drones 5)"
MANUFACTURER="$(read_cfg manufacturer "")"

if [[ ! -f "${PROFILES_JSON}" ]]; then
  echo "Profile JSON not found: ${PROFILES_JSON}" >&2
  exit 1
fi

GEN_ARGS=()
[[ -n "${MANUFACTURER}" ]] && GEN_ARGS+=(--manufacturer "${MANUFACTURER}")

echo "==> Loading drone models from ${PROFILES_JSON}"
if [[ -n "${MANUFACTURER}" ]]; then
  echo "    filter: manufacturer=${MANUFACTURER}"
fi
python3 "${GEN_PROFILES}" "${GEN_ARGS[@]}" "${PROFILES_JSON}" "${ROOT}/../RemoteIDSpoofer/drone_profiles.cpp"
python3 "${GEN_PROFILES}" "${GEN_ARGS[@]}" --list "${NUM_DRONES}" "${PROFILES_JSON}"

if ! make -C "${ROOT}" -q 2>/dev/null; then
  echo "==> Rebuilding rids-spoofer"
  make -C "${ROOT}"
fi

if [[ "${SKIP_SETUP}" -eq 0 ]]; then
  echo "==> Monitor mode on ${IFACE} channel ${SETUP_CHANNEL}"
  "${ROOT}/scripts/setup_monitor.sh" "${IFACE}" "${SETUP_CHANNEL}"
fi

ARGS=(-i "${IFACE}")
[[ -f "${CONF}" ]] && ARGS+=(-config "${CONF}")

echo "==> Starting spoofer (config: ${CONF})"
exec "${ROOT}/rids-spoofer" "${ARGS[@]}"
