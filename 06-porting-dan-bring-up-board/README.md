# Bagian 6 — Porting dan Bring-up Board Kustom

Ini bagian paling berat di seluruh repo ini, dan saya sengaja alokasikan waktu paling banyak untuknya. Sampai sekarang kita selalu pakai board `esp32s3_devkitc` yang sudah didukung penuh oleh Zephyr. Sekarang kita bikin definisi board sendiri, dari luar source tree Zephyr, berbasis modul ESP32-S3-WROOM telanjang (bukan dev board DevKitC-1 yang sudah ada USB-to-serial bridge dan tombol BOOT/RESET bawaan).

Kenapa ini penting dipelajari: di dunia nyata, produk akhir hampir tidak pernah memakai dev board langsung. Anda akan punya PCB sendiri dengan pin mapping sendiri, mungkin tanpa sebagian peripheral yang ada di dev board. Proses "board porting" inilah yang menjembatani dari referensi Espressif ke hardware produk sungguhan.

## Board definition di luar tree Zephyr

Sejak Zephyr mengadopsi hardware model v2 (HWMv2), board custom tidak perlu ditambahkan ke dalam source tree Zephyr sama sekali. Cukup taruh di folder `boards/` di dalam aplikasi Anda sendiri, dan west akan otomatis menemukannya lewat `BOARD_ROOT`:

```
aplikasi_saya/
├── boards/
│   └── others/
│       └── zi_wroom_s3/
│           ├── board.yml
│           ├── board.cmake
│           ├── Kconfig.zi_wroom_s3
│           ├── Kconfig.defconfig
│           ├── zi_wroom_s3_esp32s3_procpu.dts
│           └── zi_wroom_s3_esp32s3_procpu_defconfig
├── CMakeLists.txt
├── prj.conf
└── src/main.c
```

Path `boards/others/<nama-board>/` ini bukan asal — `others` adalah nilai `vendor` yang dipakai kalau board Anda bukan produk resmi vendor yang sudah terdaftar di `dts/bindings/vendor-prefixes.txt` milik Zephyr. Kalau board Anda memang berbasis produk vendor resmi (misalnya Anda kerja di perusahaan yang sudah terdaftar), pakai nama vendor itu; untuk proyek pribadi, `others` adalah pilihan yang tepat dan didukung resmi.

Struktur ini dipakai apa adanya lewat opsi `-DBOARD_ROOT`, atau lebih praktis taruh di `boards/` tepat di root aplikasi karena west otomatis mencarinya di situ tanpa opsi tambahan:

```bash
west build -p always -b zi_wroom_s3/esp32s3/procpu -- -DBOARD_ROOT=$(pwd)/boards
```

## File-file wajib

**board.yml** — metadata board: nama, vendor, dan daftar SoC beserta cpucluster yang didukung. ESP32-S3 punya dua core (procpu dan appcpu), untuk board custom yang cuma dipakai single-core biasanya cukup daftarkan keduanya supaya qualifier lengkap tersedia meski appcpu tidak dipakai.

```yaml
board:
  name: zi_wroom_s3
  vendor: others
  socs:
    - name: esp32s3
      cpuclusters:
        - name: procpu
        - name: appcpu
```

**Kconfig.\<nama-board\>** — mendeklarasikan symbol Kconfig boolean untuk board ini. Isinya minimal, karena logic pemilihan SoC/cpucluster sudah ditangani infrastruktur build berdasarkan `board.yml`.

**Kconfig.defconfig** — nilai default Kconfig yang berlaku khusus untuk board ini, biasanya minimal berisi default nama board string.

**\<nama-board\>_\<soc\>_\<cpucluster\>.dts** — devicetree utama board ini untuk kombinasi soc+cpucluster tertentu (jadi ada satu file per core kalau mendukung keduanya). Isinya include devicetree SoC ESP32-S3 yang sudah disediakan Zephyr, ditambah node board-specific: UART console, GPIO yang dipakai, pinctrl.

**\<nama-board\>_\<soc\>_\<cpucluster\>_defconfig** — fragmen Kconfig format `.config` (bukan syntax Kconfig biasa) berisi symbol yang harus aktif secara default untuk board ini, misalnya driver serial dan GPIO.

**board.cmake** — menentukan west runner (flash/debug tool) default untuk board ini. Untuk ESP32-S3 biasanya cukup include modul runner esp32 bawaan Zephyr.

Cara paling aman menulis semua file ini bukan dari nol murni, tapi menyalin dari board serupa yang sudah ada di source tree Zephyr (`~/zephyrproject/zephyr/boards/espressif/esp32s3_devkitc/`) sebagai starting point, lalu rename dan sesuaikan pin mapping-nya. Ini juga saran resmi dari dokumentasi Zephyr sendiri — jangan menulis board port dari nol kalau ada board dengan SoC sama yang bisa dijadikan template. Sebelum menulis file di [boards/others/zi_wroom_s3/](boards/others/zi_wroom_s3/) di repo ini, saya melakukan persis itu: `diff` file devkitc dengan yang saya tulis untuk memastikan struktur include devicetree SoC-nya konsisten.

