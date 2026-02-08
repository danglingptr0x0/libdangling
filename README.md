# libdangling

my personal util lib. I grew tired of constantly having to write things over and over or copy chunks of code, so I wrote this lib to solve that problem. mostly C99-compliant, POSIX.1-2008, AMD64. canonical source of types, err codes, mem, threading, strings, timing, net, audio, math, parsing, and binary protocols for all my projects

conforms to [OC Std 2.0](https://github.com/danglingptr0x0/dangstd)

## build

CMake 3.16+, NASM, [libyder](https://github.com/babelouest/yder), pkg-config

```sh
cmake -B build
cmake --build build -j$(nproc)
sudo cmake --install build
```

optional deps controlled via flags:

| Flag | Default | What |
|------|---------|------|
| `LDG_WITH_AUDIO` | `ON` | PipeWire or ALSA auto-detect |
| `LDG_WITH_NET` | `ON` | libcurl |
| `LDG_WITH_FMT` | `ON` | embedded uncrustify cfg |

strip everything optional:

```sh
cmake -B build -DLDG_WITH_AUDIO=OFF -DLDG_WITH_NET=OFF -DLDG_WITH_FMT=OFF
```

after install: `pkg-config --cflags --libs dangling`

## modules

### core

`core/types.h`: `ldg_byte_t`, `ldg_word_t`, `ldg_dword_t`, `ldg_qword_t` (short aliases: `byte_t`, `word_t`, `dword_t`, `qword_t`)

`core/err.h`: err codes 0-690 (libdangling reserved); 700-990 (projects). every func returns `uint32_t`. `LDG_ERR_AOK` = 0. logging: `LDG_ERRLOG_ERR/WARN/INFO()`

`core/macros.h`: `LDG_LIKELY/UNLIKELY`, `LDG_KIB/MIB/GIB`, `LDG_MS_PER_SEC`, `LDG_NS_PER_SEC`, `LDG_STRUCT_ZERO_INIT`, `LDG_AMD64_CACHE_LINE_WIDTH`, `LDG_ALIGNED_UP/DOWN()`

`core/bits.h`: `LDG_BYTE_BITS/MASK`, `LDG_NIBBLE_BITS/MASK`, `LDG_IS_POW2()`

### mem

`mem/alloc.h`: tracked allocator; sentinel-guarded, leak detection, pool alloc

```c
ldg_mem_init();
void *p = ldg_mem_alloc(1024);
ldg_mem_dealloc(p);
ldg_mem_leaks_dump();
ldg_mem_shutdown();

ldg_mem_pool_t *pool = ldg_mem_pool_create(sizeof(thing_t), 256);
thing_t *t = ldg_mem_pool_alloc(pool);
ldg_mem_pool_dealloc(pool, t);
ldg_mem_pool_destroy(pool);
```

`mem/secure.h`: constant-time ops; `ldg_mem_secure_zero/copy/cmp/cmov/neq_is()`; NASM on x86_64

### str

`str/str.h`: bounded copy (`ldg_strrbrcpy`); hex/dec conversion (`ldg_str_to_dec`, `ldg_hex_to_dword`, `ldg_hex_to_bytes`, `ldg_byte_to_hex`, `ldg_dword_to_hex`); char predicates (`ldg_char_digit_is`, `ldg_char_alpha_is`, `ldg_char_hex_is`)

### time

`time/time.h`: `ldg_time_epoch_ms_get()`, `ldg_time_epoch_ns_get()`, `ldg_time_monotonic_get()`

`time/perf.h`: frame timing via `ldg_time_ctx_t`; call `ldg_time_tick()` per frame, read `ldg_time_dt_get()`, `ldg_time_fps_get()`, `ldg_time_frame_cunt_get()`. TSC calibration via `ldg_tsc_ctx_t`

### thread

`thread/sync.h`: `ldg_mut_t`, `ldg_cond_t`, `ldg_sem_t`; all support process-shared

`thread/spsc.h`: lock-free SPSC queue; fixed capacity, arbitrary item size

`thread/mpmc.h`: lock-free MPMC queue; sequence-based coordination, blocking wait with timeout

```c
ldg_mpmc_queue_t q;
ldg_mpmc_init(&q, sizeof(job_t), 256);
ldg_mpmc_push(&q, &job);
ldg_mpmc_wait(&q, &out, 5000);
ldg_mpmc_shutdown(&q);
```

`thread/pool.h`: thread pool; two modes: long-running workers (`ldg_thread_pool_start`) or job submission (`ldg_thread_pool_submit`, backed by MPMC)

### net

`net/curl.h`: libcurl multi (concurrent + progress) and easy (single + streaming cb) interfaces; hdr list helpers

### audio

`audio/audio.h`: PipeWire preferred, ALSA fallback; master volume, per-stream volume/mute, sink/source enumeration, self-stream registration, stacked ducking

### math

`math/linalg.h`: header-only; `ldg_vec3_*` (add, sub, scale, dot, cross, length, `scaled_add`); `ldg_mat3_*` (add, sub, mul, `vec_mul`, inv, det, trace, transpose, polar decomposition). `static inline`, depends only on `<math.h>` and `<stdint.h>`

### parse

`parse/parse.h`: whitespace tokenizer into `ldg_tok_arr_t`; equality: `ldg_parse_streq_is()`

### proto

`proto/emiru.h`: EMIRU binary fmt; 32B hdr for user-space executables; encode, decode, validate. fields: magic, rev, ring, flags, entry, text/data/bss sizes

`proto/emiemi.h`: EMIEMI transfer protocol; `<<EMIEMI>XXXXXX>..payload..<<EMIEMI>>` framing; streaming recv via cb, buff encode/decode, FNV-1a integrity. max payload 16M

### arch/x86_64

`atomic.h`: `LDG_READ/WRITE_ONCE`, `LDG_LOAD_ACQUIRE/STORE_RELEASE`, `LDG_CAS`, `LDG_FETCH_ADD/SUB`

`fence.h`: `LDG_MFENCE/SFENCE/LFENCE`, `LDG_SMP_MB/WMB/RMB`

`prefetch.h`: `LDG_PREFETCH_R/W/NTA`

`tsc.h`: TSC sampling, serialized reads, calibration

`cpuid.h`: `ldg_cpuid()`; feature detection, vendor/brand, core ID

`syscall.h`: `ldg_syscall0` through `ldg_syscall4`

### fmt

`fmt/fmt.h`: embedded uncrustify cfg; `ldg_fmt_cfg_get()` returns the string; `ldg_fmt_cfg_path_get()` returns the install path

### log

`log/log.h`: `LDG_LOG_DEBUG/INFO/WARNING/ERROR()` over libyder

## tests

QEMU/KVM only, not host; see [OC Std 2.0 SS5.10](https://github.com/danglingptr0x0/dangstd)

```sh
make tests-run
```
