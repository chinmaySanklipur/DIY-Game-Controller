/**
 * @file main.c
 * @brief Firmware entry point: initialises every subsystem and runs the
 *        main polling/reporting loop.
 *
 * Loop structure (target: 1 kHz):
 *   1. Service the USB stack (tud_task) — must be called frequently and
 *      cannot be skipped without the host losing the connection.
 *   2. Poll all digital buttons and raw thumbstick ADC values.
 *   3. Read both triggers over I2C.
 *   4. Apply deadzone/curve shaping to convert raw values into final
 *      HID-ready signed 16-bit values.
 *   5. Build and send one HID report.
 *   6. Periodically advance the LED animation (not every loop iteration —
 *      animation only needs ~60 fps, not 1000 fps).
 *   7. Check for the remap-mode entry gesture (Menu+View+Guide held 3s).
 */

#include "pico/stdlib.h"
#include "pico/time.h"
#include "bsp/board_api.h"
#include "tusb.h"

#include "config.h"
#include "hid.h"
#include "inputs.h"
#include "triggers.h"
#include "leds.h"
#include "deadzone.h"
#include "config_store.h"

/** Loaded once at boot, mutated by the remap routine, persisted on change. */
static runtime_config_t g_config;

/**
 * @brief Build one complete gamepad_report_t from the latest sensor readings.
 *
 * Pulls together digital button states (already debounced by inputs.c),
 * thumbstick deadzone shaping, and trigger calibration mapping into the
 * exact byte layout hid.c expects to transmit.
 *
 * @param in        Latest debounced/raw input snapshot from inputs_poll().
 * @param trig_l    Raw left-trigger AS5600 angle for this cycle.
 * @param trig_r    Raw right-trigger AS5600 angle for this cycle.
 * @param[out] out  Report struct to populate.
 */
static void build_report(const inputs_state_t *in,
                          uint16_t trig_l, uint16_t trig_r,
                          gamepad_report_t *out)
{
    /* ---- Thumbsticks: circular deadzone + per-axis response curve ---- */
    deadzone_apply_stick(in->lstick_x_raw, in->lstick_y_raw,
                          g_config.curve_left, &out->lx, &out->ly);
    deadzone_apply_stick(in->rstick_x_raw, in->rstick_y_raw,
                          g_config.curve_right, &out->rx, &out->ry);

    /* ---- Triggers: calibrated range mapping ---- */
    out->lt = deadzone_apply_trigger(trig_l, g_config.trig_left_min, g_config.trig_left_max);
    out->rt = deadzone_apply_trigger(trig_r, g_config.trig_right_min, g_config.trig_right_max);

    /* ---- Buttons: pack each debounced bool into its bit position ---- */
    out->buttons = 0;
    if (in->btn_a)     out->buttons |= BTN_A_BIT;
    if (in->btn_b)     out->buttons |= BTN_B_BIT;
    if (in->btn_x)     out->buttons |= BTN_X_BIT;
    if (in->btn_y)     out->buttons |= BTN_Y_BIT;
    if (in->btn_lb)    out->buttons |= BTN_LB_BIT;
    if (in->btn_rb)    out->buttons |= BTN_RB_BIT;
    if (in->btn_menu)  out->buttons |= BTN_MENU_BIT;
    if (in->btn_view)  out->buttons |= BTN_VIEW_BIT;
    if (in->btn_guide) out->buttons |= BTN_GUIDE_BIT;
    if (in->btn_ls)    out->buttons |= BTN_LS_BIT;
    if (in->btn_rs)    out->buttons |= BTN_RS_BIT;

    out->share_btn = in->btn_share ? BTN_SHARE_BIT : 0;

    /* ---- D-pad: combine 4 booleans into one hat-switch value ---- */
    out->hat = hid_dpad_to_hat(in->dpad_up, in->dpad_dn, in->dpad_left, in->dpad_right);
}

