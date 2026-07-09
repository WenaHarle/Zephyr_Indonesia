# Bagian 7 — Aplikasi Blinky untuk Board Kustom

Board `zi_wroom_s3` dari [Bagian 6](../06-porting-dan-bring-up-board/README.md) sudah bisa boot dan print lewat console. Sekarang tulis aplikasi yang benar-benar memanfaatkan devicetree board itu, dengan aturan ketat: tidak ada satu pun nomor pin GPIO yang di-hardcode di kode C. Semua datang dari alias devicetree. Kalau besok pin mapping berubah (revisi PCB baru misalnya), yang diedit cuma file devicetree, `main.c` tidak disentuh sama sekali.

Ini prinsip yang sama seperti di [Bagian 4](../04-blinky-di-esp32s3/README.md), tapi sekarang dipraktikkan di board yang benar-benar custom, bukan board yang sudah disiapkan Zephyr.

## Menambah LED dan tombol lewat overlay aplikasi

Board `zi_wroom_s3` di Bagian 6 baru punya satu LED bring-up (`led0` di GPIO9). Untuk latihan ini saya tambah dua LED lagi dan satu tombol lewat overlay di level aplikasi ([src/app.overlay](src/app.overlay)), tanpa mengedit file board sama sekali — ini menunjukkan devicetree bisa ditumpuk berlapis: board dts sebagai basis, overlay aplikasi menambah atau menimpa node sesuai kebutuhan proyek spesifik.

```
GPIO9  -> LED 0 (sudah ada dari board dts, kecepatan dasar)
GPIO10 -> LED 1 (ditambah lewat overlay aplikasi)
GPIO11 -> LED 2 (ditambah lewat overlay aplikasi)
GPIO12 -> tombol (ditambah lewat overlay aplikasi, internal pull-up)
```

![Wiring multi-LED dan tombol di board kustom](images/07-wiring-multi-led-tombol.png)
<!-- TODO(author): tambahkan foto wiring tiga LED dan satu tombol pada modul WROOM custom board -->

## Aturan: tidak hardcode pin

Semua akses GPIO di [src/main.c](src/main.c) memakai `GPIO_DT_SPEC_GET` dari alias `led0`, `led1`, `led2`, dan `sw0`. Tidak ada angka GPIO literal manapun di file itu. Cara memverifikasi diri sendiri tidak melanggar aturan ini gampang: coba grep angka di file C-nya.

```bash
grep -n "GPIO_NUM\|gpio0, [0-9]" src/main.c
```

Kalau hasilnya kosong (selain di komentar), berarti bersih.

## Polling tombol

Untuk latihan ini tombol dibaca dengan polling (cek status pin tiap iterasi loop), belum pakai interrupt. Ini bukan cara yang direkomendasikan untuk kode produksi — polling boros CPU dan bisa melewatkan penekanan singkat kalau loop utama sedang sibuk di tempat lain — tapi cukup buat memahami `gpio_pin_get_dt` dulu sebelum lompat ke interrupt di [Bagian 8](../08-interrupt-dan-workqueue/README.md). Jangan loncat ke interrupt duluan kalau dasar polling belum kebayang, nanti susah membedakan bug logic dengan bug konkurensi.

Logika aplikasi: tiga LED berkedip bergantian (efek berjalan/chaser), dan setiap tombol ditekan, kecepatan kedipnya berubah lewat beberapa tingkatan (lambat - sedang - cepat - lambat lagi, siklus).

```bash
cd 07-aplikasi-blinky-board-kustom
west build -p always -b zi_wroom_s3/esp32s3/procpu src -- -DBOARD_ROOT=$(pwd)/../06-porting-dan-bring-up-board/boards
west flash --esp-device /dev/ttyUSB0
```

Opsi `-DBOARD_ROOT` di sini menunjuk ke folder `boards/` di Bagian 6 karena board kustom ini didefinisikan di sana, bukan diduplikasi di tiap bagian. Kalau Anda menyalin folder ini untuk proyek sendiri, cukup salin juga folder `boards/` ke lokasi yang sesuai dan sesuaikan path-nya.

## Sumber

- [Zephyr Devicetree Overlays](https://docs.zephyrproject.org/latest/build/dts/howtos.html#set-devicetree-overlays)
- [GPIO API Reference](https://docs.zephyrproject.org/latest/hardware/peripherals/gpio.html)
- [Zephyr Button/Switch Sample](https://docs.zephyrproject.org/latest/samples/basic/button/README.html)