```bash
find ~/zephyrproject/zephyr/dts -iname "esp32s3*"
```

Jalankan perintah itu untuk melihat file dtsi SoC ESP32-S3 yang tersedia di instalasi Anda — nama file varian modul (N8, N8R8, N16R8, dst, tergantung ukuran flash/PSRAM) bisa berbeda antar rilis Zephyr, jadi sesuaikan include path di dts board kustom Anda dengan yang benar-benar ada di direktori itu.

## Perbedaan hardware: WROOM telanjang vs DevKitC-1

DevKitC-1 sudah punya USB-to-UART bridge (chip CP2102N) yang tersambung ke UART0, plus sirkuit auto-reset yang mengontrol pin EN dan BOOT lewat sinyal DTR/RTS dari USB. Modul WROOM telanjang tidak punya semua ini — Anda harus:

- Sambungkan USB-to-serial converter eksternal (FTDI atau CP2102 board terpisah) ke UART0: GPIO43 (TXD0) dan GPIO44 (RXD0) pada chip ESP32-S3.
- Sediakan tombol manual ke pin EN (reset) dan GPIO0 (boot mode select) kalau mau masuk mode download tanpa auto-reset. Untuk masuk mode flashing manual: tahan GPIO0 ke GND, tekan-lepas EN sebentar (reset), baru lepas GPIO0.
- Beri pull-up ke EN (biasanya sudah ada resistor pull-up bawaan di modul WROOM, tapi cek datasheet modul spesifik Anda).

Console yang dipakai board kustom ini karenanya adalah UART0 biasa (`&uart0` di devicetree), bukan `usb_serial_jtag` seperti default DevKitC-1 yang punya port USB native langsung ke chip.

![Wiring modul WROOM ke USB-to-serial eksternal](images/06-wiring-wroom-external-serial.png)
<!-- TODO(author): tambahkan foto wiring modul ESP32-S3-WROOM ke USB-to-serial converter eksternal, tunjukkan pin EN, GPIO0, TX, RX -->

## Bring-up bertahap

Jangan langsung coba semua peripheral sekaligus. Urutan yang saya pakai:

**Tahap 1 — pastikan boot.** Build aplikasi paling minimal (bisa `samples/hello_world`) dengan target board baru ini. Kalau build sukses tapi tidak ada tanda kehidupan sama sekali di serial, biasanya masalah wiring power atau EN, bukan masalah software. Cek dulu dengan multimeter apakah 3.3V sampai ke modul.

**Tahap 2 — pastikan console UART.** Ini tahap paling penting karena semua debugging selanjutnya bergantung padanya. Kalau `zephyr,console` di chosen node tidak benar atau pinctrl UART0 salah pin, Anda tidak akan lihat apa pun meski board sebenarnya sudah boot dengan benar. Uji dengan print sederhana dulu sebelum menambah node lain.

**Tahap 3 — baru peripheral lain.** Setelah console jalan, tambah satu per satu: GPIO untuk LED (mudah diverifikasi visual), lalu I2C/SPI kalau dibutuhkan. Tambah satu, build, uji, baru lanjut ke berikutnya — jangan tambah lima node sekaligus lalu bingung yang mana yang salah kalau gagal.

## Latihan

File board kustom lengkap ada di [boards/others/zi_wroom_s3/](boards/others/zi_wroom_s3/). Saya beri nama `zi_wroom_s3` (singkatan proyek pribadi saya), silakan ganti nama sesuai proyek Anda sendiri — cukup rename folder dan semua prefix nama file, karena nama board harus konsisten antara nama folder, nama file, dan isi `board.yml`.

Pin mapping yang saya pakai beda dari DevKitC-1: LED bring-up di GPIO9 (DevKitC-1 WS2812 ada di GPIO48, sengaja saya pilih beda untuk menegaskan bahwa board baru ini punya devicetree sendiri, bukan warisan devkitc).

Verifikasi board baru dikenali west:

```bash
west boards | grep zi_wroom_s3
```

Build aplikasi hello world minimal ke board baru:

```bash
cd 06-porting-dan-bring-up-board
west build -p always -b zi_wroom_s3/esp32s3/procpu boards/others/zi_wroom_s3/test-app -- -DBOARD_ROOT=$(pwd)/boards
west flash --esp-device /dev/ttyUSB0
```

Device flashing di sini kemungkinan besar `/dev/ttyUSB0` (converter eksternal CP2102/FTDI), bukan `/dev/ttyACM0` seperti DevKitC-1, karena tidak ada native USB CDC dari modul WROOM langsung.

## Sumber

- [Zephyr Board Porting Guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html)
- [Zephyr Hardware Model v2](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html#create-your-board-directory)
- [ESP32-S3 Technical Reference Manual (Espressif)](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [ESP32-S3-WROOM-1 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