/**
 * @brief Check whether the user is holding the remap-mode entry combo.
 *
 * Holding Menu+View+Guide together for 3 continuous seconds enters a
 * simple remap mode (full remap UI/state machine intentionally kept out
 * of this firmware skeleton — see the TODO below for where to extend it).
 *
 * @param in  Latest input snapshot.
 * @return    true once the combo has been held continuously for 3 seconds.
 */
static bool check_remap_combo_held(const inputs_state_t *in)
{
    static absolute_time_t combo_start;
    static bool combo_active = false;

    bool combo_now = in->btn_menu && in->btn_view && in->btn_guide;

    if (combo_now && !combo_active) {
        combo_active = true;
        combo_start = get_absolute_time();
    } else if (!combo_now) {
        combo_active = false;
    }

    if (combo_active && absolute_time_diff_us(combo_start, get_absolute_time()) > 3000000) {
        combo_active = false; /* one-shot trigger, require release+re-hold to fire again */
        return true;
    }
    return false;
}

int main(void)
{
    /* ---- Core SDK/board bring-up ---- */
    board_init();
    stdio_init_all();

    /* ---- USB stack: must be initialised before any tud_* call ---- */
    tusb_init();

    /* ---- Persistent config: load saved settings, or fall back to
     *      firmware defaults if this is the board's first boot ---- */
    if (config_store_init()) {
        config_load(&g_config);
    } else {
        /* Flash filesystem unavailable (hardware fault) — run with
         * in-memory defaults rather than failing to boot entirely. */
        config_set_defaults(&g_config);
    }

    /* ---- Peripheral init ---- */
    inputs_init();
    triggers_init();
    leds_init();

    /* Show a brief solid-blue confirmation that boot completed successfully
     * before settling into the normal idle animation in the main loop. */
    leds_update_animation(LED_MODE_SOLID_BLUE);

    inputs_state_t  input_state;
    gamepad_report_t report;

    absolute_time_t next_led_update = get_absolute_time();

    while (true) {
        /* tud_task() drives the entire TinyUSB state machine — handling
         * enumeration, control transfers, and endpoint servicing. It must
         * be called every loop iteration with minimal delay, or the host
         * may consider the device unresponsive. */
        tud_task();

        /* ---- 1. Poll all digital + raw analog inputs ---- */
        inputs_poll(&input_state);

        /* ---- 2. Read both triggers over I2C ---- */
        uint16_t trig_l_raw = triggers_read_left_raw();
        uint16_t trig_r_raw = triggers_read_right_raw();
        /* 0xFFFF signals a failed I2C read (sensor not responding); treat
         * as "trigger at rest" rather than propagating garbage to the OS. */
        if (trig_l_raw == 0xFFFF) trig_l_raw = g_config.trig_left_min;
        if (trig_r_raw == 0xFFFF) trig_r_raw = g_config.trig_right_min;

        /* ---- 3. Shape inputs into the final HID report ---- */
        build_report(&input_state, trig_l_raw, trig_r_raw, &report);

        /* ---- 4. Send to host ---- */
        hid_send_report(&report);

        /* ---- 5. Advance LED animation at ~60 fps, not every loop cycle ---- */
        if (absolute_time_diff_us(get_absolute_time(), next_led_update) <= 0) {
            leds_update_animation(g_config.led_mode);
            next_led_update = delayed_by_ms(get_absolute_time(), 16);
        }

        /* ---- 6. Check for the remap-mode entry gesture ----
         * TODO: a full interactive remap UI (LED cue sequence, capturing
         * the next button press per remap slot, writing the result into
         * g_config.button_remap[], then config_save()) is intentionally
         * left as a follow-up — this skeleton only detects entry. */
        if (check_remap_combo_held(&input_state)) {
            leds_update_animation(LED_MODE_RAINBOW); /* visual confirmation */
            sleep_ms(500);
        }

        /* Target ~1 kHz polling: sleep just enough to pace the loop without
         * adding perceptible input latency. USB servicing and I2C reads
         * above already consume some of this budget. */
        sleep_us(1000);
    }

    return 0; /* unreachable — kept for standards compliance */
}
