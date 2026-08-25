#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#include <dangling/io/path.h>
#include <dangling/io/file.h>
#include <dangling/io/dir.h>
#include <dangling/mem/secure.h>

#define LDG_SYS_URL_SCHEME_HTTP "http:"
#define LDG_SYS_URL_SCHEME_HTTPS "https:"
#define LDG_SYS_URL_OPEN_CMD "xdg-open"
#define LDG_SYS_MIME_CMD "xdg-mime"
#define LDG_SYS_NOTIFY_CMD "notify-send"
#define LDG_SYS_NOTIFY_LEVEL_FLAG "-u"
#define LDG_SYS_NOTIFY_LEVEL_LOW_ARG "low"
#define LDG_SYS_NOTIFY_LEVEL_NORMAL_ARG "normal"
#define LDG_SYS_NOTIFY_LEVEL_CRIT_ARG "critical"
#define LDG_SYS_NOTIFY_ICON_FLAG "-i"
#define LDG_SYS_NOTIFY_ARGV_ICON_MAX 8
#define LDG_SYS_SELF_EXE_LINK "/proc/self/exe"
#define LDG_SYS_APPS_DIR "applications"
#define LDG_SYS_SCHEME_MIME_PREFIX "x-scheme-handler/"
#define LDG_SYS_DESKTOP_PREFIX "ldg-"
#define LDG_SYS_DESKTOP_EXT ".desktop"
#define LDG_SYS_EXIT_OK 0
#define LDG_SYS_EXIT_EXEC_FAIL 127
#define LDG_SYS_WAIT_NO_FLAG 0
#define LDG_SYS_URL_SCHEME_MAX 32
#define LDG_SYS_APP_NAME_MAX 128
#define LDG_SYS_PATH_BUFF_MAX 512
#define LDG_SYS_EXEC_BUFF_MAX 1024
#define LDG_SYS_DESKTOP_BUFF_MAX 2048
#define LDG_SYS_DIR_MODE 0755
#define LDG_SYS_FILE_MODE 0644
#define LDG_SYS_ARGV_MAX 5
#define LDG_SYS_NOTIFY_ARGV_MAX 6
#define LDG_SYS_MEMINFO_PATH "/proc/meminfo"
#define LDG_SYS_MEMINFO_AVAIL_KEY "MemAvailable:"
#define LDG_SYS_MEMINFO_LINE_MAX 256
#define LDG_SYS_MEMINFO_RD_MODE "r"
#define LDG_SYS_ASCII_ZERO '0'
#define LDG_SYS_CACHE_LEVEL_PATH "/sys/devices/system/cpu/cpu%u32/cache/index%u32/level"
#define LDG_SYS_CACHE_WIDTH_PATH "/sys/devices/system/cpu/cpu%u32/cache/index%u32/size"
#define LDG_SYS_CPU_PERF_LIST_PATH "/sys/devices/cpu_core/cpus"
#define LDG_SYS_CPU_EFFICIENT_LIST_PATH "/sys/devices/cpu_atom/cpus"
#define LDG_SYS_CACHE_IDX_MAX 10
#define LDG_SYS_SYSFS_TEXT_MAX 64
#define LDG_SYS_CPU_CLASS_BASE 0
#define LDG_SYS_CPU_CLASS_PERF 1
#define LDG_SYS_SCALE_KIB 'K'
#define LDG_SYS_SCALE_MIB 'M'
#define LDG_SYS_LIST_RANGE_SEP '-'
#define LDG_SYS_LIST_ITEM_SEP ','

#include <dangling/sys/info.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/str/fmt.h>

