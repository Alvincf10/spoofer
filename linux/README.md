# Alfa RTL8814AU — Remote ID Spoofer

**Jalur utama development:** Linux + adapter **Alfa AWUS1900** (chip **RTL8814AU**) + driver patched + binary `rids-spoofer`.

| Platform | Status |
|----------|--------|
| Linux + Alfa RTL8814AU | Didukung |
| macOS / Windows | Tidak didukung |
| ESP8266/ESP32 (`../RemoteIDSpoofer/`) | Legacy — tidak dipakai untuk setup Alfa |

---

## Ringkasan fitur

- Hingga **16 drone** simultan, profil komersial (DJI, Autel, Skydio, Parrot)
- **WiFi beacon + WiFi NAN** @ 1 Hz, full OpenDroneID pack per transmit
- MAC BSSID dengan **OUI vendor** per merek
- Serial, operator ID (US/EU/generic), LIVE_GNSS pilot, telemetry airborne realistis
- Tiga mode terbang: **Patrol**, **Mission A→B**, **Waypoint multi-leg**
- **Pilot follow** — operator mengikuti di belakang drone (mission/waypoint)
- **Tuning rute** — kecepatan & altitude per leg (file `.wpt`)

Profil drone: `../RemoteIDSpoofer/drone_profiles.json`

---

## Apa yang di-broadcast

Setiap drone mengirim **beacon management frame** + **WiFi NAN action frame** berisi ASTM OpenDroneID:

| Pesan ODID | Isi |
|------------|-----|
| Basic ID ×2 | Serial vendor (`1581F…`, `AU7YC…`) + CAA registration (operator ID) |
| Location | Status **AIRBORNE**, lat/lon, speed, heading, alt MSL/AGL, baro, vertical speed |
| System | Operator **LIVE_GNSS**, timestamp, EU class |
| Operator ID | FAA / EU / generic (`region` di config) |
| Self ID | Recreational / Commercial per profil |

**Teknis transmit:** 1 Hz · beacon interval 100 TU · `message_counter` / `nan_send_counter` per drone · injection via **libpcap** di interface monitor.

---

## Mode terbang

Prioritas mode (tinggi → rendah):

1. **Waypoint** — jika `waypoints=` atau `-waypoints` di-set
2. **Mission** — jika `lat_b`/`lon_b` atau `-latB`/`-lonB` di-set
3. **Patrol** — default

### Patrol

Terbang acak di sekitar titik `lat`/`lon`. Pilot bergerak kecil di sekitar base (kecuali `pilot_follow=1`).

```ini
lat=-6.2088
lon=106.8456
drones=8
```

### Mission A → B

Terbang lurus A→B dengan kecepatan konstan. Multi-drone: jalur paralel + stagger awal.

| Fitur | Detail |
|-------|--------|
| Kecepatan | `speed_ms` (default 12 m/s) |
| Loop | `loop=1` bolak-balik A↔B, `loop=0` berhenti di B |
| Altitude | Interpolasi AGL `alt_m` → `alt_b_m` sepanjang rute |
| Pilot | **Otomatis follow** ~30 m di belakang arah terbang |

```ini
lat=-6.2088
lon=106.8456
lat_b=-6.1988
lon_b=106.8556
speed_ms=12
alt_m=50
alt_b_m=80
loop=1
pilot_follow=1
pilot_offset_m=30
```

### Waypoint multi-leg

Rute dari file `.wpt` (CSV) atau `.gpx`. Setiap **leg** punya speed & altitude sendiri.

**Format CSV** (`routes/mission.wpt`):

```text
# lat,lon[,speed_mps][,alt_agl_m]
-6.2088,106.8456,12,45
-6.2050,106.8500,8,70
-6.2000,106.8550,15,100
-6.1988,106.8556,10,55
```

- Kolom 3 = kecepatan cruise **leg yang dimulai** di titik itu (kosong → `speed_ms` global)
- Kolom 4 = target altitude AGL di titik (kosong → `alt_m` global)
- Altitude di-interpolasi sepanjang leg (max climb ~3 m/s)

```ini
waypoints=routes/mission.wpt
speed_ms=12
alt_m=50
loop=1
pilot_follow=1
```

**GPX:** baris `<trkpt lat="…" lon="…"/>` juga didukung (tanpa speed/alt per titik).

