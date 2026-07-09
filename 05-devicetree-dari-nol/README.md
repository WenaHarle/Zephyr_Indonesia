# Bagian 5 — Devicetree dari Nol

Sampai bagian ini saya sudah beberapa kali menulis file `.overlay` tanpa menjelaskan detail sintaksnya, cuma modal contoh yang jalan. Sekarang waktunya berhenti dan benar-benar paham apa itu devicetree, karena hampir semua yang unik dari Zephyr dibanding RTOS lain terletak di sini — driver, pin mapping, dan konfigurasi hardware semuanya dideskripsikan lewat devicetree, bukan ditulis manual di kode C.

## Kenapa devicetree

Devicetree awalnya dari dunia Linux kernel, dipakai untuk mendeskripsikan hardware yang tidak bisa di-probe otomatis (beda dengan PCI/USB yang device-nya bisa query sendiri). Board embedded penuh dengan hardware semacam ini: GPIO mana yang terhubung ke LED mana, I2C address sensor, berapa banyak UART yang ada. Daripada semua ini ditulis sebagai `#define` tersebar di header file per board (yang jadi mimpi buruk kalau portingnya lintas banyak board), devicetree memisahkan deskripsi hardware dari kode driver. Driver GPIO ditulis sekali secara generik, lalu devicetree bilang "pin 4 dipakai sebagai LED, pin 5 dipakai sebagai tombol" — driver baca itu lewat macro `DT_*` saat compile time.

## Sintaks dasar

File devicetree Zephyr ada tiga jenis ekstensi:

- `.dts` — root devicetree source untuk satu board, biasanya include beberapa `.dtsi`
- `.dtsi` — devicetree source yang di-include, biasanya deskripsi SoC (isinya node-node peripheral yang sama untuk semua board dengan chip itu)
- `.overlay` — devicetree yang "ditumpuk" di atas board dts yang sudah ada, dipakai aplikasi untuk menambah atau mengubah node tanpa mengedit file board asli

Struktur dasarnya berbentuk pohon node:

```dts
/ {
	soc {
		gpio0: gpio@60004000 {
			compatible = "espressif,esp32-gpio";
			#gpio-cells = <2>;
			status = "okay";
		};
	};

	aliases {
		led0 = &myled;
	};

	leds {
		compatible = "gpio-leds";
		myled: led_0 {
			gpios = <&gpio0 4 GPIO_ACTIVE_HIGH>;
			label = "LED Saya";
		};
	};
};
```

Beberapa istilah kunci:

**Node** adalah unit dasar, ditulis `nama-node@alamat { ... }` kalau punya alamat register (peripheral fisik), atau `nama-node { ... }` tanpa alamat kalau node logis (seperti kontainer `leds` di atas).

**Properti** adalah pasangan key-value di dalam node, seperti `status = "okay"` atau `gpios = <&gpio0 4 GPIO_ACTIVE_HIGH>`.

**Label** adalah nama yang ditaruh sebelum node dengan titik dua (`myled:`), dipakai untuk mereferensikan node itu dari tempat lain dengan syntax `&myled`. Label ini yang bikin devicetree bisa cross-reference antar file tanpa harus tahu path lengkap node-nya.

**Alias** adalah node khusus bernama `aliases` yang memberi nama pendek dan stabil ke sebuah node, biasanya dipakai supaya kode C bisa merujuk `DT_ALIAS(led0)` tanpa peduli node aslinya path-nya panjang apa.

**Chosen** mirip alias tapi untuk konfigurasi sistem level tinggi, misalnya `zephyr,console` menentukan UART mana yang dipakai sebagai console default.

```dts
/ {
	chosen {
		zephyr,console = &usb_serial;
		zephyr,shell-uart = &usb_serial;
	};
};
```

