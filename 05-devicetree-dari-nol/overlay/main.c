#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/*
 * Aplikasi ini tidak menyalakan LED apa pun. Tujuannya murni membuktikan
 * bahwa devicetree overlay terbaca benar, lewat compile-time check
 * (BUILD_ASSERT) dan runtime print, tanpa hardcode ulang nomor pin di C.
 */

#define LED0_NODE   DT_ALIAS(led0)
#define LED1_NODE   DT_ALIAS(led1)
#define LED2_NODE   DT_ALIAS(led2)
#define TOMBOL_NODE DT_ALIAS(sw0)

BUILD_ASSERT(DT_NODE_EXISTS(LED0_NODE), "alias led0 tidak ditemukan di devicetree");
BUILD_ASSERT(DT_NODE_EXISTS(LED1_NODE), "alias led1 tidak ditemukan di devicetree");
BUILD_ASSERT(DT_NODE_EXISTS(LED2_NODE), "alias led2 tidak ditemukan di devicetree");
BUILD_ASSERT(DT_NODE_EXISTS(TOMBOL_NODE), "alias sw0 tidak ditemukan di devicetree");

static void cetak_info_gpio(const char *nama, const struct gpio_dt_spec *spec)
{
	printk("%-12s -> port=%s pin=%d flags=0x%02x\n",
	       nama, spec->port->name, spec->pin, spec->dt_flags);
}

int main(void)
{
	const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
	const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
	const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
	const struct gpio_dt_spec tombol = GPIO_DT_SPEC_GET(TOMBOL_NODE, gpios);

	printk("\n=== Bukti devicetree terbaca ===\n");
	printk("label led0   : %s\n", DT_PROP(LED0_NODE, label));
	printk("label led1   : %s\n", DT_PROP(LED1_NODE, label));
	printk("label led2   : %s\n", DT_PROP(LED2_NODE, label));
	printk("label tombol : %s\n", DT_PROP(TOMBOL_NODE, label));

	cetak_info_gpio("led0", &led0);
	cetak_info_gpio("led1", &led1);
	cetak_info_gpio("led2", &led2);
	cetak_info_gpio("tombol", &tombol);

	printk("=== Semua node dan alias terbaca dengan benar ===\n\n");

	return 0;
}