---

## Quick start

```bash
cd linux
sudo apt install -y build-essential dkms git iw libpcap-dev linux-headers-$(uname -r)
make
cp rids.conf.example rids.conf
nano rids.conf
# install driver morrownr (sekali) — lihat bawah
sudo ./scripts/alfa_spoof.sh
```

**Stop:** `Ctrl+C`

---

## Cara menjalankan

### Opsi 1 — `alfa_spoof.sh` (disarankan)

```bash
sudo ./scripts/alfa_spoof.sh                  # baca rids.conf, setup monitor, jalankan
sudo ./scripts/alfa_spoof.sh --no-setup     # monitor sudah aktif
sudo ./scripts/alfa_spoof.sh -c /path/rids.conf
```

### Opsi 2 — Manual

```bash
IFACE=$(./scripts/detect_alfa.sh)
sudo ./scripts/setup_monitor.sh "$IFACE" 6
sudo ./rids-spoofer -config rids.conf
```

### Opsi 3 — CLI tanpa config

```bash
# Patrol
sudo ./rids-spoofer -i wlan0mon -c 6 -lat -6.2 -lon 106.8 -n 8 -region generic

# Mission A → B
sudo ./rids-spoofer -i wlan0mon \
  -lat -6.2088 -lon 106.8456 \
  -latB -6.1988 -lonB 106.8556 \
  -speed 12 -alt 50 -altB 80 -loop 1 -n 4 \
  -pilot_follow 1 -pilot_offset 30

# Waypoint route
sudo ./rids-spoofer -i wlan0mon \
  -waypoints routes/mission.wpt \
  -speed 12 -alt 50 -loop 1 -n 4
```

### Output saat jalan

```text
Alfa/Linux Remote ID Spoofer
Interface: wlan0mon  Channel: 6  Region: generic
Flight: WAYPOINT routes/mission.wpt (3 legs) @ 12.0 m/s [loop]
Transmit: beacon + WiFi NAN (1 Hz)
Pilot: follow (30 m behind)

  drone  1: DJI Mini 3 Pro
  ...
Broadcasting 8 drone(s) — Ctrl+C to stop
```

---

## Referensi `rids.conf`

Salin template: `cp rids.conf.example rids.conf`

### Jaringan & drone

| Key | Default | Fungsi |
|-----|---------|--------|
| `iface` | — | Interface monitor (`./scripts/detect_alfa.sh`) |
| `channel` | `6` | Channel 2.4 GHz |
| `drones` / `n` | `16` | Jumlah drone (1–16) |
| `region` | `generic` | Operator ID: `us`, `eu`, `generic` |

### Posisi & mode terbang

| Key | Default | Fungsi |
|-----|---------|--------|
| `lat` / `lon` | — | Patrol center / titik A (mission) |
| `lat_b` / `lon_b` | — | Titik B — aktifkan **mission** jika diisi |
| `waypoints` | — | File `.wpt`/`.gpx` — aktifkan **waypoint** (prioritas tertinggi) |
| `speed_ms` / `speed` | `12` | Kecepatan default (m/s) |
| `loop` | `1` | `1` = loop rute, `0` = one-way / berhenti di akhir |

### Altitude & pilot

| Key | Default | Fungsi |
|-----|---------|--------|
| `alt_m` / `alt` | `50` | Altitude AGL default (m) |
| `alt_b_m` / `altB` | sama `alt_m` | Altitude di titik B (mission) |
| `pilot_follow` | auto `1` untuk mission/wp | `1` = operator ikut di belakang drone |
| `pilot_offset_m` | `30` | Jarak operator di belakang drone (m) |

> **Pilot follow:** otomatis **ON** untuk mission & waypoint. Patrol tetap fixed base kecuali `pilot_follow=1` di-set eksplisit. Operator altitude = ground level (`base_alt_m`), bukan ikut ketinggian drone.

### Region → Operator ID

| `region` | Format |
|----------|--------|
| `us` / `faa` | FAA style `FA` + 10 karakter |
| `eu` | Prefix EU + Luhn check |
| `generic` | 16-char ID |

---

## Referensi CLI

