#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define LED_NODE DT_ALIAS(led0)
#define SW_NODE  DT_ALIAS(sw0)

#define DEBOUNCE_MS      50
#define DURASI_KEDIP_MS  150

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
static const struct gpio_dt_spec tombol = GPIO_DT_SPEC_GET(SW_NODE, gpios);

static struct gpio_callback tombol_cb_data;
static uint32_t jumlah_kedip = 1;

static void burst_kedip_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	printk("Penekanan valid terdeteksi, burst %u kedip\n", jumlah_kedip);

	for (uint32_t i = 0; i < jumlah_kedip; i++) {
		gpio_pin_set_dt(&led, 1);
		k_msleep(DURASI_KEDIP_MS);
		gpio_pin_set_dt(&led, 0);
		k_msleep(DURASI_KEDIP_MS);
	}

	jumlah_kedip++;
}

K_WORK_DEFINE(burst_kedip_work, burst_kedip_handler);

static void debounce_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	/* Sinyal sudah settle DEBOUNCE_MS tanpa edge baru, ini penekanan valid. */
	k_work_submit(&burst_kedip_work);
}

K_WORK_DELAYABLE_DEFINE(debounce_work, debounce_handler);

static void tombol_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	/*
	 * ISR cuma reschedule delayable work, tidak melakukan apa pun lagi.
	 * Kalau tombol bouncing, tiap edge di sini menggeser ulang jadwal,
	 * jadi debounce_handler betulan cuma jalan sekali setelah sinyal
	 * settle selama DEBOUNCE_MS tanpa edge baru.
	 */
	k_work_reschedule(&debounce_work, K_MSEC(DEBOUNCE_MS));
}

int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&tombol)) {
		printk("Device belum siap\n");
		return 0;
	}

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&tombol, GPIO_INPUT);

	ret = gpio_pin_interrupt_configure_dt(&tombol, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Gagal konfigurasi interrupt: %d\n", ret);
		return 0;
	}

	gpio_init_callback(&tombol_cb_data, tombol_isr, BIT(tombol.pin));
	gpio_add_callback(tombol.port, &tombol_cb_data);

	printk("Siap. Tiap penekanan tombol menambah jumlah kedip berikutnya.\n");

	/* Semua kerja terjadi lewat callback interrupt dan workqueue, thread utama menganggur. */
	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
