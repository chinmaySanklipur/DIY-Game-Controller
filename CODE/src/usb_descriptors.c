/**
 * @file usb_descriptors.c
 * @brief USB device/configuration/string descriptors required by TinyUSB.
 *
 * TinyUSB calls the tud_descriptor_*_cb() callbacks defined here whenever
 * the host requests descriptor data during enumeration. The values chosen
 * (VID/PID, device class) make the controller enumerate as a standard HID
 * gamepad recognised by Windows, macOS, and Steam without any driver.
 */

#include "tusb.h"
#include "hid.h"

/* ─────────────────────────────────────────────────────────────────────────────
 * Device Descriptor
 * ───────────────────────────────────────────────────────────────────────────── */

/**
 * VID/PID pair: 0xCAFE is TinyUSB's official "test/example" Vendor ID,
 * freely usable for hobbyist projects that are not sold commercially.
 * For a commercial product, register a real VID with the USB-IF instead.
 */
#define USB_VID   0xCAFE
#define USB_PID   0x4011  /* arbitrary product ID, distinct per project */

static const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,  /* USB 2.0 (Full-Speed operation) */

    /* bDeviceClass = 0 means "defined at interface level" — the HID class
     * is declared in the interface descriptor below, which is what lets
     * the OS's generic HID driver bind without a custom .inf/driver. */
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = 64,

    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,  /* device release number 1.00 */

    .iManufacturer      = 0x01,    /* index into string descriptor table */
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

/**
 * @brief TinyUSB callback: return the device descriptor.
 * @return Pointer to the static descriptor struct above.
 */
const uint8_t *tud_descriptor_device_cb(void)
{
    return (const uint8_t *)&desc_device;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * HID Report Descriptor passthrough
 * ───────────────────────────────────────────────────────────────────────────── */

/**
 * @brief TinyUSB callback: return our HID report descriptor (defined in hid.c).
 * @param instance  HID interface instance (always 0 — we only expose one).
 * @return          Pointer to the report descriptor byte array.
 */
const uint8_t *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return hid_report_descriptor;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Configuration Descriptor
 * ───────────────────────────────────────────────────────────────────────────── */

enum {
    ITF_NUM_HID = 0,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define EPNUM_HID         0x81  /* IN endpoint 1 */

static const uint8_t desc_configuration[] = {
    /* Configuration descriptor header */
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                           TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    /* HID interface descriptor: report descriptor length comes from hid.c,
     * polling interval is HID_REPORT_INTERVAL_MS from config.h (8 ms = 125 Hz,
     * the maximum achievable on USB Full-Speed for an interrupt endpoint). */
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE,
                        sizeof(hid_report_descriptor), EPNUM_HID,
                        CFG_TUD_HID_EP_BUFSIZE, HID_REPORT_INTERVAL_MS)
};

/**
 * @brief TinyUSB callback: return the configuration descriptor.
 * @param index  Configuration index (always 0 — we only expose one).
 * @return       Pointer to the configuration descriptor byte array.
 */
const uint8_t *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * String Descriptors
 * ───────────────────────────────────────────────────────────────────────────── */

static const char *string_desc_arr[] = {
    (const char[]){0x09, 0x04},      /* 0: language ID (English, US) */
    "Chinmay A. Sanklipur",          /* 1: manufacturer */
    "DIY Game Controller",           /* 2: product */
    "GC-2026-001",                   /* 3: serial number */
};

static uint16_t desc_str[32]; /* UTF-16 buffer, reused for each request */

/**
 * @brief TinyUSB callback: return a UTF-16 string descriptor by index.
 *
 * Converts the plain ASCII strings above into the UTF-16LE format the
 * USB string descriptor format requires, on demand.
 *
 * @param index    Which string table entry to return.
 * @param langid   Requested language ID (unused — we only support one).
 * @return         Pointer to the formatted UTF-16 descriptor, or NULL.
 */
const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
            return NULL; /* index out of range */
        }

        const char *str = string_desc_arr[index];
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31) {
            chr_count = 31; /* truncate to fit our fixed buffer */
        }

        for (uint8_t i = 0; i < chr_count; i++) {
            desc_str[1 + i] = (uint16_t)str[i]; /* naive ASCII -> UTF-16 widen */
        }
    }

    /* First word encodes total descriptor length (header + data) and type. */
    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

    return desc_str;
}
