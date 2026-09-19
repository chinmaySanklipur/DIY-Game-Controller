/**
 * @file tusb_config.h
 * @brief TinyUSB compile-time configuration for this project.
 *
 * TinyUSB requires this header to exist on the include path; it controls
 * which USB classes are compiled in and various buffer sizes. Only the
 * HID device class is needed here — no host mode, no other device classes.
 */

#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ─────────────────────────────────────────────────────────────────────────────
 * Common Configuration
 * ───────────────────────────────────────────────────────────────────────────── */

#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU OPT_MCU_RP2040
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_PICO
#endif

/* RP2040 has two USB controllers internally multiplexed; root hub port 0
 * is the only physical port exposed on the Pico board. */
#define BOARD_TUD_RHPORT 0

/* This project is a USB device only — never a host. */
#define CFG_TUSB_RHPORT0_MODE  (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

/* ─────────────────────────────────────────────────────────────────────────────
 * Device Configuration
 * ───────────────────────────────────────────────────────────────────────────── */

#define CFG_TUD_ENDPOINT0_SIZE  64

/* Enable only the HID class — no CDC (serial), no MSC (mass storage),
 * no MIDI, no vendor class. Keeping this minimal reduces flash usage and
 * avoids the host enumerating unnecessary extra interfaces. */
#define CFG_TUD_HID    1
#define CFG_TUD_CDC    0
#define CFG_TUD_MSC    0
#define CFG_TUD_MIDI   0
#define CFG_TUD_VENDOR 0

/* HID buffer size: must be >= the largest single report we send.
 * sizeof(gamepad_report_t) is 19 bytes; 64 gives generous headroom and
 * matches the endpoint's max packet size for Full-Speed interrupt transfers. */
#define CFG_TUD_HID_EP_BUFSIZE  64

#ifdef __cplusplus
}
#endif

#endif /* TUSB_CONFIG_H */
