/**
 * @file inputs.c
 * @brief Implements digital button debouncing and thumbstick ADC sampling.
 */

#include "inputs.h"
#include "config.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

/**
 * @brief One debounce counter slot per digital input.
 *
 * Indexing matches the order buttons are processed in inputs_poll(); see
 * the BTN_INDEX_* enum below. Each counter tracks how many consecutive
 * polls have seen the same raw (not-yet-debounced) GPIO level.
 */
typedef enum {
    BTN_INDEX_A, BTN_INDEX_B, BTN_INDEX_X, BTN_INDEX_Y,
    BTN_INDEX_DPAD_UP, BTN_INDEX_DPAD_DN, BTN_INDEX_DPAD_LEFT, BTN_INDEX_DPAD_RIGHT,
    BTN_INDEX_LB, BTN_INDEX_RB,
    BTN_INDEX_MENU, BTN_INDEX_VIEW, BTN_INDEX_GUIDE, BTN_INDEX_SHARE,
    BTN_INDEX_LS, BTN_INDEX_RS,
    BTN_INDEX_COUNT
} button_index_t;

/** Persistent debounce state, kept alive between calls via static storage. */
static struct {
    bool     stable_state[BTN_INDEX_COUNT];   /* last accepted (debounced) state */
    bool     last_raw[BTN_INDEX_COUNT];       /* last raw reading, for change detection */
    uint8_t  match_count[BTN_INDEX_COUNT];    /* consecutive matches of last_raw */
} debounce;

/**
 * @brief Table mapping each logical button to its GPIO pin.
 *
 * Centralising the pin list here (rather than repeating gpio_init calls
 * 16 times) keeps inputs_init() and inputs_poll() consistent automatically —
 * add a button by adding one row here.
 */
static const uint8_t button_pins[BTN_INDEX_COUNT] = {
    [BTN_INDEX_A]          = PIN_BTN_A,
    [BTN_INDEX_B]          = PIN_BTN_B,
    [BTN_INDEX_X]          = PIN_BTN_X,
    [BTN_INDEX_Y]          = PIN_BTN_Y,
    [BTN_INDEX_DPAD_UP]    = PIN_DPAD_UP,
    [BTN_INDEX_DPAD_DN]    = PIN_DPAD_DN,
    [BTN_INDEX_DPAD_LEFT]  = PIN_DPAD_LEFT,
    [BTN_INDEX_DPAD_RIGHT] = PIN_DPAD_RIGHT,
    [BTN_INDEX_LB]         = PIN_BTN_LB,
    [BTN_INDEX_RB]         = PIN_BTN_RB,
    [BTN_INDEX_MENU]       = PIN_BTN_MENU,
    [BTN_INDEX_VIEW]       = PIN_BTN_VIEW,
    [BTN_INDEX_GUIDE]      = PIN_BTN_GUIDE,
    [BTN_INDEX_SHARE]      = PIN_BTN_SHARE,
    [BTN_INDEX_LS]         = PIN_BTN_LS,
    [BTN_INDEX_RS]         = PIN_BTN_RS,
};

/**
 * @brief Debounce one digital input and return its current stable state.
 *
 * Reads the GPIO, inverts it (buttons are active-LOW), and only commits
 * the change to debounce.stable_state once DEBOUNCE_COUNT consecutive
 * polls have agreed — this rejects the brief on/off chatter a mechanical
 * switch produces during the few milliseconds its contacts physically settle.
 *
 * @param idx  Which button slot (button_index_t) to update.
 * @return     The debounced boolean state: true = pressed.
 */
static bool debounce_read(button_index_t idx)
{
    bool raw_pressed = !gpio_get(button_pins[idx]); /* active-LOW: invert */

    if (raw_pressed == debounce.last_raw[idx]) {
        if (debounce.match_count[idx] < DEBOUNCE_COUNT) {
            debounce.match_count[idx]++;
        }
    } else {
        /* Raw reading changed — restart the agreement counter */
        debounce.last_raw[idx] = raw_pressed;
        debounce.match_count[idx] = 1;
    }

    if (debounce.match_count[idx] >= DEBOUNCE_COUNT) {
        debounce.stable_state[idx] = raw_pressed;
    }

    return debounce.stable_state[idx];
}

