# Remote ID Spoofer — Linux / Alfa RTL8814AU

Port ini menjalankan Remote ID Spoofer di **Linux** dengan USB WiFi adapter yang mendukung **monitor mode + packet injection**.

Didukung untuk adapter berbasis **Realtek RTL8814AU**, termasuk:

| Adapter | Chipset | USB ID (contoh) |
|---------|---------|-----------------|
| Alfa AWUS1900 | RTL8814AU | `0bda:8813` |
| Alfa AWUS036ACH (varian 8814) | RTL8814AU | `0bda:8813` |
| Adapter USB generik RTL8814AU | RTL8814AU | `0bda:8813` |

Firmware ESP8266/ESP32 tetap ada; folder `linux/` adalah backend alternatif.

---

## Quick start (ringkas)

```bash
# 1. Install driver Alfa (lihat bagian lengkap di bawah)
# 2. Build spoofer
cd linux
sudo apt install -y build-essential libpcap-dev   # Debian/Ubuntu
make

# 3. Cari nama interface
iw dev

# 4. Monitor mode + channel
sudo ./scripts/setup_monitor.sh wlan0 6

# 5. Spoof
sudo ./rids-spoofer -i wlan0 -c 6 -lat -6.2088 -lon 106.8456 -n 16
```

---

## Setup lengkap: Alfa RTL8814AU

### Langkah 0 — Persiapan sistem

**OS yang disarankan:** Ubuntu 22.04/24.04, Debian 12, Kali Linux, atau Raspberry Pi OS (64-bit).

Colokkan adapter Alfa **sebelum** atau **sesudah** install driver — keduanya OK, tapi setelah install driver biasanya perlu **cabut-colok ulang** USB.

Install paket build:

```bash
# Debian / Ubuntu / Kali / Raspberry Pi OS
sudo apt update
sudo apt install -y build-essential dkms git iw libpcap-dev linux-headers-$(uname -r)
```

Verifikasi adapter terdeteksi:

```bash
lsusb | grep -i "0bda:8813\|Realtek\|Alfa"
```

Output yang diharapkan (contoh AWUS1900):

```
Bus 001 Device 00X: ID 0bda:8813 Realtek Semiconductor Corp. RTL8814AU ...
```

> **Catatan:** Chip RTL8814AU **tidak** didukung penuh di kernel Linux mainline. Wajib pakai driver patched di bawah.

---

### Langkah 1 — Hapus driver bawaan yang bentrok (jika ada)

Driver kernel default (`rtw88_8814au`, `8814au` lama, dll.) sering **tidak** support injection dengan baik.

```bash
# Cek modul yang sedang loaded
lsmod | grep -E '8814|88xx|rtw'

# Blacklist driver bawaan (buat file baru)
sudo tee /etc/modprobe.d/blacklist-rtl8814au.conf <<'EOF'
blacklist rtw88_8814au
blacklist 8814au
EOF

sudo update-initramfs -u   # Debian/Ubuntu
```

Cabut-colok ulang adapter, atau reboot.

---

### Langkah 2 — Install driver patched (disarankan: morrownr)

Driver ini paling sering dipakai komunitas pentest karena monitor mode + injection relatif stabil.

```bash
cd ~
git clone https://github.com/morrownr/8814au.git
cd 8814au
sudo ./install-driver.sh
```

Script di atas akan:

- Compile modul kernel `8814au`
- Install via DKMS (auto-rebuild saat kernel update)
- Load driver ke kernel

**Setelah selesai:**

```bash
# Cabut-colok USB Alfa, lalu cek:
lsmod | grep 8814au
iw dev
```

Output `iw dev` yang diharapkan:

```
phy#X
	Interface wlan0
		ifindex X
		wdev 0x...
		addr XX:XX:XX:XX:XX:XX
		type managed
		channel 1 (2412 MHz), width: 20 MHz, center1: 2412 MHz
```

Catat nama interface (biasanya `wlan0`, `wlx...`, atau `wlp...`).

#### Verifikasi driver support monitor + inject

```bash
PHY=$(iw dev wlan0 info | awk '/wiphy/ {print $2}')
iw phy "$PHY" info | grep -A5 "Supported interface modes"
```

Harus ada baris `* monitor`.

Cek injection (opsional, butuh `aircrack-ng`):

```bash
sudo apt install -y aircrack-ng
sudo airmon-ng start wlan0
# Interface monitor biasanya: wlan0mon
sudo aireplay-ng --test wlan0mon
```

Jika muncul `Injection is working!` → driver siap dipakai.

#### Uninstall driver (jika perlu ganti)

```bash
cd ~/8814au
sudo ./remove-driver.sh
```

---

### Langkah 3 — Alternatif driver: aircrack-ng

Jika driver morrownr gagal di distro/kernel Anda:

```bash
sudo apt install -y bc libelf-dev
cd ~
git clone -b v5.2.20 https://github.com/aircrack-ng/rtl8814au.git
cd rtl8814au
make
sudo make install
sudo modprobe 8814au
```

Cabut-colok adapter, lalu ulangi verifikasi `iw dev` dan `aireplay-ng --test`.

---

### Langkah 4 — Build Remote ID Spoofer

```bash
cd /path/to/RemoteIDSpoofer/linux
make
# atau
./build.sh
```

