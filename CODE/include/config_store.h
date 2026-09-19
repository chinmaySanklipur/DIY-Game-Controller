/**
 * @file config_store.h
 * @brief Persistent runtime configuration stored in flash via littlefs.
 *
 * Holds the user-adjustable settings that should survive a power cycle:
 * deadzone sizes, response curve selection per axis, button remap table,
 * trigger calibration, and the last-selected LED mode.
 *
 * Storage lives in a small littlefs partition carved out of the RP2040's
 * 2 MB external flash, well above the firmware's own code/data footprint.
 */

#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <stdint.h>
#include <stdbool.h>
#include "deadzone.h"
#include "leds.h"

/**
 * @brief The full set of persisted settings, written/read as one flat block.
 *
 * __attribute__((packed)) keeps the on-flash layout stable across compiler
 * versions; the magic/crc fields let us detect corrupt or first-boot flash
 * contents and fall back to sane defaults automatically.
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;                 /**< Must equal CONFIG_MAGIC if valid   */

    float    deadzone_inner;        /**< Override for DEADZONE_INNER_FRAC   */
    float    deadzone_outer;        /**< Override for DEADZONE_OUTER_FRAC   */
    response_curve_t curve_left;    /**< Response curve for the left stick  */
    response_curve_t curve_right;   /**< Response curve for the right stick */

    uint16_t trig_left_min;         /**< Left trigger calibration: rest     */
    uint16_t trig_left_max;         /**< Left trigger calibration: full pull*/
    uint16_t trig_right_min;        /**< Right trigger calibration: rest    */
    uint16_t trig_right_max;        /**< Right trigger calibration: full   */

    uint16_t button_remap[17];      /**< Logical->physical button remap table */
    uint16_t turbo_mask;            /**< Bitmask of buttons with turbo enabled */
    led_mode_t led_mode;            /**< Last-selected LED animation mode    */

    uint32_t crc32;                 /**< CRC over all preceding fields       */
} runtime_config_t;

/**
 * @brief Mount the littlefs flash filesystem.
 *
 * Must succeed before config_load() or config_save() are used. If the
 * filesystem has never been formatted (fresh board), this will format it
 * automatically on first boot.
 *
 * @return true on success, false if the flash filesystem could not be
 *         mounted or formatted.
 */
bool config_store_init(void);

/**
 * @brief Load settings from flash into the supplied struct.
 *
 * If the stored file is missing, the magic number is wrong, or the CRC
 * does not match, this fills @p cfg with firmware defaults instead and
 * returns false so the caller knows the values were not actually loaded
 * from flash.
 *
 * @param[out] cfg  Destination struct.
 * @return true if valid stored settings were loaded, false if defaults
 *         were used instead.
 */
bool config_load(runtime_config_t *cfg);

/**
 * @brief Compute the CRC, then write the supplied settings to flash.
 *
 * @param cfg  Settings to persist. The crc32 field is overwritten by this
 *             function, so the caller does not need to compute it manually.
 * @return true on success.
 */
bool config_save(runtime_config_t *cfg);

/**
 * @brief Populate a runtime_config_t with the firmware's built-in defaults.
 *
 * Used both as the config_load() fallback and as the starting point when
 * a user performs a factory reset.
 *
 * @param[out] cfg  Struct to fill with default values.
 */
void config_set_defaults(runtime_config_t *cfg);

#endif /* CONFIG_STORE_H */
