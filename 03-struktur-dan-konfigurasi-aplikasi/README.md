# Bagian 3 — Struktur dan Konfigurasi Aplikasi

Sebelum masuk ke GPIO dan devicetree, penting paham dulu tiga file yang membentuk aplikasi Zephyr paling minimal: `CMakeLists.txt`, `prj.conf`, dan `src/main.c`. Ketiganya sering disalin-tempel begitu saja tanpa dipahami, padahal isinya menentukan hampir semua perilaku build.

## Anatomi aplikasi

Struktur direktori minimal aplikasi Zephyr:

```
aplikasi_saya/
├── CMakeLists.txt
├── prj.conf
└── src/
    └── main.c
```

**CMakeLists.txt** adalah entry point build system. Isinya minimal begini:

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aplikasi_saya)
target_sources(app PRIVATE src/main.c)
```

Baris `find_package(Zephyr REQUIRED ...)` yang melakukan keajaiban sebenarnya — dia menarik seluruh sistem build Zephyr (CMake modules, toolchain file, generator devicetree dan Kconfig) ke dalam project ini. Target bernama `app` sudah didefinisikan oleh sistem ini, kita tinggal menambahkan source file ke dalamnya lewat `target_sources`.

**prj.conf** adalah file konfigurasi Kconfig level aplikasi. Isinya daftar simbol konfigurasi yang mau diaktifkan atau diubah dari default, format `CONFIG_NAMA_SIMBOL=y` atau `=n` atau `=nilai`. File ini digabung (merge) dengan default Kconfig dari board dan SoC saat build.

**src/main.c** berisi fungsi `int main(void)` sebagai entry point aplikasi setelah kernel Zephyr selesai inisialisasi thread, driver, dan subsistem lain yang didaftarkan lewat `SYS_INIT` atau `DEVICE_DEFINE`. Ini beda dengan bare-metal biasa di mana `main` adalah titik masuk paling awal — di Zephyr, banyak hal sudah jalan duluan sebelum `main` dipanggil, termasuk driver-driver yang punya prioritas init lebih awal.

## Kconfig

Kconfig adalah sistem konfigurasi yang sama dipakai kernel Linux, dipinjam Zephyr untuk mengatur fitur mana yang dikompilasi masuk ke image akhir. Setiap subsistem Zephyr (logging, shell, driver GPIO, networking, dst) punya file `Kconfig` yang mendefinisikan simbol-simbol konfigurasinya, lengkap dengan dependency antar simbol, default value, dan help text.

Contoh simbol yang sering dipakai:

```
CONFIG_GPIO=y
CONFIG_LOG=y
CONFIG_PRINTK=y
CONFIG_MAIN_STACK_SIZE=2048
```

Kalau Anda aktifkan simbol yang butuh dependency tertentu tapi dependency-nya belum aktif, build akan gagal di tahap konfigurasi dengan pesan error yang menyebutkan simbol mana yang bermasalah — atau lebih sering, simbol yang Anda set begitu saja diam-diam tidak aktif karena dependency-nya tidak terpenuhi (ini yang lebih menjebak, karena tidak ada pesan error, cuma perilaku yang tidak sesuai harapan).

## Mencari simbol konfigurasi

Cara paling cepat mencari nama simbol Kconfig yang tepat adalah lewat command line, bukan buka-buka dokumentasi satu per satu:

```bash
west build -t guiconfig
```

atau versi teks:

```bash
west build -t menuconfig
```

Kedua perintah ini harus dijalankan setelah build directory ada (minimal sudah pernah `west build` sekali), karena dia butuh hasil konfigurasi CMake yang sudah di-generate.

Cara lain, cari langsung di source tree Zephyr:

```bash
grep -rn "config GPIO_ESP32" ~/zephyrproject/zephyr/ --include="Kconfig*"
```

Atau, kalau sudah ada build directory, semua simbol final yang aktif ada di `build/zephyr/.config`, bisa di-grep langsung:

```bash
grep GPIO build/zephyr/.config
```

Saya pribadi lebih sering pakai `menuconfig` untuk eksplorasi awal — bisa cari dengan tombol `/` lalu ketik keyword, dan langsung kelihatan help text serta dependency-nya. Setelah tahu nama simbolnya, baru saya tulis manual di `prj.conf` supaya tidak perlu buka menuconfig lagi tiap build.

![Tampilan menuconfig di terminal](images/03-menuconfig.png)
<!-- TODO(author): tambahkan screenshot tampilan west build -t menuconfig di terminal -->

## prj.conf vs overlay conf

`prj.conf` berlaku untuk semua build aplikasi tanpa syarat. Kadang kita mau konfigurasi berbeda untuk board tertentu saja, atau untuk build type tertentu (misalnya konfigurasi debug vs release). Untuk itu ada file overlay conf, formatnya `<nama>.conf` yang ditambahkan lewat opsi `-DEXTRA_CONF_FILE`, atau otomatis dipakai kalau namanya cocok pola board-specific: `boards/<board>.conf`.

Contoh, kalau ingin konfigurasi khusus yang cuma aktif untuk board esp32s3_devkitc, buat file:

```
aplikasi_saya/
├── boards/
│   └── esp32s3_devkitc.conf
├── CMakeLists.txt
├── prj.conf
└── src/main.c
```

Zephyr otomatis mendeteksi dan menggabungkan file ini kalau nama filenya cocok dengan board target build, tanpa perlu opsi tambahan apa pun di command line. Ini beda dengan overlay devicetree (`.overlay`) yang dibahas di [Bagian 5](../05-devicetree-dari-nol/README.md) — sama-sama disebut "overlay" tapi mekanisme dan tujuannya beda: overlay conf soal Kconfig (fitur software apa yang aktif), overlay devicetree soal deskripsi hardware.

## Latihan — Kconfig kustom

Kode latihan ada di [src/](src/). Aplikasi ini mendefinisikan simbol Kconfig baru bernama `CONFIG_LOG_INTERVAL_MS` lewat file `Kconfig` di root aplikasi, lalu memakainya di `main.c` untuk mengatur interval logging periodik. Nilainya bisa diubah dari `prj.conf` tanpa mengubah kode C sama sekali — ini simulasi kecil dari pola yang sebenarnya dipakai di banyak driver Zephyr asli: parameter yang sifatnya "pengaturan build-time" ditaruh di Kconfig, bukan di-hardcode atau dibaca runtime dari NVS.

File `Kconfig` di aplikasi (bukan `prj.conf`) yang mendefinisikan simbol baru:

```
menu "Konfigurasi Aplikasi Logging"

config LOG_INTERVAL_MS
	int "Interval logging dalam milidetik"
	default 1000
	help
	  Interval antara satu baris log periodik dengan berikutnya.

endmenu

source "Kconfig.zephyr"
```

Baris `source "Kconfig.zephyr"` di akhir wajib ada — ini yang menyambungkan Kconfig aplikasi ke seluruh pohon Kconfig Zephyr (tanpa ini, simbol standar seperti `CONFIG_GPIO` atau `CONFIG_LOG` tidak akan dikenali sebagai valid).

Build dan coba ubah nilainya lewat `prj.conf` (`CONFIG_LOG_INTERVAL_MS=500`) tanpa sentuh `main.c`, lalu build ulang dan lihat bedanya di serial monitor.

```bash
cd 03-struktur-dan-konfigurasi-aplikasi
west build -p always -b esp32s3_devkitc/esp32s3/procpu src
west flash
west espressif monitor
```

## Sumber

- [Zephyr Application Development](https://docs.zephyrproject.org/latest/develop/application/index.html)
- [Kconfig - Zephyr](https://docs.zephyrproject.org/latest/build/kconfig/index.html)
- [Setting Configuration Values](https://docs.zephyrproject.org/latest/build/kconfig/setting.html)
