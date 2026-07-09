#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

#define SENSOR_NODE DT_NODELABEL(mpu6050)
static const struct device *const sensor = DEVICE_DT_GET(SENSOR_NODE);

struct sensor_reading {
	int64_t timestamp_ms;
	struct sensor_value accel_x;
	struct sensor_value accel_y;
	struct sensor_value accel_z;
};

/* Antrian menampung 8 pembacaan, alignment 4 byte cukup untuk struct ini. */
K_MSGQ_DEFINE(antrian_sensor, sizeof(struct sensor_reading), 8, 4);

#define SENSOR_THREAD_STACK_SIZE 1024
#define LOGGER_THREAD_STACK_SIZE 1024
#define SENSOR_THREAD_PRIORITY   5
#define LOGGER_THREAD_PRIORITY   6

static void sensor_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (!device_is_ready(sensor)) {
		printk("Sensor MPU6050 tidak siap, thread sensor berhenti\n");
		return;
	}

	while (1) {
		struct sensor_reading data;
		int ret = sensor_sample_fetch(sensor);

		if (ret == 0) {
			sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_X, &data.accel_x);
			sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_Y, &data.accel_y);
			sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_Z, &data.accel_z);
			data.timestamp_ms = k_uptime_get();

			/*
			 * Non-blocking: kalau antrian penuh (logger lebih
			 * lambat dari sensor), buang satu sample terlama
			 * daripada thread sensor ikut terblokir menunggu.
			 */
			if (k_msgq_put(&antrian_sensor, &data, K_NO_WAIT) != 0) {
				struct sensor_reading dibuang;

				k_msgq_get(&antrian_sensor, &dibuang, K_NO_WAIT);
				k_msgq_put(&antrian_sensor, &data, K_NO_WAIT);
			}
		}

		k_msleep(1000);
	}
}

static void logger_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct sensor_reading data;

	while (1) {
		k_msgq_get(&antrian_sensor, &data, K_FOREVER);

		printk("[%lld ms] accel x=%d.%06d y=%d.%06d z=%d.%06d m/s^2\n",
		       data.timestamp_ms, data.accel_x.val1, abs(data.accel_x.val2),
		       data.accel_y.val1, abs(data.accel_y.val2), data.accel_z.val1,
		       abs(data.accel_z.val2));
	}
}

K_THREAD_DEFINE(sensor_tid, SENSOR_THREAD_STACK_SIZE, sensor_thread_fn, NULL, NULL, NULL,
		 SENSOR_THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(logger_tid, LOGGER_THREAD_STACK_SIZE, logger_thread_fn, NULL, NULL, NULL,
		 LOGGER_THREAD_PRIORITY, 0, 0);

int main(void)
{
	printk("Data logger dua-thread siap (sensor_thread + logger_thread)\n");
	return 0;
}
