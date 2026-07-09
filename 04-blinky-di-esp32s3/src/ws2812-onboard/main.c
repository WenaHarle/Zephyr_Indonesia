#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/sys/printk.h>
#include <string.h>

#define STRIP_NODE       DT_ALIAS(led_strip)
#define STRIP_NUM_PIXELS DT_PROP(STRIP_NODE, chain_length)

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

int main(void)
{
	struct led_rgb pixels[STRIP_NUM_PIXELS];

	if (!device_is_ready(strip)) {
		printk("Device led_strip tidak siap\n");
		return 0;
	}

	while (1) {
		memset(pixels, 0, sizeof(pixels));
		pixels[0].r = 20;
		pixels[0].g = 0;
		pixels[0].b = 0;
		led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
		k_msleep(500);

		memset(pixels, 0, sizeof(pixels));
		led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
		k_msleep(500);
	}

	return 0;
}
