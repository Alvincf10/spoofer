# RIDS — Remote ID Spoofer (Alfa RTL8814AU)

Broadcast **ASTM OpenDroneID** over **WiFi beacon + WiFi NAN** from **Linux** using an **Alfa USB adapter** (RTL8814AU, e.g. AWUS1900) in monitor mode with packet injection.

Simulates up to **16 drones** with commercial-style profiles (DJI / Autel / Skydio / Parrot): vendor serials, MAC OUIs, operator IDs, LIVE_GNSS pilot tracking, and realistic airborne telemetry.

| Platform | Status |
|----------|--------|
| Linux + RTL8814AU (injection) | **Supported** |
| macOS / Windows | Not supported |
| ESP8266/ESP32 (`RemoteIDSpoofer/`) | Legacy Arduino path |

**Full guide (driver, config, troubleshooting):** [linux/README.md](linux/README.md)

---

## Features

- **WiFi beacon + NAN** @ 1 Hz, full ODID message pack per transmit
- **8 vendor profiles** — edit `RemoteIDSpoofer/drone_profiles.json`
- **Flight modes:** Patrol · Mission A→B · Waypoint multi-leg (`.wpt` / `.gpx`)
- **Pilot follow** — operator LIVE_GNSS trails ~30 m behind the drone (auto on for mission/waypoint)
- **Route tuning** — per-leg speed & altitude; interpolated climb along each leg
- **Regions:** `us` (FAA), `eu`, `generic` operator IDs

---

## Quick start

```bash
cd linux
sudo apt install -y build-essential libpcap-dev linux-headers-$(uname -r)
make
cp rids.conf.example rids.conf
# edit rids.conf — see linux/README.md
# install morrownr RTL8814AU driver (once) — see linux/README.md
sudo ./scripts/alfa_spoof.sh
```

Press **Ctrl+C** to stop.

---

## Flight modes

Priority: **Waypoint** → **Mission** → **Patrol** (default)

### Patrol

Random flight around `lat` / `lon`.

### Mission A → B

Straight-line flight with cruise speed, optional altitude interpolation (`alt_m` → `alt_b_m`), ping-pong loop.

```ini
lat=-6.2088
lon=106.8456
lat_b=-6.1988
lon_b=106.8556
speed_ms=12
alt_m=50
alt_b_m=80
loop=1
```

### Waypoint route

Multi-leg path from a file. CSV format: `lat,lon[,speed_mps][,alt_m]`

```ini
waypoints=routes/mission.wpt
speed_ms=12
alt_m=50
loop=1
pilot_follow=1
pilot_offset_m=30
```

Example `linux/routes/mission.wpt`:

```text
-6.2088,106.8456,12,45
-6.2050,106.8500,8,70
-6.2000,106.8550,15,100
```

---

## Configuration (`linux/rids.conf`)

| Key | Meaning |
|-----|---------|
| `iface` | Monitor interface |
| `channel` | 2.4 GHz channel (default `6`) |
| `lat` / `lon` | Patrol center / point A |
| `lat_b` / `lon_b` | Point B — enables mission mode |
| `waypoints` | Route file — enables waypoint mode (highest priority) |
| `speed_ms` | Default cruise speed (m/s) |
| `alt_m` | Default altitude AGL (m) |
| `alt_b_m` | Mission point-B altitude |
| `loop` | `1` = loop route, `0` = one-way |
| `pilot_follow` | `1` = operator follows behind drone |
| `pilot_offset_m` | Distance behind drone (m, default `30`) |
| `drones` | Count 1–16 |
| `region` | `us`, `eu`, `generic` |

**CLI examples:**

```bash
# Mission
sudo ./rids-spoofer -i wlan0mon -lat -6.2088 -lon 106.8456 \
  -latB -6.1988 -lonB 106.8556 -speed 12 -alt 50 -altB 80 -loop 1 -n 4

# Waypoint
sudo ./rids-spoofer -i wlan0mon -waypoints routes/mission.wpt -speed 12 -n 4
```

---

## Verify injection (before first spoof)

```bash
cd linux
IFACE=$(./scripts/detect_alfa.sh)
sudo ./scripts/setup_monitor.sh "$IFACE" 6
sudo apt install -y aircrack-ng
sudo aireplay-ng --test "$IFACE"
# expect: Injection is working!
```

---

## Scanner tips (OpenDroneID app)

- Disconnect phone from home WiFi
- Match channel **6** (default) on Alfa and scanner
- Wait 5–10 minutes; start with `drones=4` if slow
- Spoofer sends both **beacon** and **WiFi NAN** frames

---

## Project layout

```
linux/                    ← use this (Alfa port)
  rids-spoofer            ← binary
  scripts/alfa_spoof.sh   ← recommended entry point
  routes/mission.wpt      ← example waypoint route
  rids.conf.example
RemoteIDSpoofer/          ← shared OpenDroneID core + drone profiles + legacy ESP
```

---

## Disclaimer

Educational / lab use only. Broadcasting false Remote ID may be illegal in your jurisdiction. You are responsible for compliance.
