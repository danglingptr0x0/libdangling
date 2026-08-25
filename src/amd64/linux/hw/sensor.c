#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/timex.h>

#include <dangling/hw/sensor.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>
#include <dangling/time/time.h>
#include <dangling/thread/spsc.h>
#include <dangling/thread/sync.h>
#include <dangling/arch/amd64/syscall.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/arch/amd64/fence.h>

#define LDG_HW_SENSOR_PATH_CAP 256
#define LDG_HW_SENSOR_BUFF_CAP 64
#define LDG_HW_SENSOR_LABEL_CAP 64
#define LDG_HW_SENSOR_CHIP_CAP 48
#define LDG_HW_SENSOR_HWMON_MAX 16
#define LDG_HW_SENSOR_INPUT_MAX 24
#define LDG_HW_SENSOR_ZONE_MAX 16
#define LDG_HW_SENSOR_RAPL_MAX 4
#define LDG_HW_SENSOR_IDX_FIRST 0
#define LDG_HW_SENSOR_INPUT_FIRST 1
#define LDG_HW_SENSOR_STEP 1
#define LDG_HW_SENSOR_DECIMAL_BASE 10
#define LDG_HW_SENSOR_DIGIT_BASE '0'
#define LDG_HW_SENSOR_DIGIT_LAST '9'
#define LDG_HW_SENSOR_NEWLINE '\n'
#define LDG_HW_SENSOR_TERM '\0'
#define LDG_HW_SENSOR_SPACE ' '
#define LDG_HW_SENSOR_TAB '\t'
#define LDG_HW_SENSOR_MINUS '-'
#define LDG_HW_SENSOR_PLUS '+'
#define LDG_HW_SENSOR_UPPER_MIN 'A'
#define LDG_HW_SENSOR_UPPER_MAX 'Z'
#define LDG_HW_SENSOR_CASE_SHIFT 32
#define LDG_HW_SENSOR_JITTER_SAMPLE_MAX 32
#define LDG_HW_SENSOR_JITTER_SPAN_NS 20000
#define LDG_HW_SENSOR_PPB_SCALE 1000000000.0
#define LDG_HW_SENSOR_PPM_SCALE 1000000.0
#define LDG_HW_SENSOR_TIMEX_FREQ_SHIFT 65536.0
#define LDG_HW_SENSOR_MIN_KELVIN_MILLIDEG (-273150)
#define LDG_HW_SENSOR_JOIN_RETRY_MS 5

#define LDG_HW_SENSOR_PATH_HWMON "/sys/class/hwmon"
#define LDG_HW_SENSOR_PATH_THERMAL "/sys/class/thermal"
#define LDG_HW_SENSOR_PATH_CPUFREQ "/sys/devices/system/cpu"
#define LDG_HW_SENSOR_PATH_RAPL "/sys/class/powercap"

#define LDG_HW_SENSOR_FMT_CHIP "%s/hwmon%u/name"
#define LDG_HW_SENSOR_FMT_TEMP "%s/hwmon%u/temp%u_input"
#define LDG_HW_SENSOR_FMT_TEMP_LABEL "%s/hwmon%u/temp%u_label"
#define LDG_HW_SENSOR_FMT_VOLTAGE "%s/hwmon%u/in%u_input"
#define LDG_HW_SENSOR_FMT_VOLTAGE_LABEL "%s/hwmon%u/in%u_label"
#define LDG_HW_SENSOR_FMT_CURRENT "%s/hwmon%u/curr%u_input"
#define LDG_HW_SENSOR_FMT_CURRENT_LABEL "%s/hwmon%u/curr%u_label"
#define LDG_HW_SENSOR_FMT_POWER "%s/hwmon%u/power%u_input"
#define LDG_HW_SENSOR_FMT_FAN "%s/hwmon%u/fan%u_input"
#define LDG_HW_SENSOR_FMT_PWM "%s/hwmon%u/pwm%u"
#define LDG_HW_SENSOR_FMT_ZONE "%s/thermal_zone%u/temp"
#define LDG_HW_SENSOR_FMT_ZONE_KIND "%s/thermal_zone%u/type"
#define LDG_HW_SENSOR_FMT_FREQ "%s/cpu%u/cpufreq/scaling_cur_freq"
#define LDG_HW_SENSOR_FMT_RAPL_NAME "%s/intel-rapl/intel-rapl:%u/name"
#define LDG_HW_SENSOR_FMT_RAPL_POWER "%s/intel-rapl/intel-rapl:%u/power_uw"

#define LDG_HW_SENSOR_NEEDLE_CORE "core"
#define LDG_HW_SENSOR_NEEDLE_PACKAGE "package"
#define LDG_HW_SENSOR_NEEDLE_TDIE "tdie"
#define LDG_HW_SENSOR_NEEDLE_TCTL "tctl"
#define LDG_HW_SENSOR_NEEDLE_VCORE "vcore"
#define LDG_HW_SENSOR_NEEDLE_VDDGFX "vddgfx"
#define LDG_HW_SENSOR_NEEDLE_DIMM "dimm"
#define LDG_HW_SENSOR_NEEDLE_DRAM "dram"
#define LDG_HW_SENSOR_NEEDLE_VDDQ "vddq"
#define LDG_HW_SENSOR_NEEDLE_SYSTIN "systin"
#define LDG_HW_SENSOR_NEEDLE_AMBIENT "ambient"
#define LDG_HW_SENSOR_NEEDLE_CORETEMP "coretemp"
#define LDG_HW_SENSOR_NEEDLE_K10TEMP "k10temp"
#define LDG_HW_SENSOR_NEEDLE_ZENPOWER "zenpower"
#define LDG_HW_SENSOR_NEEDLE_AMDGPU "amdgpu"
#define LDG_HW_SENSOR_NEEDLE_RADEON "radeon"
#define LDG_HW_SENSOR_NEEDLE_NOUVEAU "nouveau"
#define LDG_HW_SENSOR_NEEDLE_NVIDIA "nvidia"
#define LDG_HW_SENSOR_NEEDLE_ACPITZ "acpitz"

