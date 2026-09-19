/**
 * @file hid.c
 * @brief Implements the HID report descriptor and report-sending helper.
 */

#include "hid.h"

/**
 * @brief Raw HID report descriptor bytes.
 *
 * This descriptor declares a Generic Desktop "Gamepad" collection with:
 *   - 6 axes (X, Y, Z, Rx, Ry, Rz reused as LX,LY,RX,RY,LT,RT), 16-bit signed
 *   - 16 buttons, packed into 2 bytes
 *   - 1 extra button byte for Share (keeps byte alignment clean)
 *   - 1 hat switch nibble + 4 bits padding
 *
 * Byte-for-byte, this must agree with gamepad_report_t in hid.h.
 */
const uint8_t hid_report_descriptor[] = {
    0x05, 0x01,             /* Usage Page (Generic Desktop)        */
    0x09, 0x05,             /* Usage (Gamepad)                     */
    0xA1, 0x01,             /* Collection (Application)            */

    /* ---- 6 signed 16-bit axes: LX, LY, RX, RY, LT, RT ---- */
    0x05, 0x01,             /*   Usage Page (Generic Desktop)      */
    0x09, 0x30,             /*   Usage (X)   -> LX                 */
    0x09, 0x31,             /*   Usage (Y)   -> LY                 */
    0x09, 0x33,             /*   Usage (Rx)  -> RX                 */
    0x09, 0x34,             /*   Usage (Ry)  -> RY                 */
    0x09, 0x32,             /*   Usage (Z)   -> LT                 */
    0x09, 0x35,             /*   Usage (Rz)  -> RT                 */
    0x16, 0x00, 0x80,       /*   Logical Minimum (-32768)          */
    0x26, 0xFF, 0x7F,       /*   Logical Maximum (32767)           */
    0x75, 0x10,             /*   Report Size (16 bits)             */
    0x95, 0x06,             /*   Report Count (6 axes)             */
    0x81, 0x02,             /*   Input (Data, Var, Abs)            */

    /* ---- 16 buttons ---- */
    0x05, 0x09,             /*   Usage Page (Button)               */
    0x19, 0x01,             /*   Usage Minimum (Button 1)          */
    0x29, 0x10,             /*   Usage Maximum (Button 16)         */
    0x15, 0x00,             /*   Logical Minimum (0)               */
    0x25, 0x01,             /*   Logical Maximum (1)               */
    0x75, 0x01,             /*   Report Size (1 bit)                */
    0x95, 0x10,             /*   Report Count (16 buttons)         */
    0x81, 0x02,             /*   Input (Data, Var, Abs)            */

    /* ---- Share button byte (1 button + 7 bits padding) ---- */
    0x05, 0x09,             /*   Usage Page (Button)               */
    0x19, 0x11,             /*   Usage Minimum (Button 17)         */
    0x29, 0x11,             /*   Usage Maximum (Button 17)         */
    0x15, 0x00,             /*   Logical Minimum (0)               */
    0x25, 0x01,             /*   Logical Maximum (1)               */
    0x75, 0x01,             /*   Report Size (1 bit)                */
    0x95, 0x01,             /*   Report Count (1 button)           */
    0x81, 0x02,             /*   Input (Data, Var, Abs)            */
    0x75, 0x07,             /*   Report Size (7 bits padding)      */
    0x95, 0x01,             /*   Report Count (1)                  */
    0x81, 0x03,             /*   Input (Const, Var, Abs) — padding */

    /* ---- Hat switch (D-pad), 4 bits + 4 bits padding ---- */
    0x05, 0x01,             /*   Usage Page (Generic Desktop)      */
    0x09, 0x39,             /*   Usage (Hat Switch)                */
    0x15, 0x00,             /*   Logical Minimum (0)               */
    0x25, 0x07,             /*   Logical Maximum (7)               */
    0x35, 0x00,             /*   Physical Minimum (0)              */
    0x46, 0x3B, 0x01,       /*   Physical Maximum (315 degrees)    */
    0x65, 0x14,             /*   Unit (Eng Rot: Degrees)           */
    0x75, 0x04,             /*   Report Size (4 bits)               */
    0x95, 0x01,             /*   Report Count (1)                  */
    0x81, 0x42,             /*   Input (Data, Var, Abs, Null State)*/
    0x75, 0x04,             /*   Report Size (4 bits padding)      */
    0x95, 0x01,             /*   Report Count (1)                  */
    0x81, 0x03,             /*   Input (Const, Var, Abs) — padding */

    0xC0                     /* End Collection                      */
};

const uint16_t hid_report_descriptor_len = sizeof(hid_report_descriptor);

void hid_send_report(const gamepad_report_t *report)
{
    /* tud_hid_ready() returns false while the host hasn't finished
     * processing the previous report yet (USB Full-Speed HID is limited
     * to 125 reports/sec). Skipping the send here rather than blocking
     * keeps the main loop running at full speed regardless of host timing. */
    if (!tud_hid_ready()) {
        return;
    }

    /* Report ID 0 = no report ID byte prefix (single-report device) */
    tud_hid_report(0, report, sizeof(gamepad_report_t));
}

uint8_t hid_dpad_to_hat(bool up, bool down, bool left, bool right)
{
    /* Resolve the 4 raw directions into one of 8 compass points.
     * Diagonal presses (e.g. up+right) map to the corresponding diagonal
     * hat value; conflicting opposite presses (up+down) are treated as
     * "neither pressed" for that axis. */
    if (up && !down) {
        if (left && !right)  return HAT_NORTHWEST;
        if (right && !left)  return HAT_NORTHEAST;
        return HAT_NORTH;
    }
    if (down && !up) {
        if (left && !right)  return HAT_SOUTHWEST;
        if (right && !left)  return HAT_SOUTHEAST;
        return HAT_SOUTH;
    }
    if (left && !right) return HAT_WEST;
    if (right && !left) return HAT_EAST;

    return HAT_CENTRE; /* nothing pressed, or conflicting opposite pair */
}
