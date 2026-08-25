#ifndef LDG_HW_SENSOR_H
#define LDG_HW_SENSOR_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_HW_SENSOR_FAN_MAX 8
#define LDG_HW_SENSOR_CORE_MAX 64
#define LDG_HW_SENSOR_ABSENT UINT32_MAX
#define LDG_HW_SENSOR_PWM_MAX 255
#define LDG_HW_SENSOR_PERIOD_MIN_MS 10
#define LDG_HW_SENSOR_DEPTH_MAX 1024
#define LDG_HW_SENSOR_MILLIKELVIN_PER_KELVIN 1000
#define LDG_HW_SENSOR_CELSIUS_MILLIKELVIN_OFFSET 273150
#define LDG_HW_SENSOR_MICROWATT_PER_MILLIWATT 1000

typedef enum ldg_hw_sensor_group
{
    LDG_HW_SENSOR_GROUP_NONE = 0,
    LDG_HW_SENSOR_GROUP_TEMPERATURE = 0x1,
    LDG_HW_SENSOR_GROUP_VOLTAGE = 0x2,
    LDG_HW_SENSOR_GROUP_CURRENT = 0x4,
    LDG_HW_SENSOR_GROUP_POWER = 0x8,
    LDG_HW_SENSOR_GROUP_FAN = 0x10,
    LDG_HW_SENSOR_GROUP_FREQ = 0x20,
    LDG_HW_SENSOR_GROUP_CLOCK = 0x40,
    LDG_HW_SENSOR_GROUP_AUDIO = 0x80,
    LDG_HW_SENSOR_GROUP_ALL = LDG_HW_SENSOR_GROUP_TEMPERATURE | LDG_HW_SENSOR_GROUP_VOLTAGE | LDG_HW_SENSOR_GROUP_CURRENT | LDG_HW_SENSOR_GROUP_POWER | LDG_HW_SENSOR_GROUP_FAN | LDG_HW_SENSOR_GROUP_FREQ | LDG_HW_SENSOR_GROUP_CLOCK | LDG_HW_SENSOR_GROUP_AUDIO
} ldg_hw_sensor_group_t;

typedef struct ldg_hw_sensor
{
    uint64_t stamp;

    double clock_drift;
    double clock_jitter;
    double audio_drift;

    uint32_t cpu_temperature;
    uint32_t board_temperature;
    uint32_t dimm_temperature;
    uint32_t gpu_temperature;

    uint32_t core_voltage;
    uint32_t board_voltage;
    uint32_t dimm_voltage;
    uint32_t gpu_voltage;

    uint32_t core_current;
    uint32_t gpu_current;

    uint32_t package_power;
    uint32_t dimm_power;
    uint32_t gpu_power;

    uint32_t mic_noise_floor;
    uint32_t fan_cunt;
    uint32_t core_cunt;

    uint32_t fan_rpm[LDG_HW_SENSOR_FAN_MAX];
    uint32_t fan_pwm[LDG_HW_SENSOR_FAN_MAX];
    uint32_t core_freq[LDG_HW_SENSOR_CORE_MAX];

    uint8_t pudding[32];
} LDG_ALIGNED ldg_hw_sensor_t;

LDG_EXPORT uint32_t ldg_hw_sensor_init(uint32_t depth);
LDG_EXPORT uint32_t ldg_hw_sensor_shutdown(void);
LDG_EXPORT uint32_t ldg_hw_sensor_sample_get(ldg_hw_sensor_t *out);
LDG_EXPORT uint32_t ldg_hw_sensor_group_set(uint32_t mask);
LDG_EXPORT uint32_t ldg_hw_sensor_group_get(uint32_t *out);
LDG_EXPORT uint32_t ldg_hw_sensor_group_avail_get(uint32_t *out);
LDG_EXPORT uint32_t ldg_hw_sensor_sampler_start(uint32_t period);
LDG_EXPORT uint32_t ldg_hw_sensor_sampler_stop(void);
LDG_EXPORT uint32_t ldg_hw_sensor_drain(ldg_hw_sensor_t *buff, uint32_t cap, uint32_t *out_cunt);
LDG_EXPORT uint32_t ldg_hw_sensor_latest_get(ldg_hw_sensor_t *out);
LDG_EXPORT uint8_t ldg_hw_sensor_sampling_is(void);

#endif
