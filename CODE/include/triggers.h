/**
 * @file triggers.h
 * @brief AS5600 hall-effect analog trigger driver (I2C).
 *
 * Each trigger uses an AS5600 contactless magnetic angle sensor mounted
 * on the trigger's pivot. As the trigger is pulled, a small magnet sweeps
 * past the sensor and its 12-bit raw angle output tracks trigger position
 * with no mechanical wear (unlike a potentiometer).
 *
 * Left trigger  → I2C0 (GP0/GP1)
 * Right trigger → I2C1 (GP4/GP5)
 * Both sensors share I2C address 0x36, so they must be on separate buses.
 */

#ifndef TRIGGERS_H
#define TRIGGERS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialise both I2C buses used by the left and right AS5600 sensors.
 *
 * Configures I2C0 and I2C1 at 400 kHz Fast-mode and sets the relevant
 * GPIO pins to I2C function with internal pull-ups enabled (belt-and-braces;
 * the PCB also has discrete 4.7k pull-up resistors per the schematic).
 */
void triggers_init(void);

/**
 * @brief Read the raw 12-bit angle from the left trigger's AS5600.
 *
 * @return Raw angle 0-4095, or 0xFFFF if the I2C read failed (e.g. sensor
 *         not detected). Calling code should treat 0xFFFF as "no trigger
 *         input available" rather than a valid angle.
 */
uint16_t triggers_read_left_raw(void);

/**
 * @brief Read the raw 12-bit angle from the right trigger's AS5600.
 * @return Raw angle 0-4095, or 0xFFFF on I2C failure.
 */
uint16_t triggers_read_right_raw(void);

/**
 * @brief One-time calibration: capture the trigger's rest and full-pull angles.
 *
 * Call this during a calibration routine (e.g. held button combo at boot)
 * with the trigger physically at rest, then again at full pull, to record
 * the working angle range. Results are written into the supplied pointers
 * so the caller can persist them via config.c.
 *
 * @param is_left      true = read the left trigger, false = right trigger.
 * @param[out] out_min Raw angle measured at the rest position.
 * @param[out] out_max Raw angle measured at full pull.
 */
void triggers_calibrate(bool is_left, uint16_t *out_min, uint16_t *out_max);

#endif /* TRIGGERS_H */