```
Usage: rids-spoofer -i <iface> [options]

  -config FILE       Config file
  -i IFACE           Monitor interface
  -c CHANNEL         WiFi channel (default 6)
  -lat LAT           Latitude (A / patrol center)
  -lon LON           Longitude
  -latB LAT          Mission destination latitude
  -lonB LON          Mission destination longitude
  -speed MS          Cruise speed m/s
  -alt M             Default / mission-A altitude AGL
  -altB M            Mission-B altitude AGL
  -loop 0|1          Loop route / ping-pong
  -waypoints FILE    Waypoint file (.wpt / .gpx)
  -pilot_follow 0|1  Operator trails behind drone
  -pilot_offset M    Meters behind (default 30)
  -n COUNT           Drones 1–16
  -region REG        us|eu|generic
  -h                 Help
```

---

## Setup driver & hardware

### Hardware didukung

| Adapter | Chip | USB ID |
|---------|------|--------|
| Alfa AWUS1900 | RTL8814AU | `0bda:8813` |
| Generik RTL8814AU | RTL8814AU | `0bda:8813` |

```bash
lsusb | grep -i 8813
./scripts/detect_alfa.sh
```

### Install driver morrownr

Chip **tidak** ada di kernel mainline dengan injection stabil.

**Blacklist driver bentrok:**

```bash
sudo tee /etc/modprobe.d/blacklist-rtl8814au.conf <<'EOF'
blacklist rtw88_8814au
blacklist 8814au
EOF
sudo update-initramfs -u
```

**Install:**

```bash
cd ~
git clone https://github.com/morrownr/8814au.git
cd 8814au
sudo ./install-driver.sh
```

Cabut-colok Alfa, cek: `lsmod | grep 8814au`

### Tes injection (wajib)

```bash
sudo apt install -y aircrack-ng
IFACE=$(./scripts/detect_alfa.sh)
sudo ./scripts/setup_monitor.sh "${IFACE}" 6
sudo aireplay-ng --test "${IFACE}"
```

Harus muncul: **`Injection is working!`**

---

## Verifikasi & scanner

### Sniffer di PC yang sama

```bash
sudo tcpdump -i wlan0mon -n type mgt subtype beacon | head
```

### App OpenDroneID (HP)

1. **Putus** WiFi HP dari router / hotspot
2. Channel scanner = channel Alfa (default **6**)
3. Tunggu **5–10 menit**; mulai dengan `drones=4` jika lambat
4. Cek output spoofer: `Transmit: beacon + WiFi NAN`

---

## Troubleshooting

| Masalah | Solusi |
|---------|--------|
| `No RTL8814AU USB device` | Cek kabel / port USB 3 |
| `pcap_activate` gagal | `sudo ./scripts/setup_monitor.sh $(./scripts/detect_alfa.sh) 6` |
| `pcap_inject failed` | Ulangi `aireplay-ng --test`; reinstall morrownr |
| Beacon ada, app kosong | HP masih connect ke AP; channel beda |
| `waypoints: need at least 2 points` | File route minimal 2 baris koordinat |
| Secure Boot / modul gagal | Disable Secure Boot atau sign module |

**Restore WiFi normal:**

```bash
sudo ip link set wlan0 down
sudo iw dev wlan0 set type managed
sudo ip link set wlan0 up
sudo nmcli dev set wlan0 managed yes
```

---

## Arsitektur

```
linux/
  scripts/
    alfa_spoof.sh       # entry point: detect → monitor → spoofer
    detect_alfa.sh      # cari interface RTL8814AU
    setup_monitor.sh    # monitor mode + channel
  src/
    main.cpp            # CLI + config loader
    spoofer.cpp         # simulasi terbang + profil drone
    waypoints.cpp       # parser .wpt / .gpx
    wifi_tx.cpp         # radiotap + pcap_inject
    id_open_linux.cpp   # backend WiFi Linux
  routes/
    mission.wpt         # contoh rute multi-leg
  rids.conf.example
  rids-spoofer          # binary (setelah make)

../RemoteIDSpoofer/     # opendroneid, id_open, drone_profiles (shared)
```

Injection setara raw 802.11 transmit di firmware ESP, tetapi lewat **libpcap** pada interface monitor Alfa.

---

## Legal

Hanya untuk lab / penelitian terkontrol. Spoofing Remote ID dapat melanggar regulasi di yurisdiksi Anda. Anda bertanggung jawab atas kepatuhan hukum.
