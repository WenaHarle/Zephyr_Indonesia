#include <zephyr/kernel.h>
#include <zephyr/version.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	printk("Halo dari ESP32-S3! Ini aplikasi pertama saya di Zephyr.\n");
	printk("Board   : %s\n", CONFIG_BOARD);
	printk("Kernel  : Zephyr %s\n", KERNEL_VERSION_STRING);

	uint32_t hitungan = 0;

	while (1) {
		printk("masih hidup, hitungan = %u\n", hitungan);
		hitungan++;
		k_sleep(K_SECONDS(2));
	}

	return 0;
}