uint32_t ldg_sys_hostname_get(char *buff, uint64_t buff_size)
{
    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(gethostname(buff, (uint64_t)buff_size - LDG_STR_TERM_SIZE) < 0)) { return LDG_ERR_IO_RD; }

    buff[buff_size - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_cpu_cunt_get(uint32_t *cunt)
{
    int64_t res = 0;

    if (LDG_UNLIKELY(!cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *cunt = 0;

    res = (int64_t)sysconf(_SC_NPROCESSORS_ONLN);
    if (LDG_UNLIKELY(res <= 0)) { return LDG_ERR_UNSUPPORTED; }

    *cunt = (uint32_t)res;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_page_size_get(uint64_t *size)
{
    int64_t res = 0;

    if (LDG_UNLIKELY(!size)) { return LDG_ERR_FUNC_ARG_NULL; }

    *size = 0;

    res = (int64_t)sysconf(_SC_PAGESIZE);
    if (LDG_UNLIKELY(res <= 0)) { return LDG_ERR_UNSUPPORTED; }

    *size = (uint64_t)res;

    return LDG_ERR_AOK;
}

static uint32_t sys_sysfs_text_get(const char *path, char *buff, uint64_t cap)
{
    FILE *node = 0x0;
    uint8_t got = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    buff[0] = LDG_STR_TERM;

    node = fopen(path, LDG_SYS_MEMINFO_RD_MODE);
    if (!node) { return LDG_ERR_IO_NOT_FOUND; }

    got = fgets(buff, (int32_t)cap, node) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(fclose(node) != 0)) { return LDG_ERR_IO_CLOSE; }

    if (got != LDG_TRUTH_TRUE) { return LDG_ERR_IO_RD; }

    return LDG_ERR_AOK;
}

static uint64_t sys_digits_take(const char *text, uint64_t *cursor)
{
    uint64_t val = 0;

    while (ldg_char_digit_is(text[*cursor]) == LDG_TRUTH_TRUE)
    {
        val = (val * LDG_BASE_DEC) + (uint64_t)(text[*cursor] - LDG_SYS_ASCII_ZERO);
        *cursor = *cursor + LDG_SYS_CPU_CLASS_PERF;
    }

    return val;
}

static uint8_t sys_cpu_list_holds_is(const char *path, uint32_t cpu_idx)
{
    char text[LDG_SYS_SYSFS_TEXT_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t cursor = 0;
    uint64_t low = 0;
    uint64_t high = 0;

    if (sys_sysfs_text_get(path, text, LDG_SYS_SYSFS_TEXT_MAX) != LDG_ERR_AOK) { return LDG_TRUTH_FALSE; }

    while (text[cursor] != LDG_STR_TERM)
    {
        if (ldg_char_digit_is(text[cursor]) != LDG_TRUTH_TRUE)
        {
            cursor = cursor + LDG_SYS_CPU_CLASS_PERF;
            continue;
        }

        low = sys_digits_take(text, &cursor);
        high = low;

        if (text[cursor] == LDG_SYS_LIST_RANGE_SEP)
        {
            cursor = cursor + LDG_SYS_CPU_CLASS_PERF;
            high = sys_digits_take(text, &cursor);
        }

        if ((uint64_t)cpu_idx >= low && (uint64_t)cpu_idx <= high) { return LDG_TRUTH_TRUE; }
    }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_sys_cpu_cache_width_get(uint32_t cpu_idx, uint32_t level, uint64_t *out)
{
    char path[LDG_SYS_PATH_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    char text[LDG_SYS_SYSFS_TEXT_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t path_len = 0;
    uint64_t cursor = 0;
    uint64_t found = 0;
    uint64_t width = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(level == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < LDG_SYS_CACHE_IDX_MAX; idx++)
    {
        if (ldg_str_fmt(path, LDG_SYS_PATH_BUFF_MAX, &path_len, LDG_SYS_CACHE_LEVEL_PATH, cpu_idx, idx) != LDG_ERR_AOK) { return LDG_ERR_IO_FMT; }

        if (sys_sysfs_text_get(path, text, LDG_SYS_SYSFS_TEXT_MAX) != LDG_ERR_AOK) { continue; }

        cursor = 0;
        found = sys_digits_take(text, &cursor);

        if (found != (uint64_t)level) { continue; }

        if (ldg_str_fmt(path, LDG_SYS_PATH_BUFF_MAX, &path_len, LDG_SYS_CACHE_WIDTH_PATH, cpu_idx, idx) != LDG_ERR_AOK) { return LDG_ERR_IO_FMT; }

        if (sys_sysfs_text_get(path, text, LDG_SYS_SYSFS_TEXT_MAX) != LDG_ERR_AOK) { return LDG_ERR_IO_RD; }

        cursor = 0;
        width = sys_digits_take(text, &cursor);

        if (text[cursor] == LDG_SYS_SCALE_KIB) { width = width * LDG_KIB; }
        else if (text[cursor] == LDG_SYS_SCALE_MIB) { width = width * LDG_MIB; }

        if (LDG_UNLIKELY(width == 0)) { return LDG_ERR_NOT_FOUND; }

        *out = width;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_sys_cpu_class_get(uint32_t cpu_idx, uint32_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_SYS_CPU_CLASS_BASE;

    if (sys_cpu_list_holds_is(LDG_SYS_CPU_PERF_LIST_PATH, cpu_idx) == LDG_TRUTH_TRUE)
    {
        *out = LDG_SYS_CPU_CLASS_PERF;

        return LDG_ERR_AOK;
    }

    if (sys_cpu_list_holds_is(LDG_SYS_CPU_EFFICIENT_LIST_PATH, cpu_idx) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_mem_total_get(uint64_t *out)
{
    int64_t pages = 0;
    int64_t page_width = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    pages = (int64_t)sysconf(_SC_PHYS_PAGES);
    if (LDG_UNLIKELY(pages <= 0)) { return LDG_ERR_UNSUPPORTED; }

    page_width = (int64_t)sysconf(_SC_PAGESIZE);
    if (LDG_UNLIKELY(page_width <= 0)) { return LDG_ERR_UNSUPPORTED; }

    *out = (uint64_t)pages * (uint64_t)page_width;

    return LDG_ERR_AOK;
}

static uint32_t sys_mem_free_get(uint64_t *out)
{
    int64_t pages = 0;
    int64_t page_width = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    pages = (int64_t)sysconf(_SC_AVPHYS_PAGES);
    if (LDG_UNLIKELY(pages <= 0)) { return LDG_ERR_UNSUPPORTED; }

    page_width = (int64_t)sysconf(_SC_PAGESIZE);
    if (LDG_UNLIKELY(page_width <= 0)) { return LDG_ERR_UNSUPPORTED; }

    *out = (uint64_t)pages * (uint64_t)page_width;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_mem_avail_get(uint64_t *out)
{
    char line[LDG_SYS_MEMINFO_LINE_MAX] = LDG_ARR_ZERO_INIT;
    FILE *meminfo = 0x0;
    uint64_t kib = 0;
    uint64_t idx = 0;
    uint64_t key_len = 0;
    uint8_t matched = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(ldg_str_len_get(LDG_SYS_MEMINFO_AVAIL_KEY, LDG_SYS_MEMINFO_LINE_MAX, &key_len) != LDG_ERR_AOK)) { return LDG_ERR_UNSUPPORTED; }

    meminfo = fopen(LDG_SYS_MEMINFO_PATH, LDG_SYS_MEMINFO_RD_MODE);
    if (!meminfo) { return sys_mem_free_get(out); }

    while (fgets(line, (int32_t)LDG_SYS_MEMINFO_LINE_MAX, meminfo))
    {
        matched = LDG_TRUTH_TRUE;
        for (idx = 0; idx < key_len; idx++) { if (line[idx] != LDG_SYS_MEMINFO_AVAIL_KEY[idx]) { matched = LDG_TRUTH_FALSE; } }

        if (matched != LDG_TRUTH_TRUE) { continue; }

        for (idx = key_len; line[idx] != LDG_STR_TERM; idx++)
        {
            if (ldg_char_digit_is(line[idx]) != LDG_TRUTH_TRUE) { continue; }

            while (ldg_char_digit_is(line[idx]) == LDG_TRUTH_TRUE)
            {
                kib = (kib * LDG_BASE_DEC) + (uint64_t)(line[idx] - LDG_SYS_ASCII_ZERO);
                idx++;
            }

            break;
        }

        break;
    }

    if (LDG_UNLIKELY(fclose(meminfo) != 0)) { return LDG_ERR_IO_CLOSE; }

    if (kib == 0) { return sys_mem_free_get(out); }

    *out = kib * LDG_KIB;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_env_get(const char *name, char *buff, uint64_t buff_size)
{
    const char *val = 0x0;

    if (LDG_UNLIKELY(!name || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    val = getenv(name);
    if (!val) { return LDG_ERR_NOT_FOUND; }

    return ldg_str_rbrcpy(buff, val, (uint64_t)buff_size);
}

uint32_t ldg_sys_pid_get(uint64_t *pid)
{
    if (LDG_UNLIKELY(!pid)) { return LDG_ERR_FUNC_ARG_NULL; }

    *pid = (uint64_t)getpid();

    return LDG_ERR_AOK;
}

static uint8_t sys_url_scheme_allowed_is(const char *url)
{
    if (strncmp(url, LDG_SYS_URL_SCHEME_HTTPS, sizeof(LDG_SYS_URL_SCHEME_HTTPS) - LDG_STR_TERM_SIZE) == 0) { return LDG_TRUTH_TRUE; }

    if (strncmp(url, LDG_SYS_URL_SCHEME_HTTP, sizeof(LDG_SYS_URL_SCHEME_HTTP) - LDG_STR_TERM_SIZE) == 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t sys_detached_spawn(char *const *argv)
{
    int64_t inter = 0;
    int64_t grandchild = 0;

    inter = (int64_t)fork();
    if (LDG_UNLIKELY(inter < 0)) { return LDG_ERR_AGAIN; }

    if (inter == 0)
    {
        grandchild = (int64_t)fork();
        if (grandchild == 0)
        {
            execvp(argv[0], argv);
            _exit(LDG_SYS_EXIT_EXEC_FAIL);
        }

        _exit(LDG_SYS_EXIT_OK);
    }

    if (LDG_UNLIKELY(waitpid((pid_t)inter, 0x0, LDG_SYS_WAIT_NO_FLAG) < 0)) { return LDG_ERR_IO_RD; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_url_open(const char *url)
{
    char *argv[LDG_SYS_ARGV_MAX] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY(!url)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(url[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (sys_url_scheme_allowed_is(url) != LDG_TRUTH_TRUE) { return LDG_ERR_FUNC_ARG_INVALID; }

    argv[0] = (char *)LDG_SYS_URL_OPEN_CMD;
    argv[1] = (char *)url;

    return sys_detached_spawn(argv);
}

static uint8_t sys_url_scheme_valid_is(const char *scheme)
{
    uint64_t idx = 0;

    if (scheme[0] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    if (ldg_char_alpha_is(scheme[0]) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    for (idx = 0; scheme[idx] != LDG_STR_TERM; idx++)
    {
        if (idx >= LDG_SYS_URL_SCHEME_MAX) { return LDG_TRUTH_FALSE; }

        if (ldg_char_alnum_is(scheme[idx]) == LDG_TRUTH_TRUE) { continue; }

        if (scheme[idx] == '+' || scheme[idx] == '-' || scheme[idx] == '.') { continue; }

        return LDG_TRUTH_FALSE;
    }

    return LDG_TRUTH_TRUE;
}

static uint32_t sys_self_exec_get(char *buff, uint64_t buff_size)
{
    int64_t len = 0;

    len = (int64_t)readlink(LDG_SYS_SELF_EXE_LINK, buff, buff_size - LDG_STR_TERM_SIZE);
    if (LDG_UNLIKELY(len <= 0)) { return LDG_ERR_IO_RD; }

    buff[len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t sys_desktop_wr(const char *path, const char *content)
{
    ldg_io_file_t *file = 0x0;
    uint64_t wr_cunt = 0;
    uint64_t len = 0;
    uint32_t ret = 0;

    len = (uint64_t)strlen(content);

    ret = ldg_io_file_open(path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_TRUNCATE, LDG_SYS_FILE_MODE, &file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_wr(file, content, len, &wr_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_io_file_close(file);
        return ret;
    }

    ret = ldg_io_file_close(file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(wr_cunt != len)) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_url_scheme_reg(const char *scheme, const char *exec_path, const char *app_name)
{
    char exec_buff[LDG_SYS_EXEC_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    char data_dir[LDG_SYS_PATH_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    char apps_dir[LDG_SYS_PATH_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    char desktop_name[LDG_SYS_PATH_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    char desktop_path[LDG_SYS_PATH_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    char mime_type[LDG_SYS_PATH_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    char content[LDG_SYS_DESKTOP_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    char *argv[LDG_SYS_ARGV_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!scheme)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sys_url_scheme_valid_is(scheme) != LDG_TRUTH_TRUE) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (exec_path != 0x0)
    {
        ret = ldg_io_path_resolve(exec_path, exec_buff, sizeof(exec_buff));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = sys_self_exec_get(exec_buff, sizeof(exec_buff));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_io_path_data_get(data_dir, sizeof(data_dir));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_path_join(data_dir, LDG_SYS_APPS_DIR, apps_dir, sizeof(apps_dir));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ldg_io_path_exists_is(apps_dir) != LDG_TRUTH_TRUE)
    {
        ret = ldg_io_dir_create(apps_dir, LDG_SYS_DIR_MODE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (LDG_UNLIKELY(snprintf(desktop_name, sizeof(desktop_name), "%.*s%.*s%.*s", (int32_t)sizeof(LDG_SYS_DESKTOP_PREFIX), LDG_SYS_DESKTOP_PREFIX, (int32_t)LDG_SYS_URL_SCHEME_MAX, scheme, (int32_t)sizeof(LDG_SYS_DESKTOP_EXT), LDG_SYS_DESKTOP_EXT) < 0)) { return LDG_ERR_IO_FMT; }

    ret = ldg_io_path_join(apps_dir, desktop_name, desktop_path, sizeof(desktop_path));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(snprintf(mime_type, sizeof(mime_type), "%.*s%.*s", (int32_t)sizeof(LDG_SYS_SCHEME_MIME_PREFIX), LDG_SYS_SCHEME_MIME_PREFIX, (int32_t)LDG_SYS_URL_SCHEME_MAX, scheme) < 0)) { return LDG_ERR_IO_FMT; }

    if (LDG_UNLIKELY(snprintf(content, sizeof(content), "[Desktop Entry]\nType=Application\nName=%.*s\nExec=%.*s %%u\nNoDisplay=true\nTerminal=false\nMimeType=%.*s;\n", (int32_t)LDG_SYS_APP_NAME_MAX, app_name ? app_name : scheme, (int32_t)LDG_SYS_EXEC_BUFF_MAX, exec_buff, (int32_t)LDG_SYS_PATH_BUFF_MAX, mime_type) < 0)) { return LDG_ERR_IO_FMT; }

    ret = sys_desktop_wr(desktop_path, content);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    argv[0] = (char *)LDG_SYS_MIME_CMD;
    argv[1] = (char *)"default";
    argv[2] = desktop_name;
    argv[3] = mime_type;

    return sys_detached_spawn(argv);
}

uint32_t ldg_sys_secret_set(const char *app, const char *key, const char *secret)
{
    if (LDG_UNLIKELY(!app || !key || !secret)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_secret_get(const char *app, const char *key, char *out, uint64_t out_size)
{
    if (LDG_UNLIKELY(!app || !key || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_media_info_get(ldg_sys_media_info_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(out, sizeof(ldg_sys_media_info_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_media_play(void)
{
    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_media_pause(void)
{
    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_media_next_skip(void)
{
    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_media_prev_skip(void)
{
    return LDG_ERR_UNSUPPORTED;
}

static const char* sys_notify_level_arg_get(ldg_sys_notify_level_t level)
{
    if (level == LDG_SYS_NOTIFY_LEVEL_LOW) { return LDG_SYS_NOTIFY_LEVEL_LOW_ARG; }

    if (level == LDG_SYS_NOTIFY_LEVEL_CRIT) { return LDG_SYS_NOTIFY_LEVEL_CRIT_ARG; }

    return LDG_SYS_NOTIFY_LEVEL_NORMAL_ARG;
}

typedef struct ldg_sys_notify_ctx
{
    char icon[LDG_SYS_PATH_BUFF_MAX];
    uint8_t icon_set;
    uint8_t pudding[7];
} ldg_sys_notify_ctx_t;

static ldg_sys_notify_ctx_t g_sys_notify = { .icon_set = LDG_TRUTH_FALSE };

uint32_t ldg_sys_notify_icon_set(const char *path)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_str_rbrcpy(g_sys_notify.icon, path, LDG_SYS_PATH_BUFF_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    g_sys_notify.icon_set = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_notify_tray_icon_set(const char *path)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_notify_tip_set(const char *tip)
{
    if (LDG_UNLIKELY(!tip)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_notify(const char *title, const char *body, ldg_sys_notify_level_t level)
{
    char *argv[LDG_SYS_NOTIFY_ARGV_ICON_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!title)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(title[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(level > LDG_SYS_NOTIFY_LEVEL_CRIT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    argv[idx] = (char *)LDG_SYS_NOTIFY_CMD;
    idx++;
    argv[idx] = (char *)LDG_SYS_NOTIFY_LEVEL_FLAG;
    idx++;
    argv[idx] = (char *)sys_notify_level_arg_get(level);
    idx++;

    if (g_sys_notify.icon_set == LDG_TRUTH_TRUE)
    {
        argv[idx] = (char *)LDG_SYS_NOTIFY_ICON_FLAG;
        idx++;
        argv[idx] = g_sys_notify.icon;
        idx++;
    }

    argv[idx] = (char *)title;
    idx++;
    argv[idx] = (char *)((body != 0x0) ? body : "");

    return sys_detached_spawn(argv);
}

uint32_t ldg_sys_notify_close(void)
{
    return LDG_ERR_AOK;
}

uint8_t ldg_sys_focus_is(void)
{
    return LDG_ERR_UNSUPPORTED;
}
