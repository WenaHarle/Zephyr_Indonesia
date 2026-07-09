#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/*
 * Aplikasi bring-up tahap 1 dan 2: buktikan board boot dan console UART
 * jalan. LED cuma ditambahkan sebagai indikator visual sederhana untuk
 * tahap 3, dijaga seminimal mungkin supaya kalau print tidak muncul,
 * masalahnya jelas di console/UART bukan di logic LED.
 */

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
	printk("\n=== Board zi_wroom_s3 boot berhasil ===\n");
	printk("Board   : %s\n", CONFIG_BOARD);
	printk("Kernel  : Zephyr %s\n", KERNEL_VERSION_STRING);

	if (!gpio_is_ready_dt(&led)) {
		printk("Peringatan: GPIO LED belum siap, tahap 3 belum lolos\n");
		return 0;
	}

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	printk("GPIO LED siap, tahap 3 (peripheral) lolos\n");

	while (1) {
		gpio_pin_toggle_dt(&led);
		k_msleep(1000);
	}

	return 0;
}