typedef enum hw_sensor_chip
{
    HW_SENSOR_CHIP_OTHER = 0,
    HW_SENSOR_CHIP_CPU,
    HW_SENSOR_CHIP_GPU,
    HW_SENSOR_CHIP_BOARD
} hw_sensor_chip_t;

typedef struct hw_sensor_ctx
{
    ldg_spsc_queue_t ring;
    ldg_hw_sensor_t latest;
    ldg_mut_t lock;
    uint64_t thread;
    uint32_t period;
    uint32_t mask;
    uint32_t avail;
    uint32_t depth;
    volatile uint8_t running;
    uint8_t is_init;
    uint8_t has_latest;
    uint8_t pudding[5];
} LDG_ALIGNED hw_sensor_ctx_t;

hw_sensor_ctx_t g_hw_sensor = LDG_STRUCT_ZERO_INIT;

static char hw_sensor_lower_get(char raw)
{
    char res = raw;

    if (raw >= LDG_HW_SENSOR_UPPER_MIN && raw <= LDG_HW_SENSOR_UPPER_MAX) { res = (char)(raw + LDG_HW_SENSOR_CASE_SHIFT); }

    return res;
}

static uint8_t hw_sensor_label_has(const char *label, const char *needle)
{
    uint64_t idx = 0;
    uint64_t sub = 0;
    uint8_t res = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!label || !needle)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (label[idx] != LDG_HW_SENSOR_TERM)
    {
        sub = 0;

        while (needle[sub] != LDG_HW_SENSOR_TERM && hw_sensor_lower_get(label[idx + sub]) == hw_sensor_lower_get(needle[sub])) { sub++; }

        if (needle[sub] == LDG_HW_SENSOR_TERM)
        {
            res = LDG_TRUTH_TRUE;
            break;
        }

        idx++;
    }

    return res;
}

