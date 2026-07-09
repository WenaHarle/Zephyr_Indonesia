#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

#define SENSOR_NODE DT_NODELABEL(mpu6050)
static const struct device *const sensor = DEVICE_DT_GET(SENSOR_NODE);

int main(void)
{
	if (!device_is_ready(sensor)) {
		printk("Sensor MPU6050 tidak siap, cek wiring I2C dan alamat device\n");
		return 0;
	}

	printk("Sensor siap, mulai baca data setiap 1 detik\n");

	while (1) {
		struct sensor_value ax, ay, az;
		int ret = sensor_sample_fetch(sensor);

		if (ret < 0) {
			printk("Gagal fetch sample: %d\n", ret);
			k_msleep(1000);
			continue;
		}

		sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_X, &ax);
		sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_Y, &ay);
		sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_Z, &az);

		printk("accel x=%d.%06d y=%d.%06d z=%d.%06d m/s^2\n",
		       ax.val1, abs(ax.val2), ay.val1, abs(ay.val2), az.val1, abs(az.val2));

		k_msleep(1000);
	}

	return 0;
}
