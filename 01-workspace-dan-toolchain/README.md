# Bagian 1 — Workspace dan Toolchain

Sebelum menulis satu baris kode pun, ada satu hari kerja (kalau lancar) yang harus dihabiskan cuma untuk menyiapkan lingkungan build. Zephyr tidak seperti Arduino IDE yang tinggal klik install — ada beberapa lapis yang harus dipasang berurutan: dependensi sistem, Python venv, west (tool CLI Zephyr), workspace manifest, Zephyr SDK (toolchain compiler untuk semua arsitektur), dan khusus untuk ESP32-S3 ada toolchain serta blob biner tambahan dari Espressif. Urutannya penting, jangan diloncat.

Semua perintah di bawah saya jalankan di Ubuntu 24.04 LTS bersih (fresh install di VM). Saya juga sudah uji ulang di Ubuntu 22.04 dan hasilnya sama, hanya versi beberapa paket apt yang beda tapi tidak berpengaruh.

## Dependensi sistem

Install paket-paket dasar yang dibutuhkan west, CMake, device tree compiler, dan toolchain build:

```bash
sudo apt update
sudo apt upgrade -y
sudo apt install -y --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-venv python3-tk xz-utils file make gcc \
  gcc-multilib g++-multilib libsdl2-dev libmagic1 \
  libusb-1.0-0-dev
```

Cek versi CMake dan Python — Zephyr butuh CMake minimal 3.20 dan Python minimal 3.10. Ubuntu 22.04 default sudah cukup baru untuk keduanya, jadi biasanya tidak perlu upgrade manual.

```bash
cmake --version
python3 --version
```

Kalau `libusb-1.0-0-dev` tidak ditemukan di Ubuntu Anda (versi lama), cek nama paketnya lewat `apt search libusb-1.0`.

## Python virtual environment

Jangan install west atau paket Python Zephyr lain secara global dengan `pip install --user` atau lebih parah `sudo pip install`. Ini akan bentrok dengan paket Python sistem dan menyulitkan upgrade nanti. Pakai venv:

```bash
mkdir -p ~/zephyrproject
python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate
```

Setelah diaktifkan, prompt shell akan menampilkan `(.venv)` di depan. Setiap kali mau kerja dengan Zephyr, venv ini harus diaktifkan dulu. Saya biasanya menambahkan alias di `~/.bashrc` atau `~/.zshrc`:

```bash
echo 'alias zephyr-env="source ~/zephyrproject/.venv/bin/activate"' >> ~/.zshrc
```

## Install west

West adalah tool CLI yang mengatur multi-repository manifest Zephyr (mirip `repo` di proyek Android atau `vcstool` di ROS). Install di dalam venv yang tadi diaktifkan:

```bash
pip install west
```

## Inisialisasi workspace

Workspace Zephyr punya struktur khusus: ada folder `zephyr/` (source Zephyr itu sendiri), folder `modules/` (HAL vendor, driver eksternal), dan file manifest `.west/config`. Semua ini diatur otomatis oleh `west init` dan `west update`, jangan clone manual dengan `git clone`.

```bash
west init ~/zephyrproject
cd ~/zephyrproject
west update
```

Perintah `west update` ini yang paling lama — mengunduh semua modul termasuk HAL Espressif, HAL berbagai vendor lain yang terdaftar di manifest default, dan lain-lain. Bisa 20-30 menit tergantung koneksi, dan ukurannya bisa lebih dari 6 GB kalau dihitung semua HAL vendor (meski yang benar-benar dipakai untuk ESP32-S3 cuma sebagian kecil). Kalau koneksi terputus di tengah jalan, tinggal jalankan `west update` lagi, dia akan melanjutkan bukan mengulang dari nol.

Setelah selesai, install requirement Python tambahan yang dipakai script build Zephyr:

```bash
pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt
```

## Zephyr SDK