Binary: `linux/rids-spoofer`

---

### Langkah 5 — Aktifkan monitor mode

Ganti `wlan0` dengan interface adapter Anda (dari `iw dev`).

```bash
cd linux
chmod +x scripts/setup_monitor.sh
sudo ./scripts/setup_monitor.sh wlan0 6
```

Script ini akan:

1. Stop NetworkManager pada interface tersebut
2. Set type ke **monitor**
3. Set channel **6** (HT20) — sama default firmware ESP
4. Print perintah restore ke managed mode

**Manual (jika script gagal):**

```bash
sudo nmcli dev set wlan0 managed no
sudo ip link set wlan0 down
sudo iw dev wlan0 set type monitor
sudo ip link set wlan0 up
sudo iw dev wlan0 set channel 6 HT20
```

**Restore WiFi normal setelah selesai tes:**

```bash
sudo ip link set wlan0 down
sudo iw dev wlan0 set type managed
sudo ip link set wlan0 up
sudo nmcli dev set wlan0 managed yes
```

---

### Langkah 6 — Jalankan spoofer

```bash
sudo ./rids-spoofer -i wlan0 -c 6 -lat -6.2088 -lon 106.8456 -n 16
```

| Flag | Deskripsi | Default |
|------|-----------|---------|
| `-i` | Interface monitor (**wajib**) | — |
| `-c` | Channel WiFi (1–13 untuk 2.4 GHz) | `6` |
| `-lat` | Latitude pusat simulasi drone | `-6.2088` |
| `-lon` | Longitude pusat simulasi drone | `106.8456` |
| `-n` | Jumlah drone palsu (1–16) | `16` |

Output normal:

```
WiFi TX ready on wlan0 (channel 6, 2437 MHz)
Broadcasting 16 Remote ID drone(s) on wlan0 channel 6 around -6.208800, 106.845600
Press Ctrl+C to stop.
```

Jika muncul `pcap_inject failed` → lihat [Troubleshooting](#troubleshooting).

---

### Langkah 7 — Verifikasi spoof berhasil

**Di mesin yang sama (sniffer):**

```bash
# Terminal baru — jangan stop spoofer
sudo tcpdump -i wlan0 -n -e type mgt subtype beacon 2>/dev/null | head
```

Harus terlihat beacon frame keluar terus-menerus.

**Di smartphone (OpenDroneID app):**

1. Matikan WiFi ke router / disconnect dari jaringan rumah
2. Buka app OpenDroneID
3. Disable scan throttling (Android, jika ada opsi developer)
4. Tunggu **5–10 menit** — 16 drone tidak selalu muncul instan
5. Pastikan channel scanner match (coba channel 6)

---

## Troubleshooting

### Driver & hardware

| Gejala | Penyebab | Solusi |
|--------|----------|--------|
| `lsusb` tidak lihat `0bda:8813` | USB/port/cable | Ganti port USB 3.0, coba kabel lain |
| `lsmod` tidak ada `8814au` | Driver belum terinstall | Ulangi Langkah 2, reboot |
| `install-driver.sh` gagal compile | Header kernel hilang | `sudo apt install linux-headers-$(uname -r)` |
| Secure Boot error | Modul unsigned ditolak | Disable Secure Boot di BIOS, atau sign module |
| Interface muncul tapi no monitor | Driver salah / bentrok | Blacklist driver bawaan (Langkah 1), reinstall morrownr |

### Spoofer & injection

| Gejala | Penyebab | Solusi |
|--------|----------|--------|
| `pcap_activate` gagal | Bukan monitor mode / bukan root | `sudo ./scripts/setup_monitor.sh wlan0 6` |
| `pcap_inject failed` | Driver tidak support inject | Tes `aireplay-ng --test`; ganti ke driver morrownr |
| Beacon keluar tapi app kosong | Channel mismatch / HP masih connect WiFi | Set channel 6; disconnect HP dari AP |
| Hanya beberapa drone terlihat | Rate scan HP lambat | Kurangi `-n 4` dulu; tunggu lebih lama |
| `Operation not permitted` | Tanpa sudo | `sudo ./rids-spoofer ...` |

### NetworkManager ganggu interface

```bash
sudo systemctl stop NetworkManager
# ... jalankan spoofer ...
sudo systemctl start NetworkManager
```

Atau permanen ignore interface Alfa:

```bash
sudo nmcli dev set wlan0 managed no
```

---

## Arsitektur

```
main.cpp          → CLI + loop N drone
spoofer.cpp       → simulasi gerak drone
id_open.cpp       → bangun beacon OpenDroneID (shared)
id_open_linux.cpp → backend transmit Linux
wifi_tx.cpp       → radiotap header + pcap_inject
opendroneid.c     → library ASTM (shared)
```

Transmisi memakai **802.11 management frame injection** — setara `wifi_send_pkt_freedom()` di ESP8266, tapi via libpcap di Linux.

---

## Catatan legal

Spoofing Remote ID dapat melanggar regulasi telekomunikasi dan aviasi di yurisdiksi Anda. Hanya gunakan di lingkungan terkontrol untuk penelitian/edukasi. Penulis tidak bertanggung jawab atas penyalahgunaan software ini.
