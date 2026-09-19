/**
 * @file config.h
 * @brief Central configuration: GPIO pin assignments, timing, and tunable constants.
 *
 * Edit this file to remap pins or change deadzone/lighting behaviour.
 * Every other source file includes this header so changes here propagate
 * automatically at build time.
 *
 * Pin assignments mirror the KiCad schematic (game_controller.kicad_sch).
 * Net names in comments match schematic net labels for easy cross-reference.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "pico/stdlib.h"

/* ─────────────────────────────────────────────────────────────────────────────
 * GPIO — Digital Buttons
 * All buttons are active-LOW (pulled HIGH internally; pressed = connected to GND)
 * ───────────────────────────────────────────────────────────────────────────── */

/** Face buttons — schematic nets BTN_A … BTN_Y */
#define PIN_BTN_A       13   /* GP13 */
#define PIN_BTN_B       14   /* GP14 */
#define PIN_BTN_X       15   /* GP15 */
#define PIN_BTN_Y       16   /* GP16 */

/** D-pad — schematic nets DPAD_UP … DPAD_RIGHT */
#define PIN_DPAD_UP      2   /* GP2  */
#define PIN_DPAD_DN      3   /* GP3  */
#define PIN_DPAD_LEFT    6   /* GP6  */
#define PIN_DPAD_RIGHT   7   /* GP7  */

/** Bumpers — schematic nets BTN_LB / BTN_RB */
#define PIN_BTN_LB       8   /* GP8  */
#define PIN_BTN_RB       9   /* GP9  */

/** System buttons — schematic nets BTN_MENU / BTN_VIEW / BTN_GUIDE / BTN_SHARE */
#define PIN_BTN_MENU    17   /* GP17 */
#define PIN_BTN_VIEW    18   /* GP18 */
#define PIN_BTN_GUIDE   19   /* GP19 */
#define PIN_BTN_SHARE   12   /* GP12 */

/** Thumbstick click buttons — schematic nets BTN_LS / BTN_RS */
#define PIN_BTN_LS      20   /* GP20 */
#define PIN_BTN_RS      21   /* GP21 */

/* ─────────────────────────────────────────────────────────────────────────────
 * GPIO — Analog (ADC)
 * RP2040 ADC inputs are GP26-GP29.  Only GP26-GP28 are brought out on Pico.
 * Thumbstick axes are read on ADC0 and ADC1 (left stick).
 * ───────────────────────────────────────────────────────────────────────────── */

/** Left stick — schematic nets ADC0_LSTICK_X / ADC1_LSTICK_Y */
#define PIN_LSTICK_X    26   /* GP26 / ADC0 */
#define PIN_LSTICK_Y    27   /* GP27 / ADC1 */
#define ADC_LSTICK_X     0   /* ADC channel index for GP26 */
#define ADC_LSTICK_Y     1   /* ADC channel index for GP27 */

/**
 * Right stick — the Pico only exposes ADC on GP26-GP28, so the right stick
 * connects to GP10/GP11 read via external circuitry.  For this firmware we
 * treat these as the same two ADC channels multiplexed or read via the second
 * pair of physical lines.  Adjust if hardware differs.
 */
#define PIN_RSTICK_X    10   /* GP10 — right stick horizontal */
#define PIN_RSTICK_Y    11   /* GP11 — right stick vertical   */
/* Note: GP10/GP11 are NOT native ADC pins on RP2040.
 * If using a resistor-ladder or MCP3204 for right stick, replace ADC reads
 * in inputs.c with the appropriate SPI/I2C reads.
 * For a direct wiring option, swap thumbstick to GP26/GP27 for left and
 * route right stick via an external ADC IC. */

/* ─────────────────────────────────────────────────────────────────────────────
 * GPIO — I2C for AS5600 hall-effect trigger sensors
 * I2C0 → Left trigger  (U2, address 0x36)
 * I2C1 → Right trigger (U3, address 0x36 — separate bus avoids conflict)
 * ───────────────────────────────────────────────────────────────────────────── */

#define PIN_I2C0_SDA     0   /* GP0  — I2C0_SDA → U2 */
#define PIN_I2C0_SCL     1   /* GP1  — I2C0_SCL → U2 */
#define PIN_I2C1_SDA     4   /* GP4  — I2C1_SDA → U3 */
#define PIN_I2C1_SCL     5   /* GP5  — I2C1_SCL → U3 */

#define I2C_SPEED_HZ    400000   /* 400 kHz Fast-mode */
#define AS5600_ADDR     0x36     /* Fixed I2C address for AS5600 */
#define AS5600_REG_ANGLE_H 0x0C /* High byte of 12-bit raw angle */
#define AS5600_REG_ANGLE_L 0x0D /* Low  byte of 12-bit raw angle */

/* ─────────────────────────────────────────────────────────────────────────────
 * GPIO — WS2812B RGB LED data line
 * Uses PIO state machine so the CPU is never blocked during LED updates.
 * ───────────────────────────────────────────────────────────────────────────── */

#define PIN_WS2812      22   /* GP22 — WS2812_DIN through 470-ohm series R */
#define NUM_LEDS         8   /* LED1–LED8 in daisy-chain */
#define WS2812_PIO      pio0 /* Which PIO block to use */
#define WS2812_SM        0   /* State machine index within that PIO */

/* ─────────────────────────────────────────────────────────────────────────────
 * Deadzone and response-curve parameters
 * ───────────────────────────────────────────────────────────────────────────── */

/**
 * Inner circular deadzone radius as a fraction of the full stick range.
 * 0.08 = 8 % — deflections smaller than this produce zero output.
 * Prevents stick drift when the stick rests slightly off-centre.
 */
#define DEADZONE_INNER_FRAC   0.08f

/**
 * Outer clamp: values this close to maximum are treated as full deflection.
 * Compensates for sticks that do not mechanically reach true maximum.
 */
#define DEADZONE_OUTER_FRAC   0.97f

/**
 * Trigger inner dead-band: first N% of travel outputs zero.
 * Prevents accidental trigger input when the trigger rests slightly depressed.
 */
#define TRIGGER_INNER_FRAC    0.04f

/**
 * Trigger outer clamp: anything beyond this fraction is treated as fully pressed.
 */
#define TRIGGER_OUTER_FRAC    0.97f

/* ─────────────────────────────────────────────────────────────────────────────
 * ADC calibration storage keys in flash (littlefs)
 * ───────────────────────────────────────────────────────────────────────────── */

/** Path inside the littlefs volume where the config struct is stored */
#define CONFIG_FLASH_PATH  "/config.bin"

/** Magic number at start of stored config — detects uninitialised/corrupt data */
#define CONFIG_MAGIC  0xCAFE2026u

/* ─────────────────────────────────────────────────────────────────────────────
 * USB HID
 * ───────────────────────────────────────────────────────────────────────────── */

/** HID report interval in milliseconds (8 ms = 125 Hz, maximum for USB FS HID) */
#define HID_REPORT_INTERVAL_MS  8

/* ─────────────────────────────────────────────────────────────────────────────
 * Debounce
 * ───────────────────────────────────────────────────────────────────────────── */

/**
 * Number of consecutive identical readings required before a button state
 * change is accepted.  At a 1 kHz polling rate this gives a 3 ms debounce.
 */
#define DEBOUNCE_COUNT  3

#endif /* CONFIG_H */
