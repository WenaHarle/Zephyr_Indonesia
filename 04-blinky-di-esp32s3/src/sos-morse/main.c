#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Timing morse, rasio 1:3 antara titik dan garis, ini konvensi umum bukan aturan baku. */
#define DURASI_TITIK_MS      200
#define DURASI_GARIS_MS      600
#define JEDA_ANTAR_SIMBOL_MS 200
#define JEDA_ANTAR_HURUF_MS  600
#define JEDA_ANTAR_KATA_MS   1400

static void nyala(uint32_t durasi_ms)
{
	gpio_pin_set_dt(&led, 1);
	k_msleep(durasi_ms);
	gpio_pin_set_dt(&led, 0);
	k_msleep(JEDA_ANTAR_SIMBOL_MS);
}

static void titik(void)
{
	nyala(DURASI_TITIK_MS);
}

static void garis(void)
{
	nyala(DURASI_GARIS_MS);
}

static void kirim_sos(void)
{
	/* S = ... */
	titik();
	titik();
	titik();
	k_msleep(JEDA_ANTAR_HURUF_MS);

	/* O = --- */
	garis();
	garis();
	garis();
	k_msleep(JEDA_ANTAR_HURUF_MS);

	/* S = ... */
	titik();
	titik();
	titik();
}

int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		printk("Device GPIO untuk led0 tidak siap\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Gagal konfigurasi pin LED: %d\n", ret);
		return 0;
	}

	while (1) {
		kirim_sos();
		k_msleep(JEDA_ANTAR_KATA_MS);
	}

	return 0;
}
