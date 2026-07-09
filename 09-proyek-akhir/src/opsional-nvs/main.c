#include <zephyr/kernel.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>

/*
 * "storage_partition" adalah label partisi flash yang umum dipakai di
 * banyak board Zephyr (termasuk board Espressif) khusus untuk keperluan
 * penyimpanan aplikasi seperti NVS. Kalau board Anda tidak punya partisi
 * dengan label ini, cek dulu partition table lewat build/zephyr/zephyr.dts
 * dan sesuaikan label di bawah.
 */
#define NVS_PARTITION        storage_partition
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)

#define ID_PEMBACAAN_TERAKHIR 1

struct pembacaan_terakhir {
	int64_t timestamp_ms;
	int32_t accel_x_milli;
	int32_t accel_y_milli;
	int32_t accel_z_milli;
};

static struct nvs_fs fs;

int main(void)
{
	int ret;
	struct flash_pages_info info;
	struct pembacaan_terakhir data;
	ssize_t len;

	fs.flash_device = NVS_PARTITION_DEVICE;
	if (!device_is_ready(fs.flash_device)) {
		printk("Flash device untuk NVS tidak siap\n");
		return 0;
	}

	fs.offset = NVS_PARTITION_OFFSET;
	ret = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (ret != 0) {
		printk("Gagal ambil info flash page: %d\n", ret);
		return 0;
	}

	fs.sector_size = info.size;
	fs.sector_count = 4U;

	ret = nvs_mount(&fs);
	if (ret != 0) {
		printk("Gagal mount NVS: %d\n", ret);
		return 0;
	}

	len = nvs_read(&fs, ID_PEMBACAAN_TERAKHIR, &data, sizeof(data));
	if (len > 0) {
		printk("Pembacaan terakhir sebelum reboot: [%lld ms] x=%d y=%d z=%d (milli-m/s^2)\n",
		       data.timestamp_ms, data.accel_x_milli, data.accel_y_milli,
		       data.accel_z_milli);
	} else {
		printk("Belum ada data tersimpan di NVS\n");
	}

	/* Simulasi pembacaan baru -- di proyek nyata angka ini datang dari sensor_thread. */
	data.timestamp_ms = k_uptime_get();
	data.accel_x_milli = 981;
	data.accel_y_milli = 12;
	data.accel_z_milli = -34;

	ret = nvs_write(&fs, ID_PEMBACAAN_TERAKHIR, &data, sizeof(data));
	if (ret < 0) {
		printk("Gagal tulis ke NVS: %d\n", ret);
	} else {
		printk("Pembacaan baru disimpan. Reboot board untuk membuktikan datanya bertahan.\n");
	}

	return 0;
}
