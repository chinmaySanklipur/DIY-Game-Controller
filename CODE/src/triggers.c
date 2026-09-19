/**
 * @file triggers.c
 * @brief Implements I2C communication with the two AS5600 trigger sensors.
 */

#include "triggers.h"
#include "config.h"
#include "hardware/i2c.h"
#include "pico/time.h"

void triggers_init(void)
{
    /* Left trigger sensor: I2C0 on GP0 (SDA) / GP1 (SCL) */
    i2c_init(i2c0, I2C_SPEED_HZ);
    gpio_set_function(PIN_I2C0_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C0_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C0_SDA);
    gpio_pull_up(PIN_I2C0_SCL);

    /* Right trigger sensor: I2C1 on GP4 (SDA) / GP5 (SCL).
     * Using a second, independent I2C bus (rather than sharing I2C0)
     * is required because both AS5600 chips have the same fixed
     * address (0x36) and cannot otherwise coexist on one bus. */
    i2c_init(i2c1, I2C_SPEED_HZ);
    gpio_set_function(PIN_I2C1_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C1_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C1_SDA);
    gpio_pull_up(PIN_I2C1_SCL);
}

/**
 * @brief Read the 12-bit raw angle register pair from one AS5600 sensor.
 *
 * The AS5600 exposes its angle as two 8-bit registers (0x0C high byte,
 * 0x0D low byte) that together form a 12-bit value. We write the starting
 * register address, then read both bytes back in one transaction.
 *
 * @param i2c_inst  Which RP2040 I2C peripheral instance to use (i2c0/i2c1).
 * @return          Raw angle 0-4095, or 0xFFFF if the transaction failed.
 */
static uint16_t as5600_read_raw_angle(i2c_inst_t *i2c_inst)
{
    uint8_t reg = AS5600_REG_ANGLE_H;
    uint8_t buf[2];

    /* Point the sensor's internal register pointer at the angle register.
     * i2c_write_blocking with `nostop=true` keeps the bus held (no STOP
     * condition) so the following read continues the same transaction —
     * this matches the AS5600 datasheet's recommended read sequence. */
    int write_result = i2c_write_blocking(i2c_inst, AS5600_ADDR, &reg, 1, true);
    if (write_result < 0) {
        return 0xFFFF; /* sensor did not acknowledge — likely not present */
    }

    int read_result = i2c_read_blocking(i2c_inst, AS5600_ADDR, buf, 2, false);
    if (read_result < 0) {
        return 0xFFFF;
    }

    /* Combine high and low bytes; mask to 12 bits per the datasheet
     * (the top 4 bits of the high byte register are unused/reserved). */
    return (uint16_t)(((buf[0] << 8) | buf[1]) & 0x0FFF);
}

uint16_t triggers_read_left_raw(void)
{
    return as5600_read_raw_angle(i2c0);
}

uint16_t triggers_read_right_raw(void)
{
    return as5600_read_raw_angle(i2c1);
}

void triggers_calibrate(bool is_left, uint16_t *out_min, uint16_t *out_max)
{
    /* Simple two-stage capture: record the angle now (assumed rest
     * position, since the caller should prompt the user to release the
     * trigger before calling this), wait briefly, then prompt full-pull
     * via the caller's UI/LED feedback before sampling again.
     *
     * Real calibration UX (LED blink cues, timing) lives in main.c; this
     * function only does the two raw reads it's told to do. */
    uint16_t (*read_fn)(void) = is_left ? triggers_read_left_raw
                                         : triggers_read_right_raw;

    *out_min = read_fn();

    /* Give the user time to physically pull the trigger fully before the
     * second sample. In production this delay should be replaced with an
     * LED cue and a button-press confirmation rather than a fixed sleep. */
    sleep_ms(2000);

    *out_max = read_fn();
}
