/**
 * @file deadzone.h
 * @brief Thumbstick deadzone shaping and analog trigger range mapping.
 *
 * Raw ADC values are noisy near the stick's centre rest position (causing
 * "stick drift" if sent directly to the OS) and rarely reach the true
 * electrical extremes (so a naive linear scale never reports 100%).
 * This module fixes both problems before any value reaches hid.c.
 */

#ifndef DEADZONE_H
#define DEADZONE_H

#include <stdint.h>

/** Per-axis response curve shape, selectable in the runtime config. */
typedef enum {
    CURVE_LINEAR,      /**< 1:1 — output proportional to input             */
    CURVE_QUADRATIC,   /**< x^2 — finer control near centre (FPS aiming)   */
    CURVE_SCURVE       /**< 3x^2-2x^3 — smooth centre, fast near the edges */
} response_curve_t;

/**
 * @brief Apply a circular deadzone and response curve to one thumbstick axis pair.
 *
 * Treats (x, y) as a single 2D vector so the deadzone is a circle rather
 * than two independent square deadzones — this avoids the "diagonal
 * dead-square" artefact that square deadzones produce.
 *
 * @param raw_x        Raw ADC reading for the X axis, 0-4095.
 * @param raw_y        Raw ADC reading for the Y axis, 0-4095.
 * @param curve        Which response curve to apply to the output magnitude.
 * @param[out] out_x   Final signed 16-bit X value for the HID report.
 * @param[out] out_y   Final signed 16-bit Y value for the HID report.
 */
void deadzone_apply_stick(uint16_t raw_x, uint16_t raw_y,
                           response_curve_t curve,
                           int16_t *out_x, int16_t *out_y);

/**
 * @brief Map a trigger's raw AS5600 angle into a calibrated 0-32767 output.
 *
 * Applies inner/outer flat zones (see config.h) using the supplied
 * calibration range, so trigger output reads exactly 0 at rest and
 * exactly 32767 at full pull even if the mechanical range varies slightly
 * between units.
 *
 * @param raw_angle    Raw 12-bit AS5600 angle reading, 0-4095.
 * @param cal_min      Angle reading recorded at the trigger's rest position.
 * @param cal_max      Angle reading recorded at the trigger's full-pull position.
 * @return              Scaled trigger value, 0-32767.
 */
int16_t deadzone_apply_trigger(uint16_t raw_angle, uint16_t cal_min, uint16_t cal_max);

#endif /* DEADZONE_H */
