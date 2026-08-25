#include <windows.h>
#include <setupapi.h>
#include <poclass.h>
#include <stdio.h>

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

#define LDG_HW_SENSOR_ZONE_MAX 16
#define LDG_HW_SENSOR_DETAIL_CAP 64
#define LDG_HW_SENSOR_IDX_FIRST 0
#define LDG_HW_SENSOR_STEP 1
#define LDG_HW_SENSOR_NO_FLAG 0
#define LDG_HW_SENSOR_WAIT_IMMEDIATE 0
#define LDG_HW_SENSOR_TEMP_LOW 0
#define LDG_HW_SENSOR_TEMP_HIGH UINT32_MAX
#define LDG_HW_SENSOR_MILLIKELVIN_PER_DECIKELVIN 100
#define LDG_HW_SENSOR_KHZ_PER_MHZ 1000
#define LDG_HW_SENSOR_PROC_INFO_LEVEL 11
#define LDG_HW_SENSOR_JITTER_SAMPLE_MAX 32
#define LDG_HW_SENSOR_JITTER_SPAN_NS 20000
#define LDG_HW_SENSOR_FILETIME_PER_SEC 10000000
#define LDG_HW_SENSOR_PPB_SCALE 1000000000.0
#define LDG_HW_SENSOR_PPM_SCALE 1000000.0
#define LDG_HW_SENSOR_THREAD_OK 0
#define LDG_HW_SENSOR_JOIN_TIMEOUT_MS 2000

#define LDG_HW_SENSOR_THERMAL_ZONE_GUID { 0x4AFA3D51, 0x74A7, 0x11D0, { 0xBE, 0x5E, 0x00, 0xA0, 0xC9, 0x06, 0x28, 0x57 } }

#define LDG_HW_SENSOR_DLL_POWRPROF "powrprof.dll"
#define LDG_HW_SENSOR_PROC_POWER_INFO "CallNtPowerInformation"

typedef struct hw_sensor_proc_power
{
    ULONG number;
    ULONG max_mhz;
    ULONG current_mhz;
    ULONG mhz_limit;
    ULONG max_idle_state;
    ULONG current_idle_state;
} hw_sensor_proc_power_t;

typedef LONG (WINAPI *hw_sensor_power_info_fn)(ULONG level, PVOID in_buff, ULONG in_cap, PVOID out_buff, ULONG out_cap);

