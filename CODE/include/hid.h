/**
 * @file hid.h
 * @brief USB HID gamepad report descriptor and report structure.
 *
 * Defines the HID descriptor that tells the host OS how to interpret our
 * USB reports, and the C struct that mirrors that descriptor's layout.
 *
 * The descriptor follows the USB HID specification (version 1.11) and the
 * HID Usage Tables. It declares:
 *   - 6 signed 16-bit axes   (LX, LY, RX, RY, LT, RT)
 *   - 17 buttons             (16 packed into 2 bytes + 1 Share button byte)
 *   - 1 hat switch           (8-direction D-pad, 4 bits, 4 bits padding)
 *
 * Windows, macOS, and Steam recognise this layout as a standard gamepad
 * without any driver installation.
 */

#ifndef HID_H
#define HID_H

#include <stdint.h>
#include <stdbool.h>
#include "tusb.h"    /* TinyUSB — included with the Pico SDK */

/* ─────────────────────────────────────────────────────────────────────────────
 * HID Report Descriptor
 * Declared extern here, defined in hid.c, registered in usb_descriptors.c.
 * ───────────────────────────────────────────────────────────────────────────── */
extern const uint8_t hid_report_descriptor[];
extern const uint16_t hid_report_descriptor_len;

/* ─────────────────────────────────────────────────────────────────────────────
 * Gamepad Report Structure
 *
 * Must match the HID descriptor byte-for-byte.
 * __attribute__((packed)) prevents the compiler inserting padding bytes
 * that would desynchronise the struct layout from what the OS expects.
 * ───────────────────────────────────────────────────────────────────────────── */
typedef struct __attribute__((packed)) {
    int16_t  lx;          /**< Left  stick X axis, -32768 … +32767 */
    int16_t  ly;          /**< Left  stick Y axis, -32768 … +32767 */
    int16_t  rx;          /**< Right stick X axis, -32768 … +32767 */
    int16_t  ry;          /**< Right stick Y axis, -32768 … +32767 */
    int16_t  lt;          /**< Left  trigger,       0 … +32767      */
    int16_t  rt;          /**< Right trigger,       0 … +32767      */
    uint16_t buttons;     /**< Bitmask of 16 buttons                */
    uint8_t  share_btn;   /**< Bit 0 = Share; bits 1-7 reserved     */
    uint8_t  hat;         /**< Hat switch: 0=N 1=NE 2=E … 7=NW 8=centre */
} gamepad_report_t;

/* ─────────────────────────────────────────────────────────────────────────────
 * Button bit positions within gamepad_report_t.buttons
 * ───────────────────────────────────────────────────────────────────────────── */
#define BTN_A_BIT       (1u << 0)
#define BTN_B_BIT       (1u << 1)
#define BTN_X_BIT       (1u << 2)
#define BTN_Y_BIT       (1u << 3)
#define BTN_LB_BIT      (1u << 4)
#define BTN_RB_BIT      (1u << 5)
#define BTN_MENU_BIT    (1u << 6)   /* Xbox "Menu"  (≈ Start)  */
#define BTN_VIEW_BIT    (1u << 7)   /* Xbox "View"  (≈ Select) */
#define BTN_GUIDE_BIT   (1u << 8)   /* Xbox guide / home       */
#define BTN_LS_BIT      (1u << 9)   /* Left  stick click       */
#define BTN_RS_BIT      (1u << 10)  /* Right stick click       */
#define BTN_SHARE_BIT   (1u << 0)   /* In share_btn byte, bit 0 */

/* Hat-switch values (matches HID spec, 0-based CCW from North) */
#define HAT_NORTH       0
#define HAT_NORTHEAST   1
#define HAT_EAST        2
#define HAT_SOUTHEAST   3
#define HAT_SOUTH       4
#define HAT_SOUTHWEST   5
#define HAT_WEST        6
#define HAT_NORTHWEST   7
#define HAT_CENTRE      8   /**< No direction pressed */

/* ─────────────────────────────────────────────────────────────────────────────
 * Public API
 * ───────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Send the current gamepad_report_t to the USB host.
 *
 * Calls tud_hid_report() and handles the case where the USB stack is not
 * yet ready (report is silently dropped rather than blocking).
 *
 * @param report  Pointer to the populated report struct.
 */
void hid_send_report(const gamepad_report_t *report);

/**
 * @brief Compute the hat-switch value from the four D-pad button states.
 *
 * Combines the four booleans into a single 4-bit hat direction code
 * per the HID Usage Tables spec.
 *
 * @param up     True when D-pad UP is pressed.
 * @param down   True when D-pad DOWN is pressed.
 * @param left   True when D-pad LEFT is pressed.
 * @param right  True when D-pad RIGHT is pressed.
 * @return       HAT_* constant (0-7) or HAT_CENTRE (8) if none pressed.
 */
uint8_t hid_dpad_to_hat(bool up, bool down, bool left, bool right);

#endif /* HID_H */