Zephyr SDK berisi toolchain compiler (GCC cross-compiler) untuk semua arsitektur yang didukung Zephyr, termasuk Xtensa (arsitektur core ESP32-S3). Jangan pakai toolchain Xtensa dari ESP-IDF untuk build Zephyr — meski sama-sama Xtensa, versi dan patch-nya berbeda dan sering menyebabkan error build yang membingungkan.

```bash
cd ~
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/zephyr-sdk-0.17.0_linux-x86_64.tar.xz
wget -O - https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/sha256.sum | shasum --check --ignore-missing
tar xvf zephyr-sdk-0.17.0_linux-x86_64.tar.xz
cd zephyr-sdk-0.17.0
./setup.sh
```

Cek dulu di [halaman rilis sdk-ng](https://github.com/zephyrproject-rtos/sdk-ng/releases) untuk nomor versi SDK yang direkomendasikan untuk versi Zephyr yang Anda pakai — biasanya tercantum di `zephyr/SDK_VERSION` di dalam source tree Zephyr. Jangan asal pakai versi terbaru kalau versi Zephyr Anda lebih lama, kadang tidak kompatibel.

Script `setup.sh` akan menawarkan install udev rules untuk board debugging (J-Link, OpenOCD, dst). Jawab "yes" untuk semua toolchain yang ditanyakan kalau tidak yakin arsitektur mana saja yang dipakai — ruang disk tambahan tidak signifikan dibanding waktu yang hilang kalau nanti ternyata kurang.

## Toolchain dan HAL Espressif

Selain Zephyr SDK, ESP32-S3 butuh beberapa komponen tambahan dari HAL Espressif: toolchain khusus untuk beberapa tool flashing/OTA, dan blob biner (bootloader stage tertentu, WiFi/BT firmware blob) yang tidak didistribusikan sebagai source karena lisensinya dari Espressif.

```bash
source ~/zephyrproject/.venv/bin/activate
cd ~/zephyrproject
west blobs fetch hal_espressif
```

Perintah ini mengunduh blob biner yang terdaftar di modul `hal_espressif` — termasuk firmware WiFi/Bluetooth kalau board Anda memakainya nanti, dan beberapa bootloader stage 2 untuk chip Espressif tertentu. Untuk kasus blinky sederhana blob-blob ini mungkin tidak semuanya kepakai, tapi fetch saja sekarang supaya tidak ada kejutan "file not found" di tengah build nanti.

Espressif juga mendistribusikan toolchain sendiri (`riscv32-esp-elf` dan `xtensa-esp-elf`) lewat `west espressif` — untuk kebanyakan build aplikasi Zephyr biasa, Zephyr SDK saja sudah cukup karena compiler Xtensa/RISC-V generik sudah ada di dalamnya. Tapi kalau nanti butuh tool tambahan seperti `esptool.py` versi terbaru dari Espressif, environment variable `ESPRESSIF_TOOLCHAIN_PATH` bisa diarahkan lewat instalasi terpisah. Untuk keperluan tulisan ini, Zephyr SDK plus blob dari `hal_espressif` sudah cukup untuk semua bagian sampai proyek akhir.

## Verifikasi instalasi

Cara paling cepat verifikasi semua terpasang benar adalah langsung build sample:

```bash
cd ~/zephyrproject/zephyr
west build -p always -b esp32s3_devkitc/esp32s3/procpu samples/hello_world
```

Kalau build selesai tanpa error dan diakhiri baris semacam `Memory region  Used Size  Region Size  %age Used`, berarti toolchain, SDK, dan HAL semuanya sudah nyambung dengan benar. Detail build dan flashing yang sebenarnya dibahas di [Bagian 2](../02-aplikasi-pertama/README.md) — bagian ini cuma untuk memastikan environment beres dulu.

![Output west build yang berhasil](images/01-build-sukses.png)
<!-- TODO(author): tambahkan screenshot terminal setelah west build selesai tanpa error, tampilkan bagian Memory region -->

## Troubleshooting akses USB

Ini bagian yang paling sering bikin frustrasi buat yang baru pindah dari Windows/macOS ke Linux: board terdeteksi lsusb tapi `west flash` gagal dengan permission denied di `/dev/ttyUSB0` atau `/dev/ttyACM0`.

Cek dulu device muncul di mana:

```bash
dmesg | tail -20
ls -l /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

ESP32-S3-DevKitC-1 biasanya muncul sebagai `/dev/ttyACM0` kalau memakai port USB native (JTAG/serial bawaan chip), atau `/dev/ttyUSB0` kalau lewat chip USB-to-serial converter (CP2102/CH340) tergantung revisi board. Cek permission-nya:

```bash
ls -l /dev/ttyACM0
```

Kalau ownership-nya `root:dialout` dan user Anda belum masuk grup `dialout`, tambahkan:

```bash
sudo usermod -aG dialout $USER
```

Setelah ini **logout dan login lagi** (atau reboot) — perubahan grup tidak langsung berlaku di sesi shell yang sedang jalan, ini jebakan yang saya sendiri lupa berkali-kali dan bingung kenapa masih permission denied padahal sudah `usermod`.

Alternatif tanpa logout, cukup untuk sesi terminal saat itu saja:

```bash
newgrp dialout
```

Kalau board dua-duanya tidak muncul sama sekali di `/dev`, coba kabel USB lain (banyak kabel cuma untuk charging, tidak ada jalur data), atau coba port USB fisik yang berbeda di komputer.

## Troubleshooting lain yang pernah saya temui

Build gagal dengan pesan semacam `Unable to find Zephyr SDK` — biasanya karena environment variable `ZEPHYR_SDK_INSTALL_DIR` tidak terdeteksi. Set manual atau pastikan script `setup.sh` SDK dijalankan sampai selesai (dia menulis file cmake package registry di `~/.cmake/packages/Zephyr-sdk/`).

Error `west: command not found` setelah membuka terminal baru — venv belum diaktifkan lagi. Solusinya ya aktifkan lagi venv, atau pakai alias yang sudah disiapkan di atas.

Kalau `west update` macet di tengah dengan error timeout koneksi ke GitHub, jalankan ulang saja `west update`, dia resume dari commit yang belum ter-fetch. Kalau sering putus, cek dulu apakah ada proxy/firewall kantor yang membatasi koneksi git.

Kalau muncul error seperti ini:
```
The module for fetcher "git_lfs" could not be imported (No module named 'requests').
The module for fetcher "http" could not be imported (No module named 'requests').
Missing jsonschema dependency
```
artinya venv Anda kehilangan dua dependency Python yang dipakai west's blob fetcher — `requests` dan `jsonschema`. Ini biasanya kejadian kalau langkah `pip install -r requirements.txt` di bagian "Inisialisasi workspace" sebelumnya sempat terlewat atau gagal sebagian. Perbaikannya, install ulang requirements resmi (lebih aman daripada install manual dua paket saja, karena sekalian menangkap dependency lain yang mungkin ikut hilang):
```bash
pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt
```
Kalau masih mau cepat tanpa install ulang semua requirements, cukup:
```bash
pip install requests jsonschema
```
Setelah itu jalankan lagi `west blobs fetch hal_espressif`.
```

Selain itu, sebaiknya tambahkan satu baris juga di bagian **Troubleshooting lain yang pernah saya temui**, biar orang yang scroll ke situ dulu (tanpa baca urut) tetap ketemu solusinya:

```markdown
Error `No module named 'requests'` atau `Missing jsonschema dependency` saat `west blobs fetch` — venv belum lengkap paketnya. Jalankan `pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt` di dalam venv yang aktif.
```

Ini konsisten dengan gaya dokumen kamu yang lain — errornya ditaruh dekat command pemicunya, plus disebut ulang ringkas di ringkasan troubleshooting akhir.

## Sumber

- [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
- [Espressif Zephyr Getting Started](https://docs.espressif.com/projects/zephyr/en/latest/introduction.html)
- [Zephyr SDK releases](https://github.com/zephyrproject-rtos/sdk-ng/releases)
- [West documentation](https://docs.zephyrproject.org/latest/develop/west/index.html)
