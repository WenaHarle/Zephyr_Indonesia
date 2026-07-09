# Belajar Zephyr RTOS di ESP32-S3

Ini catatan belajar pribadi, bukan dokumentasi resmi Zephyr Project maupun Espressif. Isinya adalah apa yang saya pelajari, coba, dan kadang gagal, sambil memahami Zephyr RTOS dengan target board ESP32-S3-DevKitC-1. Kalau ada yang tidak sesuai dengan dokumentasi resmi terbaru, percayai dokumentasi resmi — repo ini cuma jalan pintas biar konteksnya tidak hilang lagi kalau lupa.

Target audiens: siapa pun yang sudah bisa C dan sudah pernah pegang mikrokontroler (Arduino, STM32, ESP-IDF murni, apa saja), tapi baru mulai masuk ke Zephyr. Saya tidak menjelaskan dasar-dasar C atau elektronika dasar (resistor pull-up, apa itu GPIO, dll), karena itu sudah di luar cakupan.

## Kenapa Zephyr, kenapa bukan ESP-IDF saja

ESP-IDF tetap pilihan paling matang kalau proyeknya cuma jalan di chip Espressif. Saya pindah ke Zephyr karena portabilitas devicetree dan abstraksi HAL-nya lebih konsisten lintas vendor, dan karena beberapa proyek saya butuh dukungan multi-board tanpa nulis ulang driver tiap kali ganti chip. Kalau proyek Anda cuma sekali pakai dan tidak akan pernah pindah board, ESP-IDF murni mungkin lebih cepat sampai ke hasil. Trade-off ini nyata, bukan basa-basi.

## Prasyarat

Hardware:

- Board ESP32-S3-DevKitC-1 (revisi apa saja, saya pakai yang N8R8 — 8 MB flash, 8 MB PSRAM)
- Kabel USB-C ke USB-A yang mendukung transfer data (banyak kabel charging-only yang tidak bisa dipakai, ini jebakan klasik)
- Breadboard, kabel jumper male-to-male dan male-to-female
- Beberapa LED (minimal 3-4 warna berbeda untuk latihan multi-LED)
- Resistor 220-330 ohm untuk LED, beberapa
- Push button tactile switch, minimal 1-2 buah
- Sensor I2C untuk bagian 9 — MPU6050 (accelerometer/gyro) atau sensor suhu seperti BMP280/AHT20, apa saja yang Anda punya

Software/OS:

- Ubuntu 22.04 LTS atau 24.04 LTS (dua-duanya saya coba, keduanya bekerja tanpa masalah berarti)
- Akses sudo di mesin tersebut
- Koneksi internet yang stabil untuk pertama kali `west update` (unduhan awal workspace Zephyr bisa lebih dari 1 GB)

## Catatan versi

Tulisan ini disusun dengan Zephyr v4.2 sebagai acuan utama (west manifest, HAL Espressif, dan Zephyr SDK yang saya pakai semuanya dari rilis itu). Perintah west dan struktur devicetree yang dipakai di sini seharusnya tetap valid untuk rilis 4.x berikutnya, tapi kalau Anda membaca ini beberapa bulan setelah ditulis, cek dulu [release notes Zephyr](https://docs.zephyrproject.org/latest/releases/index.html) — nama qualifier board atau simbol Kconfig kadang berubah antar rilis minor. Board qualifier yang dipakai di seluruh materi ini adalah `esp32s3_devkitc/esp32s3/procpu` (core utama, bukan `appcpu`).

## Daftar Isi

1. [Workspace dan Toolchain](01-workspace-dan-toolchain/README.md) — estimasi 1.5-2 jam (termasuk waktu unduh). Instalasi dependensi, west, Zephyr SDK, toolchain Espressif, verifikasi.
2. [Aplikasi Pertama](02-aplikasi-pertama/README.md) — estimasi 45 menit. Build hello_world, flash, serial monitor, modifikasi kecil.
3. [Struktur dan Konfigurasi Aplikasi](03-struktur-dan-konfigurasi-aplikasi/README.md) — estimasi 1.5 jam. Anatomi aplikasi, Kconfig, menuconfig, bikin Kconfig sendiri.
4. [Blinky di ESP32-S3](04-blinky-di-esp32s3/README.md) — estimasi 1.5-2 jam. GPIO API, WS2812 onboard, LED eksternal, pola SOS.
5. [Devicetree dari Nol](05-devicetree-dari-nol/README.md) — estimasi 2 jam. Sintaks devicetree, overlay, macro DT_, tiga LED plus tombol.
6. [Porting dan Bring-up Board Kustom](06-porting-dan-bring-up-board/README.md) — estimasi 3 jam ke atas, ini bagian terberat. Board definition sendiri, hardware model v2.
7. [Aplikasi Blinky Board Kustom](07-aplikasi-blinky-board-kustom/README.md) — estimasi 1.5 jam. Aplikasi devicetree-driven murni, blinky multi-LED dengan tombol.
8. [Interrupt dan Workqueue](08-interrupt-dan-workqueue/README.md) — estimasi 2 jam. GPIO interrupt, system workqueue, debouncing.
9. [Proyek Akhir](09-proyek-akhir/README.md) — estimasi 4 jam ke atas, dikerjakan bertahap. Data logger sensor dengan shell interaktif, threading, message queue.

Total waktu kalau dikerjakan runtut tanpa banyak troubleshooting: kira-kira 18-20 jam kerja aktif. Realistisnya, alokasikan dua sampai tiga kali lipat itu karena akan ada momen debugging yang tidak terduga — terutama di bagian 6.

## Struktur folder

Tiap folder bernomor berisi satu README.md sebagai materi utama, subfolder `src/` (atau `overlay/`, `boards/` sesuai konteks) berisi kode yang bisa langsung di-build, dan subfolder `images/` sebagai tempat menaruh screenshot atau foto yang direferensikan dari README. Sebagian besar slot gambar masih placeholder karena foto wiring dan screenshot menuconfig harus diambil manual sesuai setup masing-masing.

## Sumber

- [Zephyr Project Documentation](https://docs.zephyrproject.org/latest/)
- [Espressif Zephyr Getting Started](https://docs.espressif.com/projects/zephyr/en/latest/)
- [Zephyr GitHub repository](https://github.com/zephyrproject-rtos/zephyr)
