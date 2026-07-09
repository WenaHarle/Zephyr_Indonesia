# Bagian 9 — Proyek Akhir: Data Logger Sensor

Proyek akhir yang umum dipakai di tutorial Zephyr adalah brick breaker atau game sederhana lain yang menunjukkan grafis di layar. Saya ganti dengan sesuatu yang menurut saya jauh lebih relevan untuk konteks ESP32-S3 dan menggabungkan semua yang sudah dipelajari di bagian sebelumnya: data logger sensor dengan status LED dan shell interaktif. Ini pola yang benar-benar dipakai di banyak proyek embedded nyata — baca sensor secara berkala, olah di background, kasih kontrol lewat command line untuk debugging atau operasional.

Cakupannya: baca sensor I2C, threading (thread sensor terpisah dari thread logger), message queue sebagai jembatan antar thread, Zephyr shell untuk kontrol interaktif (start/stop/status), dan logging subsystem menggantikan `printk` mentah. Dipecah jadi empat tahap, tiap tahap bisa di-build dan diuji sendiri sebelum lanjut ke tahap berikutnya — jangan loncat ke tahap 4 kalau tahap 1 saja belum pernah berhasil baca sensor.

## Sensor yang dipakai

Saya pakai MPU6050 (accelerometer + gyroscope 6-axis) sebagai contoh karena sudah ada driver bawaan Zephyr (`invensense,mpu6050`) dan modulnya murah serta gampang didapat. Kalau Anda punya sensor lain (BME280, AHT20, apa saja yang ada driver Zephyr-nya), pola kodenya sama persis — cuma ganti compatible string di overlay dan channel yang di-fetch di `sensor_channel_get`. Cek daftar driver sensor yang didukung di `~/zephyrproject/zephyr/drivers/sensor/` kalau sensor Anda beda.

Wiring I2C: SDA ke GPIO8, SCL ke GPIO9 (pin I2C default yang saya pakai konsisten di semua tahap), VCC ke 3.3V, GND ke GND. MPU6050 alamat I2C default `0x68` (atau `0x69` kalau pin AD0 ditarik high — sesuaikan `reg` di overlay kalau modul Anda berbeda).

![Wiring MPU6050 ke ESP32-S3 lewat I2C](images/09-wiring-mpu6050.png)
<!-- TODO(author): tambahkan foto wiring MPU6050 ke ESP32-S3-DevKitC-1, tunjukkan SDA/SCL/VCC/GND -->

## Tahap 1 — baca sensor

[src/tahap-1-baca-sensor/](src/tahap-1-baca-sensor/) adalah versi paling sederhana: satu thread (`main`), baca sensor tiap satu detik, print lewat `printk`. Tujuannya murni membuktikan wiring I2C dan driver sensor bekerja sebelum menambah kompleksitas apa pun.

```bash
cd 09-proyek-akhir
west build -p always -b esp32s3_devkitc/esp32s3/procpu src/tahap-1-baca-sensor
west flash
west espressif monitor
```

Kalau tahap ini gagal dengan sensor tidak terdeteksi (`device_is_ready` false atau `sensor_sample_fetch` selalu error), cek dulu wiring dan alamat I2C sebelum lanjut — jangan coba debug threading atau shell kalau fondasinya saja belum jalan. Alat bantu yang berguna: scan I2C bus dengan `i2c_detect` lewat shell (perlu `CONFIG_I2C_SHELL=y`, kombinasi dengan tahap 3 nanti) untuk memastikan alamat device benar-benar terdeteksi di bus.

## Tahap 2 — threading dan message queue

[src/tahap-2-threading-msgq/](src/tahap-2-threading-msgq/) memecah kerja jadi dua thread: `sensor_thread` yang baca sensor tiap detik dan push hasilnya ke `k_msgq`, dan `logger_thread` yang nunggu data di message queue lalu print. Pola ini disebut producer-consumer, dan alasannya nyata: kalau nanti logger perlu menulis ke storage lambat (SD card, flash) atau kirim lewat jaringan, itu tidak akan memblokir pembacaan sensor berikutnya karena keduanya berjalan independen, cuma bertukar data lewat antrian.

Perhatikan `sensor_thread_fn` memakai `k_msgq_put` dengan `K_NO_WAIT` — kalau antrian penuh (logger lebih lambat dari sensor), sample terlama dibuang daripada thread sensor ikut terblokir menunggu ruang kosong. Trade-off ini masuk akal untuk data logger: lebih baik kehilangan satu sample lama daripada pembacaan sensor jadi tersendat.

```bash
west build -p always -b esp32s3_devkitc/esp32s3/procpu src/tahap-2-threading-msgq
west flash
west espressif monitor
```