**Binding** adalah file YAML yang mendefinisikan skema sebuah `compatible` string — properti apa saja yang valid, mana yang wajib, tipe datanya apa. Binding untuk `gpio-leds` misalnya ada di `dts/bindings/led/gpio-leds.yaml` pada source tree Zephyr. Kalau bikin driver atau node custom, binding inilah yang harus ditulis supaya devicetree compiler tahu cara memvalidasi node itu.

## Membaca hasil build devicetree

Setelah `west build` selesai sekali, ada satu file yang sangat berguna untuk debugging: `build/zephyr/zephyr.dts`. Ini adalah hasil akhir devicetree setelah semua `.dts`, `.dtsi`, dan `.overlay` digabung dan di-resolve. Kalau bingung kenapa `DT_ALIAS` tidak ketemu atau node tidak muncul seperti yang diharapkan, buka file ini dulu sebelum menebak-nebak:

```bash
grep -A 10 "leds {" build/zephyr/zephyr.dts
```

Ada juga `build/zephyr/zephyr.dts.d` (dependency file, kurang berguna untuk dibaca manual) dan yang tidak kalah penting, `build/zephyr/include/generated/zephyr/devicetree_generated.h` — ini header hasil generate dari devicetree yang isinya semua macro `DT_N_*` mentah yang dipakai `DT_ALIAS`, `DT_PROP`, dst di baliknya. Biasanya tidak perlu dibaca langsung, tapi kalau macro `DT_` yang dipakai tidak sesuai ekspektasi, error compiler seringkali menunjuk ke sini.

## Latihan — tiga LED dan satu tombol

Overlay di [overlay/app.overlay](overlay/app.overlay) mendefinisikan tiga node LED eksternal (alias `led0`, `led1`, `led2`) dan satu node tombol (alias `sw0`), lengkap dengan binding `gpio-leds` dan `gpio-keys` yang sudah standar di Zephyr. Wiring yang saya pakai:

```
GPIO4  -> resistor -> LED merah   -> GND
GPIO5  -> resistor -> LED kuning  -> GND
GPIO6  -> resistor -> LED hijau   -> GND
GPIO7  -> tombol   -> GND  (internal pull-up diaktifkan lewat devicetree)
```

![Wiring tiga LED dan satu tombol di breadboard](images/05-wiring-tiga-led-tombol.png)
<!-- TODO(author): tambahkan foto wiring tiga LED (merah, kuning, hijau) dan satu tombol di breadboard, tunjukkan semua koneksi GND -->

Aplikasi di [overlay/main.c](overlay/main.c) tidak menyalakan apa pun — tujuannya murni membuktikan lewat compile-time assertion dan print bahwa devicetree terbaca benar. Dipakai macro `DT_NODE_EXISTS`, `DT_PROP`, dan `DT_GPIO_PIN` untuk menampilkan nomor pin GPIO tiap LED dan tombol langsung dari devicetree, tanpa hardcode ulang angka pin di C:

```bash
cd 05-devicetree-dari-nol
west build -p always -b esp32s3_devkitc/esp32s3/procpu overlay
west flash
west espressif monitor
```

Output yang diharapkan menunjukkan nomor pin GPIO tiap alias sesuai wiring di atas — kalau angkanya tidak sesuai fisik yang dipasang, cek lagi overlay-nya sebelum lanjut nyalakan LED beneran.

![Output serial monitor membuktikan devicetree terbaca](images/05-output-dt-macro.png)
<!-- TODO(author): tambahkan screenshot serial monitor menampilkan hasil print DT_PROP untuk tiap LED dan tombol -->

## Sumber

- [Zephyr Devicetree Guide](https://docs.zephyrproject.org/latest/build/dts/index.html)
- [Devicetree HOWTOs](https://docs.zephyrproject.org/latest/build/dts/howtos.html)
- [Devicetree API Macros](https://docs.zephyrproject.org/latest/build/dts/api/api.html)
- [devicetree.org Specification](https://www.devicetree.org/specifications/)
