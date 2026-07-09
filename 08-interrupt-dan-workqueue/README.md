# Bagian 8 — Interrupt dan Workqueue

Polling tombol di [Bagian 7](../07-aplikasi-blinky-board-kustom/README.md) punya masalah nyata: kalau loop utama sedang sibuk (misalnya sedang delay lama atau memproses sesuatu), penekanan tombol yang singkat bisa terlewat. Solusinya interrupt — CPU langsung dipanggil begitu ada perubahan level sinyal di pin, tidak perlu menunggu giliran polling.

Kembali pakai board `esp32s3_devkitc` untuk bagian ini supaya fokus ke konsep interrupt dan workqueue, bukan devicetree board kustom lagi.

## GPIO interrupt di Zephyr

Alur dasarnya: konfigurasi pin sebagai input, aktifkan interrupt dengan trigger tertentu (edge naik, edge turun, atau level), daftarkan fungsi callback lewat `gpio_init_callback` dan `gpio_add_callback`. Callback ini yang dipanggil kernel setiap kali interrupt terjadi — dan callback ini berjalan **di dalam konteks ISR**, bukan di thread biasa.

```c
gpio_pin_interrupt_configure_dt(&tombol, GPIO_INT_EDGE_TO_ACTIVE);
gpio_init_callback(&tombol_cb_data, tombol_isr, BIT(tombol.pin));
gpio_add_callback(tombol.port, &tombol_cb_data);
```

## Kenapa tidak boleh kerja berat di ISR

ISR (Interrupt Service Routine) berjalan dengan interrupt lain dalam prioritas sama atau lebih rendah tertunda (tergantung arsitektur dan level interrupt). Kalau ISR melakukan sesuatu yang lama — delay, akses I2C yang blocking, alokasi memori, apalagi print lewat UART yang bisa blocking kalau buffer penuh — seluruh sistem bisa macet menunggu interrupt itu selesai, atau lebih parah, interrupt lain yang lebih penting (misalnya timer sistem) jadi telat diproses. Aturan praktis: ISR cuma boleh melakukan operasi yang sangat cepat dan non-blocking, idealnya cuma "catat bahwa ini terjadi" lalu serahkan pekerjaan sebenarnya ke context lain.

Context lain itu bisa thread biasa yang menunggu semaphore, atau — yang lebih umum dipakai di Zephyr untuk kasus seperti ini — **workqueue**.

## System workqueue vs workqueue kustom

Workqueue adalah thread khusus yang menjalankan item kerja (`k_work`) yang di-submit dari tempat lain, termasuk dari ISR. Zephyr sudah punya satu system workqueue bawaan yang jalan otomatis begitu kernel start, cukup dipakai lewat `k_work_submit()` tanpa perlu setup tambahan.

Kapan butuh workqueue kustom sendiri (`k_work_queue_start` dengan stack dan prioritas sendiri)? Kalau item kerja Anda berpotensi memblokir lama atau prioritasnya perlu diatur terpisah dari pekerjaan lain yang juga memakai system workqueue — karena semua k_work yang di-submit ke system workqueue berjalan berurutan di satu thread yang sama, satu item yang lama akan menunda semua item lain yang antre di belakangnya. Untuk latihan ini, system workqueue saja sudah cukup karena beban kerjanya ringan (blink LED beberapa kali).

## k_work dan k_work_delayable

`k_work` adalah unit kerja paling dasar: definisikan dengan `K_WORK_DEFINE`, isi handler-nya, submit dengan `k_work_submit()`. Sekali di-submit, dia akan dijalankan workqueue secepat workqueue itu available, tidak ada delay.

`k_work_delayable` adalah variannya yang bisa dijadwalkan setelah jeda waktu tertentu, didefinisikan dengan `K_WORK_DELAYABLE_DEFINE`, dan disubmit lewat `k_work_schedule()` atau `k_work_reschedule()`. Bedanya dua fungsi ini penting untuk debouncing: `k_work_schedule` tidak melakukan apa-apa kalau item itu sudah terjadwal sebelumnya, sementara `k_work_reschedule` selalu menggeser ulang waktu tunggu meski item itu sudah terjadwal. Untuk debounce, kita pakai `k_work_reschedule` — setiap kali ada edge baru (termasuk bouncing), waktu tunggu digeser ulang dari nol, sehingga handler sebenarnya baru jalan kalau sinyal sudah diam (settle) selama periode tertentu tanpa edge baru.

## Latihan

Kode di [src/](src/) mengimplementasikan pola berikut: tombol (alias `sw0`, harus ditambahkan lewat overlay sama seperti Bagian 5) memicu interrupt pada edge aktif. ISR (`tombol_isr`) tidak melakukan apa pun selain memanggil `k_work_reschedule()` pada delayable work debounce dengan jeda 50 ms. Kalau tombol bouncing (menghasilkan beberapa edge listrik dalam hitungan milidetik, umum terjadi pada saklar mekanik), setiap edge menggeser ulang jadwal, sehingga handler debounce betulan cuma jalan sekali setelah sinyal benar-benar settle.

Begitu handler debounce jalan (artinya penekanan valid terdeteksi), dia submit `k_work` biasa (`burst_kedip_work`) ke system workqueue, yang menjalankan burst kedip LED sebanyak `jumlah_kedip` kali — angka ini bertambah satu setiap penekanan valid, jadi tekan pertama menghasilkan 1 kedipan, tekan kedua 2 kedipan, dan seterusnya.

```bash
cd 08-interrupt-dan-workqueue
west build -p always -b esp32s3_devkitc/esp32s3/procpu src
west flash
west espressif monitor
```

Wiring sama seperti Bagian 4/5: LED di GPIO4, tombol di GPIO7 dengan internal pull-up, overlay-nya ada di [src/app.overlay](src/app.overlay).

![Serial monitor menunjukkan burst kedip bertambah tiap tekan](images/08-burst-kedip-serial.png)
<!-- TODO(author): tambahkan screenshot serial monitor menunjukkan log jumlah burst kedip bertambah tiap penekanan tombol -->

Coba juga sengaja tekan tombol berkali-kali sangat cepat (spam) untuk melihat efek debounce — harusnya cuma terhitung sebagai penekanan-penekanan terpisah kalau jedanya lebih dari 50 ms, bukan satu tekan panjang dihitung berkali-kali karena bouncing kontak mekanik.

## Sumber

- [Zephyr GPIO Interrupt](https://docs.zephyrproject.org/latest/hardware/peripherals/gpio.html)
- [Zephyr Workqueue Documentation](https://docs.zephyrproject.org/latest/kernel/services/threads/workqueue.html)
- [Zephyr Kernel Services - Work Queues API](https://docs.zephyrproject.org/latest/doxygen/html/group__workqueue__apis.html)