## Tahap 3 — shell interaktif

[src/tahap-3-shell-interaktif/](src/tahap-3-shell-interaktif/) menambah kontrol lewat Zephyr shell subsystem: perintah `datalogger start`, `datalogger stop`, `datalogger status`. Logging tidak otomatis jalan begitu boot — harus di-start manual lewat shell, ini simulasi kecil dari operasional nyata di mana Anda mau kontrol kapan device mulai mencatat data.

```bash
west build -p always -b esp32s3_devkitc/esp32s3/procpu src/tahap-3-shell-interaktif
west flash
west espressif monitor
```

Setelah serial monitor terbuka, ketik langsung di terminal itu:

```
uart:~$ datalogger start
Logging diaktifkan
uart:~$ datalogger status
Status  : aktif
Total   : 5 pembacaan tercatat
uart:~$ datalogger stop
Logging dihentikan
```

Tekan tombol Tab untuk autocomplete perintah, ini salah satu enaknya Zephyr shell dibanding sekadar parsing string manual dari UART.

![Shell interaktif menjalankan perintah datalogger](images/09-shell-datalogger.png)
<!-- TODO(author): tambahkan screenshot serial monitor menampilkan perintah datalogger start/status/stop -->

## Tahap 4 — logging subsystem lengkap

[src/tahap-4-logging-lengkap/](src/tahap-4-logging-lengkap/) adalah versi final yang menggabungkan semua tahap sebelumnya, ditambah dua hal: `printk` diganti dengan Zephyr logging subsystem (`LOG_INF`, `LOG_WRN`, `LOG_ERR` lewat modul bernama `datalogger`), dan status LED yang mencerminkan kondisi logging — mati saat idle, menyala solid saat logging aktif, dan log level error kalau pembacaan sensor gagal.

Beda `printk` dengan logging subsystem bukan cuma kosmetik: logging subsystem punya level (`DBG`, `INF`, `WRN`, `ERR`), bisa diatur per-modul saat runtime lewat shell (`log enable/disable`), dan bisa dikonfigurasi buffer asinkron supaya proses logging sendiri tidak memperlambat thread yang memanggilnya — cocok kalau nanti volume log tinggi.

```bash
west build -p always -b esp32s3_devkitc/esp32s3/procpu src/tahap-4-logging-lengkap
west flash
west espressif monitor
```

Coba juga perintah shell bawaan untuk kontrol log level saat runtime:

```
uart:~$ log enable dbg datalogger
uart:~$ log disable datalogger
```

## Latihan tambahan opsional

Dua arah pengembangan lanjut yang tidak saya buatkan kode lengkapnya di sini (di luar cakupan minimum proyek ini, tapi layak dicoba sendiri):

**Kirim data lewat UART kedua.** ESP32-S3 punya beberapa UART hardware. Tambahkan node `uart1` di overlay, aktifkan dengan pinctrl pin yang belum dipakai, lalu di `logger_thread` selain print ke console, kirim juga format data (CSV atau JSON sederhana) ke UART itu. Berguna kalau mau device lain (Raspberry Pi, komputer kedua) menerima stream data tanpa mengganggu console debug utama.

**Simpan ke flash dengan NVS.** Non-Volatile Storage (NVS) adalah key-value store ringan bawaan Zephyr untuk flash internal. Ada contoh minimal di [src/opsional-nvs/](src/opsional-nvs/) yang menyimpan pembacaan sensor terakhir ke NVS supaya bisa dibaca lagi setelah reboot — berguna untuk device yang perlu ingat state terakhir sebelum mati listrik tiba-tiba. Jangan pakai NVS untuk data logging volume tinggi (tiap detik terus-menerus) karena flash punya batas siklus tulis-hapus; NVS lebih cocok untuk konfigurasi atau checkpoint periodik, bukan pengganti database time-series.

```bash
west build -p always -b esp32s3_devkitc/esp32s3/procpu src/opsional-nvs
west flash
west espressif monitor
```

## Sumber

- [Zephyr Sensor API](https://docs.zephyrproject.org/latest/hardware/peripherals/sensor/index.html)
- [Zephyr Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html)
- [Zephyr Message Queue](https://docs.zephyrproject.org/latest/kernel/services/data_passing/message_queues.html)
- [Zephyr Shell Subsystem](https://docs.zephyrproject.org/latest/services/shell/index.html)
- [Zephyr Logging Subsystem](https://docs.zephyrproject.org/latest/services/logging/index.html)
- [Zephyr NVS (Non-Volatile Storage)](https://docs.zephyrproject.org/latest/services/storage/nvs/nvs.html)
