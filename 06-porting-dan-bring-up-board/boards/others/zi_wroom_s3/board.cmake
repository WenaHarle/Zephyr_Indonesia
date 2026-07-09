# SPDX-License-Identifier: Apache-2.0

# Modul WROOM telanjang tidak punya native USB, flashing selalu lewat
# converter serial eksternal. Sesuaikan device port sesuai hasil dmesg,
# default di bawah cuma tebakan awal yang paling umum.
board_runner_args(esp32 "--esp-device=/dev/ttyUSB0")

include(${ZEPHYR_BASE}/boards/common/esp32.board.cmake)
