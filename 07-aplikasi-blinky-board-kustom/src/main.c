#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* Semua GPIO diambil dari alias devicetree, tidak ada nomor pin hardcode di file ini. */
static const struct gpio_dt_spec leds[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
};

static const struct gpio_dt_spec tombol = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

/* Tiga tingkat kecepatan, siklus balik ke awal setelah yang tercepat. */
static const uint32_t daftar_delay_ms[] = {500, 250, 100};
#define JUMLAH_TINGKAT ARRAY_SIZE(daftar_delay_ms)

int main(void)
{
	size_t indeks_led = 0;
	size_t tingkat_kecepatan = 0;
	bool tombol_ditekan_sebelumnya = false;

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		if (!gpio_is_ready_dt(&leds[i])) {
			printk("LED indeks %d tidak siap\n", i);
			return 0;
		}
		gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
	}

	if (!gpio_is_ready_dt(&tombol)) {
		printk("Tombol tidak siap\n");
		return 0;
	}
	gpio_pin_configure_dt(&tombol, GPIO_INPUT);

	printk("Blinky board kustom siap. Tekan tombol untuk ganti kecepatan.\n");

	while (1) {
		/*
		 * Polling sederhana: deteksi transisi lepas ke tekan saja
		 * (edge terdeteksi lewat perbandingan state), supaya satu
		 * kali tekan tidak dihitung berkali-kali selama ditahan.
		 * Ini bukan debouncing yang benar, cuma cukup untuk latihan
		 * polling -- versi interrupt dengan debounce ada di Bagian 8.
		 */
		bool tombol_sekarang = gpio_pin_get_dt(&tombol) > 0;

		if (tombol_sekarang && !tombol_ditekan_sebelumnya) {
			tingkat_kecepatan = (tingkat_kecepatan + 1) % JUMLAH_TINGKAT;
			printk("Kecepatan berubah ke tingkat %d (%u ms)\n",
			       tingkat_kecepatan, daftar_delay_ms[tingkat_kecepatan]);
		}
		tombol_ditekan_sebelumnya = tombol_sekarang;

		gpio_pin_set_dt(&leds[indeks_led], 0);
		indeks_led = (indeks_led + 1) % ARRAY_SIZE(leds);
		gpio_pin_set_dt(&leds[indeks_led], 1);

		k_msleep(daftar_delay_ms[tingkat_kecepatan]);
	}

	return 0;
}
