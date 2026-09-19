/**
 * @file deadzone.c
 * @brief Implements thumbstick deadzone shaping and trigger range mapping.
 */

#include "deadzone.h"
#include "config.h"
#include <math.h>

/**
 * @brief Apply the selected response curve to a 0.0-1.0 magnitude value.
 *
 * All curves preserve the property f(0)=0 and f(1)=1 so the deadzone
 * boundary and maximum deflection both still land in the right place —
 * only the shape of the response between those two points changes.
 *
 * @param magnitude  Normalised input magnitude, 0.0 to 1.0.
 * @param curve      Which curve shape to apply.
 * @return           Shaped output magnitude, 0.0 to 1.0.
 */
static float apply_curve(float magnitude, response_curve_t curve)
{
    switch (curve) {
        case CURVE_QUADRATIC:
            /* x^2 gives finer control near the centre — useful for precise
             * aiming, since small stick movements produce even smaller
             * output movements, while full deflection is unaffected. */
            return magnitude * magnitude;

        case CURVE_SCURVE:
            /* Smoothstep-style cubic: gentle near centre AND near the
             * edges, with the steepest response in the middle of the
             * range — a common "feel" preference for racing games. */
            return (3.0f * magnitude * magnitude) - (2.0f * magnitude * magnitude * magnitude);

        case CURVE_LINEAR:
        default:
            return magnitude;
    }
}

void deadzone_apply_stick(uint16_t raw_x, uint16_t raw_y,
                           response_curve_t curve,
                           int16_t *out_x, int16_t *out_y)
{
    /* Step 1: centre the raw 12-bit ADC values (0-4095) around zero.
     * The ADC midpoint is ~2048, so subtracting that gives a signed range
     * of roughly -2048 to +2047 representing stick deflection from centre. */
    float cx = (float)raw_x - 2048.0f;
    float cy = (float)raw_y - 2048.0f;

    /* Step 2: treat (cx, cy) as a single 2D vector and compute its
     * magnitude and direction. Using a circular deadzone (rather than
     * clamping X and Y independently) avoids the "dead square" artefact
     * where diagonal movements feel less sensitive than cardinal ones. */
    float magnitude = sqrtf(cx * cx + cy * cy);
    float max_magnitude = 2048.0f; /* theoretical max distance from centre */

    if (magnitude < 1.0f) {
        /* Stick is dead-centre (or so close that direction is meaningless) —
         * avoid a divide-by-zero in the normalisation step below. */
        *out_x = 0;
        *out_y = 0;
        return;
    }

    float norm_magnitude = magnitude / max_magnitude;
    if (norm_magnitude > 1.0f) {
        norm_magnitude = 1.0f; /* clamp — raw ADC noise can slightly exceed the ideal max */
    }

    /* Step 3: apply the inner/outer deadzone thresholds from config.h.
     * Anything inside the inner radius reports zero; anything outside the
     * outer radius is clamped to full deflection; the band between them
     * is rescaled to fill the full 0.0-1.0 output range smoothly (so there
     * is no sudden jump in reported value right at the deadzone boundary). */
    float shaped_magnitude;
    if (norm_magnitude < DEADZONE_INNER_FRAC) {
        shaped_magnitude = 0.0f;
    } else if (norm_magnitude > DEADZONE_OUTER_FRAC) {
        shaped_magnitude = 1.0f;
    } else {
        shaped_magnitude = (norm_magnitude - DEADZONE_INNER_FRAC) /
                            (DEADZONE_OUTER_FRAC - DEADZONE_INNER_FRAC);
    }

    /* Step 4: apply the selected response curve to the shaped magnitude. */
    shaped_magnitude = apply_curve(shaped_magnitude, curve);

    /* Step 5: scale back up to a full vector using the ORIGINAL direction
     * (cx, cy), so the output points the same way the physical stick does,
     * just with a reshaped magnitude. */
    float scale = (magnitude > 0.0f) ? (shaped_magnitude * 32767.0f / magnitude) : 0.0f;

    float final_x = cx * scale;
    float final_y = cy * scale;

    /* Clamp to int16 range defensively — floating point rounding could in
     * principle push a value 1 unit past +-32767. */
    if (final_x > 32767.0f)  final_x = 32767.0f;
    if (final_x < -32768.0f) final_x = -32768.0f;
    if (final_y > 32767.0f)  final_y = 32767.0f;
    if (final_y < -32768.0f) final_y = -32768.0f;

    *out_x = (int16_t)final_x;
    *out_y = (int16_t)final_y;
}

int16_t deadzone_apply_trigger(uint16_t raw_angle, uint16_t cal_min, uint16_t cal_max)
{
    /* Defensive guard: if calibration hasn't been performed yet (min==max,
     * or max < min from a bad calibration run), avoid a divide-by-zero and
     * just report zero rather than producing garbage output. */
    if (cal_max <= cal_min) {
        return 0;
    }

    /* Normalise the raw angle into the calibrated 0.0-1.0 travel range. */
    float range = (float)(cal_max - cal_min);
    float position = ((float)raw_angle - (float)cal_min) / range;

    if (position < 0.0f) position = 0.0f; /* trigger angle below rest calibration point */
    if (position > 1.0f) position = 1.0f; /* trigger angle beyond full-pull calibration point */

    /* Apply the trigger's own inner/outer flat zones, same rationale as
     * the thumbstick deadzone: avoids chatter at rest, guarantees a true
     * 100% reading is reachable even with slight mechanical variance. */
    float shaped;
    if (position < TRIGGER_INNER_FRAC) {
        shaped = 0.0f;
    } else if (position > TRIGGER_OUTER_FRAC) {
        shaped = 1.0f;
    } else {
        shaped = (position - TRIGGER_INNER_FRAC) /
                 (TRIGGER_OUTER_FRAC - TRIGGER_INNER_FRAC);
    }

    return (int16_t)(shaped * 32767.0f);
}
