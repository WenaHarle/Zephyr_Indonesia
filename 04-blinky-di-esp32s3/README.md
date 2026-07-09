# Bagian 4 — Blinky di ESP32-S3

Blinky adalah "hello world"-nya embedded, dan Zephyr punya sample bawaan `samples/basic/blinky` yang memakai devicetree alias `led0` supaya kode C-nya sama persis di semua board yang didukung. Masalahnya, ESP32-S3-DevKitC-1 tidak punya LED biasa yang terhubung ke satu pin GPIO — board ini memakai satu WS2812 addressable RGB LED (sering disebut NeoPixel) di GPIO48. Beda dari LED biasa yang cukup di-set high/low, WS2812 butuh protokol timing presisi mikrodetik untuk mengirim data warna, jadi tidak bisa langsung pakai `gpio_pin_toggle_dt` begitu saja.

Ada dua jalur yang saya bahas di sini: pakai WS2812 onboard lewat driver `led_strip`, atau pasang LED biasa eksternal di breadboard lewat GPIO umum. Opini saya: untuk belajar GPIO API dasar, LED eksternal jauh lebih enak — behaviornya predictable, tidak ada driver protokol tambahan yang perlu dipahami dulu, dan kalau salah wiring gejalanya jelas (LED tidak nyala) bukan error timing yang susah didiagnosis. WS2812 saya taruh sebagai variasi kedua karena tetap relevan (ini LED yang benar-benar ada di board tanpa perlu komponen tambahan).

## GPIO API dasar Zephyr

Pola standar kerja dengan GPIO di Zephyr selalu tiga langkah: ambil `struct gpio_dt_spec` dari devicetree, cek device siap dengan `gpio_is_ready_dt`, konfigurasi arah pin dengan `gpio_pin_configure_dt`, baru kontrol dengan `gpio_pin_set_dt` / `gpio_pin_toggle_dt`. Kuncinya adalah semua ini didorong dari devicetree lewat macro `GPIO_DT_SPEC_GET`, bukan angka pin hardcode di kode C. Kalau besok pindah board, yang berubah cuma devicetree/overlay, kode C-nya tidak disentuh sama sekali — ini prinsip yang akan lebih jelas terasa manfaatnya di [Bagian 7](../07-aplikasi-blinky-board-kustom/README.md).

## Jalur 1 — LED eksternal di GPIO biasa

Ini yang saya rekomendasikan dikerjakan lebih dulu. Rangkai LED dengan resistor 220-330 ohm ke GPIO4 (pin ini aman dipakai umum, bukan strapping pin — hindari GPIO0, GPIO3, GPIO45, GPIO46 untuk keperluan umum karena dipakai strapping boot mode dan bisa mengganggu proses boot kalau ditarik ke level yang salah saat reset).

```
GPIO4 ---[resistor 220-330 ohm]---[anoda LED]
                                   [katoda LED] --- GND
```

![Wiring LED eksternal ke GPIO4 di breadboard](images/04-wiring-led-eksternal.png)
<!-- TODO(author): tambahkan foto wiring LED eksternal ke GPIO4 di breadboard, tunjukkan orientasi resistor dan polaritas LED -->

Kode di [src/external-led/](src/external-led/) mendefinisikan node LED lewat overlay devicetree (`app.overlay`), memakai binding standar `gpio-leds` yang sudah tersedia di Zephyr, kemudian `main.c` membaca alias `led0` dan blink setiap 500 ms:

```bash
cd 04-blinky-di-esp32s3
west build -p always -b esp32s3_devkitc/esp32s3/procpu src/external-led
west flash
```

## Jalur 2 — WS2812 onboard

WS2812 di Zephyr dikontrol lewat subsistem `led_strip`, dengan driver backend yang beberapa macam (bit-bang GPIO, SPI, atau RMT — RMT yang paling umum dipakai di chip Espressif karena ada peripheral hardware khusus untuk generate timing presisi ini). Contoh resmi ada di `samples/drivers/led_ws2812` pada source tree Zephyr Anda, lengkap dengan overlay board `esp32s3_devkitc` yang sudah disiapkan tim Zephyr/Espressif.

Catatan penting: property dan compatible string node RMT ini termasuk yang paling sering berubah sintaksnya antar rilis Zephyr (nama macro pinmux, property clock, dll). Overlay di [src/ws2812-onboard/app.overlay](src/ws2812-onboard/app.overlay) saya tulis berdasar pola RMT yang berlaku saat tulisan ini dibuat — kalau build Anda gagal karena property tidak dikenali, jangan tebak-tebak, buka langsung `samples/drivers/led_ws2812/boards/esp32s3_devkitc.overlay` di dalam source tree Zephyr Anda sebagai referensi paling akurat untuk versi yang sedang Anda pakai, lalu samakan.

```bash
cd 04-blinky-di-esp32s3
west build -p always -b esp32s3_devkitc/esp32s3/procpu src/ws2812-onboard
west flash
```

Kode `main.c` memakai `led_strip_update_rgb()` untuk mengirim array `struct led_rgb` ke device, dengan jumlah piksel diambil dari property `chain-length` di devicetree lewat `DT_PROP`, bukan angka hardcode.

![WS2812 onboard menyala warna merah](images/04-ws2812-menyala.png)
<!-- TODO(author): tambahkan foto WS2812 onboard board menyala, tunjukkan warna yang dipakai di kode -->

## Latihan — pola SOS morse

Kode di [src/sos-morse/](src/sos-morse/) memakai LED eksternal dari jalur 1 (overlay sama persis, tinggal dicopy), tapi pola blink-nya bukan sekadar on-off berkala — ini kode morse SOS: tiga kedip pendek (titik), tiga kedip panjang (garis), tiga kedip pendek lagi, lalu jeda panjang sebelum mengulang.

```bash
cd 04-blinky-di-esp32s3
west build -p always -b esp32s3_devkitc/esp32s3/procpu src/sos-morse
west flash
```

Timing yang saya pakai: titik 200 ms nyala, garis 600 ms nyala, jeda antar simbol dalam satu huruf 200 ms, jeda antar huruf 600 ms, jeda antar pengulangan kata 1400 ms. Angka-angka ini konvensi umum morse (rasio 1:3 antara titik dan garis), bukan aturan baku yang harus persis — sesuaikan saja kalau mau lebih cepat atau lebih pelan dilihat mata.

## Sumber

- [Zephyr Blinky Sample](https://docs.zephyrproject.org/latest/samples/basic/blinky/README.html)
- [GPIO API Reference](https://docs.zephyrproject.org/latest/hardware/peripherals/gpio.html)
- [LED Strip API Reference](https://docs.zephyrproject.org/latest/hardware/peripherals/led_strip.html)
- [ESP32-S3-DevKitC-1 Datasheet (Espressif)](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide.html)