static uint32_t hw_sensor_sysfs_rd(const char *path, char *buff, uint64_t cap)
{
    int32_t fd = 0;
    int64_t got = 0;
    uint64_t len = 0;

    if (LDG_UNLIKELY(!path || !buff || cap == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    buff[LDG_HW_SENSOR_IDX_FIRST] = LDG_HW_SENSOR_TERM;

    fd = open(path, O_RDONLY);
    if (LDG_UNLIKELY(fd < 0)) { return LDG_ERR_IO_OPEN; }

    got = read(fd, buff, cap - LDG_HW_SENSOR_STEP);

    if (LDG_UNLIKELY(close(fd) != 0)) { return LDG_ERR_IO_CLOSE; }

    if (LDG_UNLIKELY(got < 0)) { return LDG_ERR_IO_RD; }

    len = (uint64_t)got;
    buff[len] = LDG_HW_SENSOR_TERM;

    while (len > 0 && (buff[len - LDG_HW_SENSOR_STEP] == LDG_HW_SENSOR_NEWLINE || buff[len - LDG_HW_SENSOR_STEP] == LDG_HW_SENSOR_SPACE))
    {
        len--;
        buff[len] = LDG_HW_SENSOR_TERM;
    }

    return LDG_ERR_AOK;
}

static uint32_t hw_sensor_sysfs_int_rd(const char *path, int64_t *out)
{
    char buff[LDG_HW_SENSOR_BUFF_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;
    int64_t val = 0;
    uint64_t idx = 0;
    uint8_t neg = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!path || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = hw_sensor_sysfs_rd(path, buff, sizeof(buff));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (buff[idx] == LDG_HW_SENSOR_SPACE || buff[idx] == LDG_HW_SENSOR_TAB) { idx++; }

    if (buff[idx] == LDG_HW_SENSOR_MINUS)
    {
        neg = LDG_TRUTH_TRUE;
        idx++;
    }
    else if (buff[idx] == LDG_HW_SENSOR_PLUS) { idx++; }

    if (LDG_UNLIKELY(buff[idx] < LDG_HW_SENSOR_DIGIT_BASE || buff[idx] > LDG_HW_SENSOR_DIGIT_LAST)) { return LDG_ERR_IO_FMT; }

    while (buff[idx] >= LDG_HW_SENSOR_DIGIT_BASE && buff[idx] <= LDG_HW_SENSOR_DIGIT_LAST)
    {
        val = val * LDG_HW_SENSOR_DECIMAL_BASE + (int64_t)(buff[idx] - LDG_HW_SENSOR_DIGIT_BASE);
        idx++;
    }

    *out = (neg == LDG_TRUTH_TRUE) ? -val : val;

    return LDG_ERR_AOK;
}

static uint32_t hw_sensor_millikelvin_get(int64_t millideg, uint32_t *out)
{
    int64_t shifted = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_HW_SENSOR_ABSENT;

    if (LDG_UNLIKELY(millideg < LDG_HW_SENSOR_MIN_KELVIN_MILLIDEG)) { return LDG_ERR_RANGE; }

    shifted = millideg + LDG_HW_SENSOR_CELSIUS_MILLIKELVIN_OFFSET;

    if (LDG_UNLIKELY(shifted > (int64_t)UINT32_MAX)) { return LDG_ERR_RANGE; }

    *out = (uint32_t)shifted;

    return LDG_ERR_AOK;
}

static hw_sensor_chip_t hw_sensor_chip_classify(const char *chip)
{
    if (hw_sensor_label_has(chip, LDG_HW_SENSOR_NEEDLE_CORETEMP) == LDG_TRUTH_TRUE) { return HW_SENSOR_CHIP_CPU; }

    if (hw_sensor_label_has(chip, LDG_HW_SENSOR_NEEDLE_K10TEMP) == LDG_TRUTH_TRUE) { return HW_SENSOR_CHIP_CPU; }

    if (hw_sensor_label_has(chip, LDG_HW_SENSOR_NEEDLE_ZENPOWER) == LDG_TRUTH_TRUE) { return HW_SENSOR_CHIP_CPU; }

    if (hw_sensor_label_has(chip, LDG_HW_SENSOR_NEEDLE_AMDGPU) == LDG_TRUTH_TRUE) { return HW_SENSOR_CHIP_GPU; }

    if (hw_sensor_label_has(chip, LDG_HW_SENSOR_NEEDLE_RADEON) == LDG_TRUTH_TRUE) { return HW_SENSOR_CHIP_GPU; }

    if (hw_sensor_label_has(chip, LDG_HW_SENSOR_NEEDLE_NOUVEAU) == LDG_TRUTH_TRUE) { return HW_SENSOR_CHIP_GPU; }

    if (hw_sensor_label_has(chip, LDG_HW_SENSOR_NEEDLE_NVIDIA) == LDG_TRUTH_TRUE) { return HW_SENSOR_CHIP_GPU; }

    if (hw_sensor_label_has(chip, LDG_HW_SENSOR_NEEDLE_ACPITZ) == LDG_TRUTH_TRUE) { return HW_SENSOR_CHIP_BOARD; }

    return HW_SENSOR_CHIP_BOARD;
}

static void hw_sensor_temperature_place(ldg_hw_sensor_t *out, hw_sensor_chip_t chip, const char *label, uint32_t millikelvin)
{
    if (hw_sensor_label_has(label, LDG_HW_SENSOR_NEEDLE_DIMM) == LDG_TRUTH_TRUE)
    {
        if (out->dimm_temperature == LDG_HW_SENSOR_ABSENT) { out->dimm_temperature = millikelvin; }

        return;
    }

    if (chip == HW_SENSOR_CHIP_GPU)
    {
        if (out->gpu_temperature == LDG_HW_SENSOR_ABSENT) { out->gpu_temperature = millikelvin; }

        return;
    }

    if (chip == HW_SENSOR_CHIP_CPU)
    {
        if (out->cpu_temperature == LDG_HW_SENSOR_ABSENT) { out->cpu_temperature = millikelvin; }

        return;
    }

    if (hw_sensor_label_has(label, LDG_HW_SENSOR_NEEDLE_PACKAGE) == LDG_TRUTH_TRUE || hw_sensor_label_has(label, LDG_HW_SENSOR_NEEDLE_TDIE) == LDG_TRUTH_TRUE || hw_sensor_label_has(label, LDG_HW_SENSOR_NEEDLE_TCTL) == LDG_TRUTH_TRUE)
    {
        if (out->cpu_temperature == LDG_HW_SENSOR_ABSENT) { out->cpu_temperature = millikelvin; }

        return;
    }

    if (out->board_temperature == LDG_HW_SENSOR_ABSENT) { out->board_temperature = millikelvin; }
}

static void hw_sensor_voltage_place(ldg_hw_sensor_t *out, hw_sensor_chip_t chip, const char *label, uint32_t millivolt)
{
    if (hw_sensor_label_has(label, LDG_HW_SENSOR_NEEDLE_VCORE) == LDG_TRUTH_TRUE)
    {
        if (out->core_voltage == LDG_HW_SENSOR_ABSENT) { out->core_voltage = millivolt; }

        return;
    }

    if (hw_sensor_label_has(label, LDG_HW_SENSOR_NEEDLE_VDDGFX) == LDG_TRUTH_TRUE || chip == HW_SENSOR_CHIP_GPU)
    {
        if (out->gpu_voltage == LDG_HW_SENSOR_ABSENT) { out->gpu_voltage = millivolt; }

        return;
    }

    if (hw_sensor_label_has(label, LDG_HW_SENSOR_NEEDLE_DIMM) == LDG_TRUTH_TRUE || hw_sensor_label_has(label, LDG_HW_SENSOR_NEEDLE_DRAM) == LDG_TRUTH_TRUE || hw_sensor_label_has(label, LDG_HW_SENSOR_NEEDLE_VDDQ) == LDG_TRUTH_TRUE)
    {
        if (out->dimm_voltage == LDG_HW_SENSOR_ABSENT) { out->dimm_voltage = millivolt; }

        return;
    }

    if (out->board_voltage == LDG_HW_SENSOR_ABSENT) { out->board_voltage = millivolt; }
}

static LDG_NOINLINE void hw_sensor_hwmon_collect(ldg_hw_sensor_t *out, uint32_t mask)
{
    char path[LDG_HW_SENSOR_PATH_CAP] = LDG_ARR_ZERO_INIT;
    char chip_name[LDG_HW_SENSOR_CHIP_CAP] = LDG_ARR_ZERO_INIT;
    char label[LDG_HW_SENSOR_LABEL_CAP] = LDG_ARR_ZERO_INIT;
    hw_sensor_chip_t chip = HW_SENSOR_CHIP_OTHER;
    int64_t raw = 0;
    uint32_t hwmon_idx = 0;
    uint32_t input_idx = 0;
    uint32_t converted = 0;

    for (hwmon_idx = LDG_HW_SENSOR_IDX_FIRST; hwmon_idx < LDG_HW_SENSOR_HWMON_MAX; hwmon_idx++)
    {
        if (snprintf(path, sizeof(path), LDG_HW_SENSOR_FMT_CHIP, LDG_HW_SENSOR_PATH_HWMON, hwmon_idx) < 0) { continue; }

        if (hw_sensor_sysfs_rd(path, chip_name, sizeof(chip_name)) != LDG_ERR_AOK) { continue; }

        chip = hw_sensor_chip_classify(chip_name);

        for (input_idx = LDG_HW_SENSOR_INPUT_FIRST; input_idx < LDG_HW_SENSOR_INPUT_MAX; input_idx++)
        {
            if ((mask & (uint32_t)LDG_HW_SENSOR_GROUP_TEMPERATURE) != 0) { if (snprintf(path, sizeof(path), LDG_HW_SENSOR_FMT_TEMP, LDG_HW_SENSOR_PATH_HWMON, hwmon_idx, input_idx) > 0 && hw_sensor_sysfs_int_rd(path, &raw) == LDG_ERR_AOK)
                {
                    label[LDG_HW_SENSOR_IDX_FIRST] = LDG_HW_SENSOR_TERM;
                    if (snprintf(path, sizeof(path), LDG_HW_SENSOR_FMT_TEMP_LABEL, LDG_HW_SENSOR_PATH_HWMON, hwmon_idx, input_idx) > 0) { hw_sensor_sysfs_rd(path, label, sizeof(label)); }

                    if (hw_sensor_millikelvin_get(raw, &converted) == LDG_ERR_AOK) { hw_sensor_temperature_place(out, chip, label, converted); }
                }
            }

            if ((mask & (uint32_t)LDG_HW_SENSOR_GROUP_VOLTAGE) != 0) { if (snprintf(path, sizeof(path), LDG_HW_SENSOR_FMT_VOLTAGE, LDG_HW_SENSOR_PATH_HWMON, hwmon_idx, input_idx) > 0 && hw_sensor_sysfs_int_rd(path, &raw) == LDG_ERR_AOK && raw >= 0)
                {
                    label[LDG_HW_SENSOR_IDX_FIRST] = LDG_HW_SENSOR_TERM;
                    if (snprintf(path, sizeof(path), LDG_HW_SENSOR_FMT_VOLTAGE_LABEL, LDG_HW_SENSOR_PATH_HWMON, hwmon_idx, input_idx) > 0) { hw_sensor_sysfs_rd(path, label, sizeof(label)); }

                    hw_sensor_voltage_place(out, chip, label, (uint32_t)raw);
                }
            }

            if ((mask & (uint32_t)LDG_HW_SENSOR_GROUP_CURRENT) != 0) { if (snprintf(path, sizeof(path), LDG_HW_SENSOR_FMT_CURRENT, LDG_HW_SENSOR_PATH_HWMON, hwmon_idx, input_idx) > 0 && hw_sensor_sysfs_int_rd(path, &raw) == LDG_ERR_AOK && raw >= 0) { if (chip == HW_SENSOR_CHIP_GPU) { if (out->gpu_current == LDG_HW_SENSOR_ABSENT) { out->gpu_current = (uint32_t)raw; } }
                    else if (out->core_current == LDG_HW_SENSOR_ABSENT) { out->core_current = (uint32_t)raw; } } }

            if ((mask & (uint32_t)LDG_HW_SENSOR_GROUP_POWER) != 0) { if (snprintf(path, sizeof(path), LDG_HW_SENSOR_FMT_POWER, LDG_HW_SENSOR_PATH_HWMON, hwmon_idx, input_idx) > 0 && hw_sensor_sysfs_int_rd(path, &raw) == LDG_ERR_AOK && raw >= 0)
                {
                    converted = (uint32_t)(raw / LDG_HW_SENSOR_MICROWATT_PER_MILLIWATT);
                    if (chip == HW_SENSOR_CHIP_GPU) { if (out->gpu_power == LDG_HW_SENSOR_ABSENT) { out->gpu_power = converted; } }
                    else if (out->package_power == LDG_HW_SENSOR_ABSENT) { out->package_power = converted; }
                }
            }

            if ((mask & (uint32_t)LDG_HW_SENSOR_GROUP_FAN) != 0 && out->fan_cunt < LDG_HW_SENSOR_FAN_MAX) { if (snprintf(path, sizeof(path), LDG_HW_SENSOR_FMT_FAN, LDG_HW_SENSOR_PATH_HWMON, hwmon_idx, input_idx) > 0 && hw_sensor_sysfs_int_rd(path, &raw) == LDG_ERR_AOK && raw >= 0)
                {
                    out->fan_rpm[out->fan_cunt] = (uint32_t)raw;
                    if (snprintf(path, sizeof(path), LDG_HW_SENSOR_FMT_PWM, LDG_HW_SENSOR_PATH_HWMON, hwmon_idx, input_idx) > 0 && hw_sensor_sysfs_int_rd(path, &raw) == LDG_ERR_AOK && raw >= 0) { out->fan_pwm[out->fan_cunt] = (uint32_t)raw; }

                    out->fan_cunt++;
                }
            }
        }
    }
}

static LDG_NOINLINE void hw_sensor_thermal_collect(ldg_hw_sensor_t *out)
{
    char path[LDG_HW_SENSOR_PATH_CAP] = LDG_ARR_ZERO_INIT;
    char kind[LDG_HW_SENSOR_LABEL_CAP] = LDG_ARR_ZERO_INIT;
    int64_t raw = 0;
    uint32_t zone_idx = 0;
    uint32_t converted = 0;

    for (zone_idx = LDG_HW_SENSOR_IDX_FIRST; zone_idx < LDG_HW_SENSOR_ZONE_MAX; zone_idx++)
    {
        if (snprintf(path, sizeof(path), LDG_HW_SENSOR_FMT_ZONE, LDG_HW_SENSOR_PATH_THERMAL, zone_idx) < 0) { continue; }

        if (hw_sensor_sysfs_int_rd(path, &raw) != LDG_ERR_AOK) { continue; }

        if (hw_sensor_millikelvin_get(raw, &converted) != LDG_ERR_AOK) { continue; }

        kind[LDG_HW_SENSOR_IDX_FIRST] = LDG_HW_SENSOR_TERM;

        if (snprintf(path, sizeof(path), LDG_HW_SENSOR_FMT_ZONE_KIND, LDG_HW_SENSOR_PATH_THERMAL, zone_idx) > 0) { hw_sensor_sysfs_rd(path, kind, sizeof(kind)); }

        if (hw_sensor_label_has(kind, LDG_HW_SENSOR_NEEDLE_CORE) == LDG_TRUTH_TRUE || hw_sensor_label_has(kind, LDG_HW_SENSOR_NEEDLE_PACKAGE) == LDG_TRUTH_TRUE) { if (out->cpu_temperature == LDG_HW_SENSOR_ABSENT) { out->cpu_temperature = converted; } }
        else if (out->board_temperature == LDG_HW_SENSOR_ABSENT) { out->board_temperature = converted; }
    }
}

static LDG_NOINLINE void hw_sensor_freq_collect(ldg_hw_sensor_t *out)
{
    char path[LDG_HW_SENSOR_PATH_CAP] = LDG_ARR_ZERO_INIT;
    int64_t raw = 0;
    uint32_t core_idx = 0;

    for (core_idx = LDG_HW_SENSOR_IDX_FIRST; core_idx < LDG_HW_SENSOR_CORE_MAX; core_idx++)
    {
        if (snprintf(path, sizeof(path), LDG_HW_SENSOR_FMT_FREQ, LDG_HW_SENSOR_PATH_CPUFREQ, core_idx) < 0) { break; }

        if (hw_sensor_sysfs_int_rd(path, &raw) != LDG_ERR_AOK) { break; }

        if (raw < 0) { break; }

        out->core_freq[core_idx] = (uint32_t)raw;
        out->core_cunt++;
    }
}

static LDG_NOINLINE void hw_sensor_rapl_collect(ldg_hw_sensor_t *out)
{
    char path[LDG_HW_SENSOR_PATH_CAP] = LDG_ARR_ZERO_INIT;
    char name[LDG_HW_SENSOR_LABEL_CAP] = LDG_ARR_ZERO_INIT;
    int64_t raw = 0;
    uint32_t rapl_idx = 0;

    for (rapl_idx = LDG_HW_SENSOR_IDX_FIRST; rapl_idx < LDG_HW_SENSOR_RAPL_MAX; rapl_idx++)
    {
        if (snprintf(path, sizeof(path), LDG_HW_SENSOR_FMT_RAPL_NAME, LDG_HW_SENSOR_PATH_RAPL, rapl_idx) < 0) { continue; }

        if (hw_sensor_sysfs_rd(path, name, sizeof(name)) != LDG_ERR_AOK) { continue; }

        if (snprintf(path, sizeof(path), LDG_HW_SENSOR_FMT_RAPL_POWER, LDG_HW_SENSOR_PATH_RAPL, rapl_idx) < 0) { continue; }

        if (hw_sensor_sysfs_int_rd(path, &raw) != LDG_ERR_AOK || raw < 0) { continue; }

        if (hw_sensor_label_has(name, LDG_HW_SENSOR_NEEDLE_DRAM) == LDG_TRUTH_TRUE) { if (out->dimm_power == LDG_HW_SENSOR_ABSENT) { out->dimm_power = (uint32_t)(raw / LDG_HW_SENSOR_MICROWATT_PER_MILLIWATT); } }
        else if (out->package_power == LDG_HW_SENSOR_ABSENT) { out->package_power = (uint32_t)(raw / LDG_HW_SENSOR_MICROWATT_PER_MILLIWATT); }
    }
}

static LDG_NOINLINE void hw_sensor_clock_collect(ldg_hw_sensor_t *out)
{
    struct timex trim = LDG_STRUCT_ZERO_INIT;
    struct timespec spec = LDG_STRUCT_ZERO_INIT;
    uint64_t tsc_open = 0;
    uint64_t tsc_close = 0;
    uint64_t mono_open = 0;
    uint64_t mono_close = 0;
    double ratio[LDG_HW_SENSOR_JITTER_SAMPLE_MAX] = LDG_ARR_ZERO_INIT;
    double mean = 0.0;
    double variance = 0.0;
    double diff = 0.0;
    double tsc_span = 0.0;
    double mono_span = 0.0;
    uint32_t idx = 0;
    uint32_t taken = 0;

    if (adjtimex(&trim) >= 0) { out->clock_drift = (double)trim.freq / LDG_HW_SENSOR_TIMEX_FREQ_SHIFT; }

    for (idx = LDG_HW_SENSOR_IDX_FIRST; idx < LDG_HW_SENSOR_JITTER_SAMPLE_MAX; idx++)
    {
        tsc_open = ldg_rdtsc();

        if (clock_gettime(CLOCK_MONOTONIC, &spec) != 0) { break; }

        mono_open = (uint64_t)spec.tv_sec * LDG_NS_PER_SEC + (uint64_t)spec.tv_nsec;

        LDG_BARRIER;

        do
        {
            if (clock_gettime(CLOCK_MONOTONIC, &spec) != 0) { break; }

            mono_close = (uint64_t)spec.tv_sec * LDG_NS_PER_SEC + (uint64_t)spec.tv_nsec;
        }
        while (mono_close > mono_open && mono_close - mono_open < LDG_HW_SENSOR_JITTER_SPAN_NS);

        tsc_close = ldg_rdtsc();

        if (mono_close <= mono_open) { continue; }

        if (mono_close - mono_open < LDG_HW_SENSOR_JITTER_SPAN_NS) { continue; }

        tsc_span = (double)(tsc_close - tsc_open);
        mono_span = (double)(mono_close - mono_open);

        ratio[taken] = tsc_span / mono_span;
        mean += ratio[taken];
        taken++;
    }

    if (taken == 0) { return; }

    mean = mean / (double)taken;

    if (mean <= 0.0) { return; }

    for (idx = LDG_HW_SENSOR_IDX_FIRST; idx < taken; idx++)
    {
        diff = ratio[idx] - mean;
        variance += diff * diff;
    }

    variance = variance / (double)taken;

    out->clock_jitter = __builtin_sqrt(variance) / mean * LDG_HW_SENSOR_PPB_SCALE;
}

static void hw_sensor_absent_set(ldg_hw_sensor_t *out)
{
    uint32_t idx = 0;

    out->stamp = 0;
    out->clock_drift = 0.0;
    out->clock_jitter = 0.0;
    out->audio_drift = 0.0;

    out->cpu_temperature = LDG_HW_SENSOR_ABSENT;
    out->board_temperature = LDG_HW_SENSOR_ABSENT;
    out->dimm_temperature = LDG_HW_SENSOR_ABSENT;
    out->gpu_temperature = LDG_HW_SENSOR_ABSENT;

    out->core_voltage = LDG_HW_SENSOR_ABSENT;
    out->board_voltage = LDG_HW_SENSOR_ABSENT;
    out->dimm_voltage = LDG_HW_SENSOR_ABSENT;
    out->gpu_voltage = LDG_HW_SENSOR_ABSENT;

    out->core_current = LDG_HW_SENSOR_ABSENT;
    out->gpu_current = LDG_HW_SENSOR_ABSENT;

    out->package_power = LDG_HW_SENSOR_ABSENT;
    out->dimm_power = LDG_HW_SENSOR_ABSENT;
    out->gpu_power = LDG_HW_SENSOR_ABSENT;

    out->mic_noise_floor = LDG_HW_SENSOR_ABSENT;
    out->fan_cunt = 0;
    out->core_cunt = 0;

    for (idx = LDG_HW_SENSOR_IDX_FIRST; idx < LDG_HW_SENSOR_FAN_MAX; idx++)
    {
        out->fan_rpm[idx] = LDG_HW_SENSOR_ABSENT;
        out->fan_pwm[idx] = LDG_HW_SENSOR_ABSENT;
    }

    for (idx = LDG_HW_SENSOR_IDX_FIRST; idx < LDG_HW_SENSOR_CORE_MAX; idx++) { out->core_freq[idx] = LDG_HW_SENSOR_ABSENT; }
}

static uint32_t hw_sensor_avail_derive(const ldg_hw_sensor_t *sample)
{
    uint32_t avail = LDG_HW_SENSOR_GROUP_NONE;

    if (sample->cpu_temperature != LDG_HW_SENSOR_ABSENT || sample->board_temperature != LDG_HW_SENSOR_ABSENT || sample->dimm_temperature != LDG_HW_SENSOR_ABSENT || sample->gpu_temperature != LDG_HW_SENSOR_ABSENT) { avail |= (uint32_t)LDG_HW_SENSOR_GROUP_TEMPERATURE; }

    if (sample->core_voltage != LDG_HW_SENSOR_ABSENT || sample->board_voltage != LDG_HW_SENSOR_ABSENT || sample->dimm_voltage != LDG_HW_SENSOR_ABSENT || sample->gpu_voltage != LDG_HW_SENSOR_ABSENT) { avail |= (uint32_t)LDG_HW_SENSOR_GROUP_VOLTAGE; }

    if (sample->core_current != LDG_HW_SENSOR_ABSENT || sample->gpu_current != LDG_HW_SENSOR_ABSENT) { avail |= (uint32_t)LDG_HW_SENSOR_GROUP_CURRENT; }

    if (sample->package_power != LDG_HW_SENSOR_ABSENT || sample->dimm_power != LDG_HW_SENSOR_ABSENT || sample->gpu_power != LDG_HW_SENSOR_ABSENT) { avail |= (uint32_t)LDG_HW_SENSOR_GROUP_POWER; }

    if (sample->fan_cunt > 0) { avail |= (uint32_t)LDG_HW_SENSOR_GROUP_FAN; }

    if (sample->core_cunt > 0) { avail |= (uint32_t)LDG_HW_SENSOR_GROUP_FREQ; }

    if (sample->clock_jitter > 0.0) { avail |= (uint32_t)LDG_HW_SENSOR_GROUP_CLOCK; }

    if (sample->mic_noise_floor != LDG_HW_SENSOR_ABSENT) { avail |= (uint32_t)LDG_HW_SENSOR_GROUP_AUDIO; }

    return avail;
}

uint32_t ldg_hw_sensor_sample_get(ldg_hw_sensor_t *out)
{
    uint32_t mask = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    hw_sensor_absent_set(out);

    mask = LDG_RD_ONCE(g_hw_sensor.mask);

    if (mask == LDG_HW_SENSOR_GROUP_NONE) { mask = (uint32_t)LDG_HW_SENSOR_GROUP_ALL; }

    out->stamp = ldg_time_epoch_ns_get();

    hw_sensor_hwmon_collect(out, mask);

    if ((mask & (uint32_t)LDG_HW_SENSOR_GROUP_TEMPERATURE) != 0) { hw_sensor_thermal_collect(out); }

    if ((mask & (uint32_t)LDG_HW_SENSOR_GROUP_FREQ) != 0) { hw_sensor_freq_collect(out); }

    if ((mask & (uint32_t)LDG_HW_SENSOR_GROUP_POWER) != 0) { hw_sensor_rapl_collect(out); }

    if ((mask & (uint32_t)LDG_HW_SENSOR_GROUP_CLOCK) != 0) { hw_sensor_clock_collect(out); }

    LDG_WR_ONCE(g_hw_sensor.avail, hw_sensor_avail_derive(out));

    return LDG_ERR_AOK;
}

static void* hw_sensor_sampler_enter(void *arg)
{
    struct timespec nap = LDG_STRUCT_ZERO_INIT;
    ldg_hw_sensor_t sample = LDG_STRUCT_ZERO_INIT;
    int64_t nap_nsec = 0;
    uint32_t period = 0;

    if (LDG_UNLIKELY(!arg)) { return 0x0; }

    while (LDG_RD_ONCE(g_hw_sensor.running) == LDG_TRUTH_TRUE)
    {
        if (ldg_hw_sensor_sample_get(&sample) == LDG_ERR_AOK)
        {
            if (ldg_spsc_full_is(&g_hw_sensor.ring) == LDG_TRUTH_TRUE) { ldg_spsc_pop(&g_hw_sensor.ring, 0x0); }

            ldg_spsc_push(&g_hw_sensor.ring, &sample);

            if (ldg_mut_lock(&g_hw_sensor.lock) == LDG_ERR_AOK)
            {
                if (LDG_UNLIKELY(memcpy(&g_hw_sensor.latest, &sample, sizeof(ldg_hw_sensor_t)) != &g_hw_sensor.latest)) { LDG_ERRLOG_ERR("hw: sensor latest copy failed"); }

                g_hw_sensor.has_latest = LDG_TRUTH_TRUE;

                if (ldg_mut_unlock(&g_hw_sensor.lock) != LDG_ERR_AOK) { LDG_ERRLOG_ERR("hw: sensor latest unlock failed"); }
            }
        }

        period = LDG_RD_ONCE(g_hw_sensor.period);

        nap_nsec = (int64_t)(period % LDG_MS_PER_SEC) * LDG_NS_PER_MS;

        nap.tv_sec = (time_t)(period / LDG_MS_PER_SEC);
        nap.tv_nsec = nap_nsec;

        if (nanosleep(&nap, 0x0) != 0) { continue; }
    }

    return 0x0;
}

uint32_t ldg_hw_sensor_init(uint32_t depth)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(depth == 0 || depth > LDG_HW_SENSOR_DEPTH_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (g_hw_sensor.is_init == LDG_TRUTH_TRUE) { return LDG_ERR_EXISTS; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(&g_hw_sensor, (uint64_t)sizeof(hw_sensor_ctx_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    ret = ldg_spsc_init(&g_hw_sensor.ring, (uint64_t)sizeof(ldg_hw_sensor_t), (uint64_t)depth);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mut_init(&g_hw_sensor.lock, LDG_TRUTH_FALSE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_spsc_shutdown(&g_hw_sensor.ring);
        return ret;
    }

    hw_sensor_absent_set(&g_hw_sensor.latest);

    g_hw_sensor.depth = depth;
    g_hw_sensor.mask = (uint32_t)LDG_HW_SENSOR_GROUP_ALL;
    g_hw_sensor.period = LDG_HW_SENSOR_PERIOD_MIN_MS;
    g_hw_sensor.is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_hw_sensor_shutdown(void)
{
    if (g_hw_sensor.is_init != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (LDG_RD_ONCE(g_hw_sensor.running) == LDG_TRUTH_TRUE) { if (ldg_hw_sensor_sampler_stop() != LDG_ERR_AOK) { LDG_ERRLOG_WARN("hw: sensor sampler stop failed during shutdown"); } }

    if (ldg_spsc_shutdown(&g_hw_sensor.ring) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("hw: sensor ring shutdown failed"); }

    if (ldg_mut_destroy(&g_hw_sensor.lock) != LDG_ERR_AOK) { LDG_ERRLOG_WARN("hw: sensor lock destroy failed"); }

    g_hw_sensor.is_init = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_hw_sensor_group_set(uint32_t mask)
{
    if (LDG_UNLIKELY(mask == LDG_HW_SENSOR_GROUP_NONE || (mask & ~(uint32_t)LDG_HW_SENSOR_GROUP_ALL) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    LDG_WR_ONCE(g_hw_sensor.mask, mask);

    return LDG_ERR_AOK;
}

uint32_t ldg_hw_sensor_group_get(uint32_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_RD_ONCE(g_hw_sensor.mask);

    return LDG_ERR_AOK;
}

uint32_t ldg_hw_sensor_group_avail_get(uint32_t *out)
{
    ldg_hw_sensor_t probe = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_HW_SENSOR_GROUP_NONE;

    if (LDG_RD_ONCE(g_hw_sensor.avail) == LDG_HW_SENSOR_GROUP_NONE)
    {
        ret = ldg_hw_sensor_sample_get(&probe);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    *out = LDG_RD_ONCE(g_hw_sensor.avail);

    return LDG_ERR_AOK;
}

uint32_t ldg_hw_sensor_sampler_start(uint32_t period)
{
    pthread_t handle = LDG_STRUCT_ZERO_INIT;

    LDG_BOOL_ASSERT(sizeof(pthread_t) <= sizeof(uint64_t));

    if (g_hw_sensor.is_init != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(period < LDG_HW_SENSOR_PERIOD_MIN_MS)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_RD_ONCE(g_hw_sensor.running) == LDG_TRUTH_TRUE) { return LDG_ERR_BUSY; }

    LDG_WR_ONCE(g_hw_sensor.period, period);
    LDG_WR_ONCE(g_hw_sensor.running, LDG_TRUTH_TRUE);

    if (LDG_UNLIKELY(pthread_create(&handle, 0x0, hw_sensor_sampler_enter, &g_hw_sensor) != 0))
    {
        LDG_WR_ONCE(g_hw_sensor.running, LDG_TRUTH_FALSE);
        return LDG_ERR_BUSY;
    }

    if (LDG_UNLIKELY(memcpy(&g_hw_sensor.thread, &handle, sizeof(pthread_t)) != &g_hw_sensor.thread))
    {
        LDG_WR_ONCE(g_hw_sensor.running, LDG_TRUTH_FALSE);
        return LDG_ERR_MEM_BAD;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_hw_sensor_sampler_stop(void)
{
    pthread_t handle = LDG_STRUCT_ZERO_INIT;

    if (LDG_RD_ONCE(g_hw_sensor.running) != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    LDG_WR_ONCE(g_hw_sensor.running, LDG_TRUTH_FALSE);

    if (LDG_UNLIKELY(memcpy(&handle, &g_hw_sensor.thread, sizeof(pthread_t)) != &handle)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(pthread_join(handle, 0x0) != 0)) { return LDG_ERR_BUSY; }

    return LDG_ERR_AOK;
}

uint32_t ldg_hw_sensor_drain(ldg_hw_sensor_t *buff, uint32_t cap, uint32_t *out_cunt)
{
    uint32_t taken = 0;

    if (LDG_UNLIKELY(!buff || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (g_hw_sensor.is_init != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    while (taken < cap)
    {
        if (ldg_spsc_pop(&g_hw_sensor.ring, &buff[taken]) != LDG_ERR_AOK) { break; }

        taken++;
    }

    *out_cunt = taken;

    return LDG_ERR_AOK;
}

uint32_t ldg_hw_sensor_latest_get(ldg_hw_sensor_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (g_hw_sensor.is_init != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (g_hw_sensor.has_latest != LDG_TRUTH_TRUE) { return ldg_hw_sensor_sample_get(out); }

    ret = ldg_mut_lock(&g_hw_sensor.lock);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(memcpy(out, &g_hw_sensor.latest, sizeof(ldg_hw_sensor_t)) != out))
    {
        if (ldg_mut_unlock(&g_hw_sensor.lock) != LDG_ERR_AOK) { LDG_ERRLOG_ERR("hw: sensor latest unlock failed"); }

        return LDG_ERR_MEM_BAD;
    }

    ret = ldg_mut_unlock(&g_hw_sensor.lock);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

uint8_t ldg_hw_sensor_sampling_is(void)
{
    uint8_t res = LDG_TRUTH_FALSE;

    if (g_hw_sensor.is_init != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    res = (LDG_RD_ONCE(g_hw_sensor.running) == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return res;
}