typedef struct hw_sensor_ctx
{
    ldg_spsc_queue_t ring;
    ldg_hw_sensor_t latest;
    ldg_mut_t lock;
    hw_sensor_proc_power_t proc_power[LDG_HW_SENSOR_CORE_MAX];
    LARGE_INTEGER perf_base;
    LARGE_INTEGER perf_freq;
    ULARGE_INTEGER time_base;
    HANDLE thread;
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

static uint32_t hw_sensor_millikelvin_get(ULONG decikelvin, uint32_t *out)
{
    uint64_t scaled = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_HW_SENSOR_ABSENT;

    scaled = (uint64_t)decikelvin * LDG_HW_SENSOR_MILLIKELVIN_PER_DECIKELVIN;

    if (LDG_UNLIKELY(scaled >= (uint64_t)UINT32_MAX)) { return LDG_ERR_RANGE; }

    *out = (uint32_t)scaled;

    return LDG_ERR_AOK;
}

static LDG_NOINLINE void hw_sensor_thermal_collect(ldg_hw_sensor_t *out)
{
    GUID zone_class = LDG_HW_SENSOR_THERMAL_ZONE_GUID;
    SP_DEVICE_INTERFACE_DATA face = LDG_STRUCT_ZERO_INIT;
    THERMAL_WAIT_READ wait = LDG_STRUCT_ZERO_INIT;
    HDEVINFO info_set = INVALID_HANDLE_VALUE;
    HANDLE zone = INVALID_HANDLE_VALUE;
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail = 0x0;
    uint64_t detail_buff[LDG_HW_SENSOR_DETAIL_CAP] = LDG_ARR_ZERO_INIT;
    ULONG decikelvin = 0;
    DWORD needed = 0;
    DWORD given = 0;
    DWORD idx = 0;
    uint32_t converted = 0;

    info_set = SetupDiGetClassDevsW(&zone_class, 0x0, 0x0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (LDG_UNLIKELY(info_set == INVALID_HANDLE_VALUE)) { return; }

    face.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)detail_buff;

    for (idx = 0; idx < LDG_HW_SENSOR_ZONE_MAX; idx++)
    {
        if (!SetupDiEnumDeviceInterfaces(info_set, 0x0, &zone_class, idx, &face)) { break; }

        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        if (!SetupDiGetDeviceInterfaceDetailW(info_set, &face, detail, (DWORD)sizeof(detail_buff), &needed, 0x0)) { continue; }

        zone = CreateFileW(detail->DevicePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0x0, OPEN_EXISTING, LDG_HW_SENSOR_NO_FLAG, 0x0);
        if (zone == INVALID_HANDLE_VALUE) { continue; }

        wait.Timeout = LDG_HW_SENSOR_WAIT_IMMEDIATE;
        wait.LowTemperature = LDG_HW_SENSOR_TEMP_LOW;
        wait.HighTemperature = LDG_HW_SENSOR_TEMP_HIGH;

        if (DeviceIoControl(zone, IOCTL_THERMAL_READ_TEMPERATURE, &wait, (DWORD)sizeof(wait), &decikelvin, (DWORD)sizeof(decikelvin), &given, 0x0)) { if (hw_sensor_millikelvin_get(decikelvin, &converted) == LDG_ERR_AOK) { if (out->cpu_temperature == LDG_HW_SENSOR_ABSENT) { out->cpu_temperature = converted; }
                else if (out->board_temperature == LDG_HW_SENSOR_ABSENT) { out->board_temperature = converted; } } }

        if (!CloseHandle(zone)) { LDG_ERRLOG_WARN("hw: sensor thermal zone close failed"); }
    }

    if (!SetupDiDestroyDeviceInfoList(info_set)) { LDG_ERRLOG_WARN("hw: sensor thermal info set destroy failed"); }
}

static LDG_NOINLINE void hw_sensor_freq_collect(ldg_hw_sensor_t *out)
{
    HMODULE powrprof = 0x0;
    hw_sensor_power_info_fn power_info = 0x0;
    SYSTEM_INFO info = LDG_STRUCT_ZERO_INIT;
    uint32_t core_cunt = 0;
    uint32_t idx = 0;
    uint8_t varied = LDG_TRUTH_FALSE;

    powrprof = LoadLibraryA(LDG_HW_SENSOR_DLL_POWRPROF);
    if (LDG_UNLIKELY(!powrprof)) { return; }

    power_info = (hw_sensor_power_info_fn)(void *)GetProcAddress(powrprof, LDG_HW_SENSOR_PROC_POWER_INFO);
    if (LDG_UNLIKELY(!power_info))
    {
        if (!FreeLibrary(powrprof)) { LDG_ERRLOG_WARN("hw: sensor powrprof release failed"); }

        return;
    }

    GetSystemInfo(&info);

    core_cunt = (uint32_t)info.dwNumberOfProcessors;

    if (core_cunt > LDG_HW_SENSOR_CORE_MAX) { core_cunt = LDG_HW_SENSOR_CORE_MAX; }

    if (ldg_mut_lock(&g_hw_sensor.lock) == LDG_ERR_AOK)
    {
        if (power_info(LDG_HW_SENSOR_PROC_INFO_LEVEL, 0x0, LDG_HW_SENSOR_NO_FLAG, g_hw_sensor.proc_power, (ULONG)(core_cunt * sizeof(hw_sensor_proc_power_t))) == LDG_HW_SENSOR_THREAD_OK)
        {
            for (idx = LDG_HW_SENSOR_IDX_FIRST; idx < core_cunt; idx++) { if (g_hw_sensor.proc_power[idx].current_mhz != g_hw_sensor.proc_power[idx].max_mhz) { varied = LDG_TRUTH_TRUE; } }

            if (varied == LDG_TRUTH_TRUE) { for (idx = LDG_HW_SENSOR_IDX_FIRST; idx < core_cunt; idx++)
                {
                    out->core_freq[idx] = (uint32_t)g_hw_sensor.proc_power[idx].current_mhz * LDG_HW_SENSOR_KHZ_PER_MHZ;
                    out->core_cunt++;
                }
            }
        }

        if (ldg_mut_unlock(&g_hw_sensor.lock) != LDG_ERR_AOK) { LDG_ERRLOG_ERR("hw: sensor freq unlock failed"); }
    }

    if (!FreeLibrary(powrprof)) { LDG_ERRLOG_WARN("hw: sensor powrprof release failed"); }
}

static LDG_NOINLINE void hw_sensor_clock_collect(ldg_hw_sensor_t *out)
{
    LARGE_INTEGER perf_open = LDG_STRUCT_ZERO_INIT;
    LARGE_INTEGER perf_close = LDG_STRUCT_ZERO_INIT;
    ULARGE_INTEGER stamp = LDG_STRUCT_ZERO_INIT;
    FILETIME wall = LDG_STRUCT_ZERO_INIT;
    double ratio[LDG_HW_SENSOR_JITTER_SAMPLE_MAX] = LDG_ARR_ZERO_INIT;
    double mean = 0.0;
    double variance = 0.0;
    double diff = 0.0;
    double tsc_span = 0.0;
    double perf_span = 0.0;
    double perf_elapsed = 0.0;
    double wall_elapsed = 0.0;
    int64_t span_tick = 0;
    uint64_t tsc_open = 0;
    uint64_t tsc_close = 0;
    uint32_t idx = 0;
    uint32_t taken = 0;

    GetSystemTimePreciseAsFileTime(&wall);

    stamp.LowPart = wall.dwLowDateTime;
    stamp.HighPart = wall.dwHighDateTime;

    if (QueryPerformanceCounter(&perf_open) && g_hw_sensor.perf_freq.QuadPart > 0 && stamp.QuadPart > g_hw_sensor.time_base.QuadPart)
    {
        perf_elapsed = (double)(perf_open.QuadPart - g_hw_sensor.perf_base.QuadPart) / (double)g_hw_sensor.perf_freq.QuadPart;
        wall_elapsed = (double)(stamp.QuadPart - g_hw_sensor.time_base.QuadPart) / (double)LDG_HW_SENSOR_FILETIME_PER_SEC;

        if (wall_elapsed >= 1.0 && perf_elapsed > 0.0) { out->clock_drift = (perf_elapsed / wall_elapsed - 1.0) * LDG_HW_SENSOR_PPM_SCALE; }
    }

    span_tick = g_hw_sensor.perf_freq.QuadPart * LDG_HW_SENSOR_JITTER_SPAN_NS / LDG_NS_PER_SEC;

    if (span_tick <= 0) { return; }

    for (idx = LDG_HW_SENSOR_IDX_FIRST; idx < LDG_HW_SENSOR_JITTER_SAMPLE_MAX; idx++)
    {
        tsc_open = ldg_rdtsc();

        if (!QueryPerformanceCounter(&perf_open)) { break; }

        LDG_BARRIER;

        do{ if (!QueryPerformanceCounter(&perf_close)) { break; } }
        while (perf_close.QuadPart > perf_open.QuadPart && perf_close.QuadPart - perf_open.QuadPart < span_tick);

        tsc_close = ldg_rdtsc();

        if (perf_close.QuadPart <= perf_open.QuadPart) { continue; }

        if (perf_close.QuadPart - perf_open.QuadPart < span_tick) { continue; }

        tsc_span = (double)(tsc_close - tsc_open);
        perf_span = (double)(perf_close.QuadPart - perf_open.QuadPart);

        ratio[taken] = tsc_span / perf_span;
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

    if ((mask & (uint32_t)LDG_HW_SENSOR_GROUP_TEMPERATURE) != 0) { hw_sensor_thermal_collect(out); }

    if ((mask & (uint32_t)LDG_HW_SENSOR_GROUP_FREQ) != 0) { hw_sensor_freq_collect(out); }

    if ((mask & (uint32_t)LDG_HW_SENSOR_GROUP_CLOCK) != 0) { hw_sensor_clock_collect(out); }

    LDG_WR_ONCE(g_hw_sensor.avail, hw_sensor_avail_derive(out));

    return LDG_ERR_AOK;
}

static DWORD WINAPI hw_sensor_sampler_enter(LPVOID arg)
{
    ldg_hw_sensor_t sample = LDG_STRUCT_ZERO_INIT;
    uint32_t period = 0;

    if (LDG_UNLIKELY(!arg)) { return LDG_HW_SENSOR_THREAD_OK; }

    while (LDG_RD_ONCE(g_hw_sensor.running) == LDG_TRUTH_TRUE)
    {
        if (ldg_hw_sensor_sample_get(&sample) == LDG_ERR_AOK)
        {
            if (ldg_spsc_full_is(&g_hw_sensor.ring) == LDG_TRUTH_TRUE) { ldg_spsc_pop(&g_hw_sensor.ring, 0x0); }

            ldg_spsc_push(&g_hw_sensor.ring, &sample);

            if (ldg_mut_lock(&g_hw_sensor.lock) == LDG_ERR_AOK)
            {
                if (LDG_UNLIKELY(ldg_mem_secure_cpy(&g_hw_sensor.latest, &sample, (uint64_t)sizeof(ldg_hw_sensor_t)) != LDG_ERR_AOK)) { LDG_ERRLOG_ERR("hw: sensor latest copy failed"); }

                g_hw_sensor.has_latest = LDG_TRUTH_TRUE;

                if (ldg_mut_unlock(&g_hw_sensor.lock) != LDG_ERR_AOK) { LDG_ERRLOG_ERR("hw: sensor latest unlock failed"); }
            }
        }

        period = LDG_RD_ONCE(g_hw_sensor.period);

        Sleep((DWORD)period);
    }

    return LDG_HW_SENSOR_THREAD_OK;
}

uint32_t ldg_hw_sensor_init(uint32_t depth)
{
    FILETIME wall = LDG_STRUCT_ZERO_INIT;
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

    if (!QueryPerformanceFrequency(&g_hw_sensor.perf_freq)) { LDG_ERRLOG_WARN("hw: sensor performance frequency unavailable"); }

    if (!QueryPerformanceCounter(&g_hw_sensor.perf_base)) { LDG_ERRLOG_WARN("hw: sensor performance counter unavailable"); }

    GetSystemTimePreciseAsFileTime(&wall);

    g_hw_sensor.time_base.LowPart = wall.dwLowDateTime;
    g_hw_sensor.time_base.HighPart = wall.dwHighDateTime;

    g_hw_sensor.thread = INVALID_HANDLE_VALUE;
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
    HANDLE handle = INVALID_HANDLE_VALUE;

    if (g_hw_sensor.is_init != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(period < LDG_HW_SENSOR_PERIOD_MIN_MS)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_RD_ONCE(g_hw_sensor.running) == LDG_TRUTH_TRUE) { return LDG_ERR_BUSY; }

    LDG_WR_ONCE(g_hw_sensor.period, period);
    LDG_WR_ONCE(g_hw_sensor.running, LDG_TRUTH_TRUE);

    handle = CreateThread(0x0, LDG_HW_SENSOR_NO_FLAG, hw_sensor_sampler_enter, &g_hw_sensor, LDG_HW_SENSOR_NO_FLAG, 0x0);
    if (LDG_UNLIKELY(!handle))
    {
        LDG_WR_ONCE(g_hw_sensor.running, LDG_TRUTH_FALSE);
        return LDG_ERR_BUSY;
    }

    g_hw_sensor.thread = handle;

    return LDG_ERR_AOK;
}

uint32_t ldg_hw_sensor_sampler_stop(void)
{
    if (LDG_RD_ONCE(g_hw_sensor.running) != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_INIT; }

    LDG_WR_ONCE(g_hw_sensor.running, LDG_TRUTH_FALSE);

    if (g_hw_sensor.thread == INVALID_HANDLE_VALUE) { return LDG_ERR_NOT_INIT; }

    if (WaitForSingleObject(g_hw_sensor.thread, LDG_HW_SENSOR_JOIN_TIMEOUT_MS) == WAIT_TIMEOUT) { LDG_ERRLOG_WARN("hw: sensor sampler join timed out"); }

    if (!CloseHandle(g_hw_sensor.thread)) { LDG_ERRLOG_WARN("hw: sensor sampler handle close failed"); }

    g_hw_sensor.thread = INVALID_HANDLE_VALUE;

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

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(out, &g_hw_sensor.latest, (uint64_t)sizeof(ldg_hw_sensor_t)) != LDG_ERR_AOK))
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
