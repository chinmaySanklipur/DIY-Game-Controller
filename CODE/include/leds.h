/**
 * @file leds.h
 * @brief WS2812B addressable RGB LED driver using RP2040 PIO.
 *
 * Driving WS2812B over PIO (rather than bit-banging in software) means the
 * 800 kHz NRZ timing is generated entirely in hardware — the CPU just pushes
 * 24-bit colour words into a FIFO and continues running the main loop with
 * zero blocking.
 *
 * Colour order on the wire is GRB (not RGB) per the WS2812B datasheet.
 */

#ifndef LEDS_H
#define LEDS_H

#include <stdint.h>

/** Lighting modes selectable via the Guide-button hold gesture. */
typedef enum {
    LED_MODE_SOLID_BLUE,    /**< Idle / USB-connected state           */
    LED_MODE_BREATHING_CYAN,/**< Active gameplay indicator             */
    LED_MODE_RAINBOW        /**< Cycling rainbow, toggled by the user  */
} led_mode_t;

/**
 * @brief Claim a PIO state machine and load the WS2812 program onto it.
 *
 * Must be called once at startup before any other leds_* function.
 */
void leds_init(void);

/**
 * @brief Set a single LED's colour in the local framebuffer (not yet sent).
 *
 * @param index  LED position in the chain, 0-7 (LED1...LED8 on schematic).
 * @param r,g,b  8-bit colour channel values, 0-255 each.
 */
void leds_set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Push the local framebuffer out to the physical LED chain.
 *
 * Must be called after one or more leds_set_pixel() calls for the change
 * to become visible — set_pixel only touches the in-memory buffer.
 */
void leds_show(void);

/**
 * @brief Advance the currently active lighting animation by one frame.
 *
 * Call this periodically (e.g. every 16 ms for ~60 fps animation) from the
 * main loop. Internally tracks animation phase using time since boot, so
 * it is safe to call at an inconsistent cadence.
 *
 * @param mode  Which animation pattern to render this frame.
 */
void leds_update_animation(led_mode_t mode);

#endif /* LEDS_H */