void inputs_init(void)
{
    /* Configure every digital button pin as an input with the RP2040's
     * internal pull-up enabled. Buttons short the pin to GND when pressed,
     * so the pin reads HIGH when released and LOW when pressed. */
    for (int i = 0; i < BTN_INDEX_COUNT; i++) {
        gpio_init(button_pins[i]);
        gpio_set_dir(button_pins[i], GPIO_IN);
        gpio_pull_up(button_pins[i]);
    }

    /* ADC peripheral init for the thumbsticks. adc_init() powers up the
     * ADC block; each pin must also be individually marked for ADC use
     * via adc_gpio_init() so its digital input buffer is disabled (this
     * reduces noise and power draw on analog pins). */
    adc_init();
    adc_gpio_init(PIN_LSTICK_X);
    adc_gpio_init(PIN_LSTICK_Y);

    /* PIN_RSTICK_X / PIN_RSTICK_Y (GP10/GP11) are not native RP2040 ADC
     * pins. If the right stick is wired through an external ADC instead
     * of direct GPIO, initialise that peripheral here (e.g. spi_init()
     * for an MCP3204) rather than calling adc_gpio_init() on those pins. */
}

/**
 * @brief Read one ADC channel and return its raw 12-bit value.
 *
 * Selects the channel, then performs a single blocking conversion.
 * RP2040's ADC is 12-bit (0-4095) even though the result register is
 * wider, so we mask down to the valid range.
 *
 * @param channel  ADC input number (0-3 for GP26-GP29).
 * @return         Raw conversion result, 0-4095.
 */
static uint16_t adc_read_channel(uint8_t channel)
{
    adc_select_input(channel);
    return adc_read() & 0x0FFF;
}

void inputs_poll(inputs_state_t *state)
{
    /* ---- Digital buttons: each goes through the shared debounce filter ---- */
    state->btn_a      = debounce_read(BTN_INDEX_A);
    state->btn_b      = debounce_read(BTN_INDEX_B);
    state->btn_x      = debounce_read(BTN_INDEX_X);
    state->btn_y      = debounce_read(BTN_INDEX_Y);

    state->dpad_up    = debounce_read(BTN_INDEX_DPAD_UP);
    state->dpad_dn    = debounce_read(BTN_INDEX_DPAD_DN);
    state->dpad_left  = debounce_read(BTN_INDEX_DPAD_LEFT);
    state->dpad_right = debounce_read(BTN_INDEX_DPAD_RIGHT);

    state->btn_lb     = debounce_read(BTN_INDEX_LB);
    state->btn_rb     = debounce_read(BTN_INDEX_RB);

    state->btn_menu   = debounce_read(BTN_INDEX_MENU);
    state->btn_view   = debounce_read(BTN_INDEX_VIEW);
    state->btn_guide  = debounce_read(BTN_INDEX_GUIDE);
    state->btn_share  = debounce_read(BTN_INDEX_SHARE);

    state->btn_ls     = debounce_read(BTN_INDEX_LS);
    state->btn_rs     = debounce_read(BTN_INDEX_RS);

    /* ---- Thumbsticks: raw ADC, no debounce (analog signals don't bounce) ---- */
    state->lstick_x_raw = adc_read_channel(ADC_LSTICK_X);
    state->lstick_y_raw = adc_read_channel(ADC_LSTICK_Y);

    /* Right stick placeholder: if wired to native ADC pins instead of
     * GP10/GP11, replace this with adc_read_channel() calls using the
     * correct channel numbers. Left at mid-scale here so downstream
     * deadzone math has a defined value even before hardware-specific
     * wiring is finalised. */
    state->rstick_x_raw = 2048;
    state->rstick_y_raw = 2048;
}
