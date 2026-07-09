#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	printk("Interval logging saat ini: %d ms (diatur lewat CONFIG_LOG_INTERVAL_MS)\n",
	       CONFIG_LOG_INTERVAL_MS);

	uint32_t hitungan = 0;

	while (1) {
		printk("log periodik #%u\n", hitungan);
		hitungan++;
		/* Nilai interval murni dari Kconfig, tidak ada angka hardcode di sini. */
		k_sleep(K_MSEC(CONFIG_LOG_INTERVAL_MS));
	}

	return 0;
}
