/**
 * @file inputs.h
 * @brief Digital button and analog thumbstick polling interface.
 *
 * Handles GPIO initialisation for all buttons, software debouncing, and
 * ADC sampling for the two thumbsticks. All raw button states are
 * collected into a single inputs_state_t snapshot once per polling cycle.
 */

#ifndef INPUTS_H
#define INPUTS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Snapshot of every digital and raw-analog input for one poll cycle.
 *
 * Analog values here are RAW 12-bit ADC readings (0-4095), not yet scaled
 * or deadzone-corrected — that happens later in deadzone.c.
 */
typedef struct {
    bool btn_a, btn_b, btn_x, btn_y;
    bool dpad_up, dpad_dn, dpad_left, dpad_right;
    bool btn_lb, btn_rb;
    bool btn_menu, btn_view, btn_guide, btn_share;
    bool btn_ls, btn_rs;          /* thumbstick click buttons */

    uint16_t lstick_x_raw;        /* 0-4095 raw ADC */
    uint16_t lstick_y_raw;
    uint16_t rstick_x_raw;
    uint16_t rstick_y_raw;
} inputs_state_t;

/**
 * @brief Configure every digital button GPIO as input with pull-up enabled,
 *        and initialise the ADC peripheral for thumbstick reads.
 *
 * Must be called once during startup before inputs_poll() is used.
 */
void inputs_init(void);

/**
 * @brief Read every button and thumbstick, applying software debouncing.
 *
 * Designed to be called once per main-loop iteration (1 kHz target).
 * Debouncing requires DEBOUNCE_COUNT consecutive identical raw readings
 * before a button's reported state changes, filtering out mechanical
 * contact bounce without adding perceptible input latency.
 *
 * @param[out] state  Filled with the debounced/sampled snapshot.
 */
void inputs_poll(inputs_state_t *state);

#endif /* INPUTS_H */
