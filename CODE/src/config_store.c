/**
 * @file config_store.c
 * @brief Implements flash persistence for runtime_config_t via littlefs.
 *
 * This file depends on the third-party "pico-littlefs" library
 * (https://github.com/lurk101/pico-littlefs), which must be added as a
 * git submodule — see README.md for setup instructions. It is not part
 * of the official Pico SDK.
 *
 * pico-littlefs exposes a small POSIX-like API (pico_mount, pico_open,
 * pico_read, pico_write, pico_close) backed by a configurable region of
 * the RP2040's external QSPI flash, reserved automatically below the
 * compiled firmware image.
 */

#include "config_store.h"
#include "config.h"
#include "pico_hal.h"   /* from the pico-littlefs submodule */
#include <string.h>

/**
 * @brief Compute a CRC32 checksum over a block of memory.
 *
 * Standard bit-reflected CRC32 (same polynomial as zlib/PNG), implemented
 * directly rather than pulling in an external library since we only need
 * it for this one struct's integrity check.
 *
 * @param data    Pointer to the bytes to checksum.
 * @param length  Number of bytes to include.
 * @return        32-bit CRC value.
 */
static uint32_t crc32_compute(const void *data, size_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;

    for (size_t i = 0; i < length; i++) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; bit++) {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

bool config_store_init(void)
{
    /* pico_mount(false) attempts to mount an existing filesystem without
     * reformatting. If no valid filesystem is found (fresh/blank flash),
     * it returns an error — in that case we format once and mount again,
     * which only happens on a board's very first boot. */
    if (pico_mount(false) != 0) {
        if (pico_mount(true) != 0) {
            return false; /* formatting also failed — flash hardware problem */
        }
    }
    return true;
}

void config_set_defaults(runtime_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    cfg->magic = CONFIG_MAGIC;
    cfg->deadzone_inner = DEADZONE_INNER_FRAC;
    cfg->deadzone_outer = DEADZONE_OUTER_FRAC;
    cfg->curve_left  = CURVE_LINEAR;
    cfg->curve_right = CURVE_LINEAR;

    /* Trigger calibration defaults span the AS5600's full 12-bit range;
     * real values should be overwritten by triggers_calibrate() the first
     * time the user runs the calibration routine. */
    cfg->trig_left_min  = 0;
    cfg->trig_left_max  = 4095;
    cfg->trig_right_min = 0;
    cfg->trig_right_max = 4095;

    /* Default remap table is the identity mapping: logical button N
     * reports as physical button N. button_remap[i] = i for all 17 slots. */
    for (int i = 0; i < 17; i++) {
        cfg->button_remap[i] = (uint16_t)i;
    }

    cfg->turbo_mask = 0;          /* turbo disabled on every button by default */
    cfg->led_mode = LED_MODE_SOLID_BLUE;

    /* crc32 is intentionally left at 0 here — config_save() recomputes it
     * right before writing, so an in-memory "defaults" struct never needs
     * a valid CRC of its own. */
}

bool config_load(runtime_config_t *cfg)
{
    int file = pico_open(CONFIG_FLASH_PATH, LFS_O_RDONLY);
    if (file < 0) {
        /* No config file yet (fresh board) — fall back to defaults. */
        config_set_defaults(cfg);
        return false;
    }

    lfs_size_t bytes_read = pico_read(file, cfg, sizeof(runtime_config_t));
    pico_close(file);

    if (bytes_read != sizeof(runtime_config_t)) {
        /* Partial/truncated file — treat as corrupt. */
        config_set_defaults(cfg);
        return false;
    }

    if (cfg->magic != CONFIG_MAGIC) {
        /* File exists but doesn't look like our struct (e.g. firmware
         * version mismatch from an old layout) — fall back to defaults
         * rather than interpreting garbage as valid settings. */
        config_set_defaults(cfg);
        return false;
    }

    /* Verify integrity: recompute the CRC over everything except the
     * stored crc32 field itself, and compare. */
    uint32_t stored_crc = cfg->crc32;
    cfg->crc32 = 0;
    uint32_t computed_crc = crc32_compute(cfg, sizeof(runtime_config_t));
    cfg->crc32 = stored_crc;

    if (computed_crc != stored_crc) {
        config_set_defaults(cfg);
        return false;
    }

    return true;
}

bool config_save(runtime_config_t *cfg)
{
    cfg->magic = CONFIG_MAGIC;

    /* CRC is computed over the struct with crc32 itself zeroed, so the
     * checksum doesn't depend on its own previous value. */
    cfg->crc32 = 0;
    cfg->crc32 = crc32_compute(cfg, sizeof(runtime_config_t));

    int file = pico_open(CONFIG_FLASH_PATH, LFS_O_WRONLY | LFS_O_CREAT);
    if (file < 0) {
        return false;
    }

    lfs_size_t bytes_written = pico_write(file, cfg, sizeof(runtime_config_t));
    pico_close(file);

    return bytes_written == sizeof(runtime_config_t);
}
