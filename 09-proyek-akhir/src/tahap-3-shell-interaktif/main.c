#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

#define SENSOR_NODE DT_NODELABEL(mpu6050)
static const struct device *const sensor = DEVICE_DT_GET(SENSOR_NODE);

struct sensor_reading {
	int64_t timestamp_ms;
	struct sensor_value accel_x;
	struct sensor_value accel_y;
	struct sensor_value accel_z;
};

K_MSGQ_DEFINE(antrian_sensor, sizeof(struct sensor_reading), 8, 4);

#define SENSOR_THREAD_STACK_SIZE 1024
#define LOGGER_THREAD_STACK_SIZE 1024
#define SENSOR_THREAD_PRIORITY   5
#define LOGGER_THREAD_PRIORITY   6

static atomic_t logging_aktif = ATOMIC_INIT(0);
static uint32_t total_dicatat;

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
		/* Kalau logging tidak aktif, thread sensor tetap idle, tidak fetch sama sekali. */
		if (atomic_get(&logging_aktif)) {
			struct sensor_reading data;
			int ret = sensor_sample_fetch(sensor);

			if (ret == 0) {
				sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_X, &data.accel_x);
				sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_Y, &data.accel_y);
				sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_Z, &data.accel_z);
				data.timestamp_ms = k_uptime_get();

				if (k_msgq_put(&antrian_sensor, &data, K_NO_WAIT) != 0) {
					struct sensor_reading dibuang;

					k_msgq_get(&antrian_sensor, &dibuang, K_NO_WAIT);
					k_msgq_put(&antrian_sensor, &data, K_NO_WAIT);
				}
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
		total_dicatat++;

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

static int cmd_start(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	atomic_set(&logging_aktif, 1);
	shell_print(sh, "Logging diaktifkan");
	return 0;
}

static int cmd_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	atomic_set(&logging_aktif, 0);
	shell_print(sh, "Logging dihentikan");
	return 0;
}

static int cmd_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Status  : %s", atomic_get(&logging_aktif) ? "aktif" : "berhenti");
	shell_print(sh, "Total   : %u pembacaan tercatat", total_dicatat);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_datalogger,
	SHELL_CMD(start, NULL, "Mulai logging sensor", cmd_start),
	SHELL_CMD(stop, NULL, "Hentikan logging sensor", cmd_stop),
	SHELL_CMD(status, NULL, "Tampilkan status logging", cmd_status),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(datalogger, &sub_datalogger, "Perintah data logger sensor", NULL);

int main(void)
{
	printk("Data logger siap. Ketik 'datalogger start' di shell untuk mulai.\n");
	return 0;
}
