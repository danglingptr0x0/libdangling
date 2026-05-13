# libdangling

my personal util lib. I grew tired of constantly having to write things over and over or copy chunks of code, so I wrote this lib to solve that problem. C99, POSIX.1-2008, AMD64. dual-platform (Linux native, Windows cross-compile via mingw-w64). canonical source of types, err codes, mem, threading, strings, timing, net, audio, gpu compute + graphics, math, parsing, binary protocols, I/O, and system utilities for all my projects

conforms to [dangstd 2.5](https://github.com/danglingptr0x0/dangstd)

## build

CMake 3.16+, NASM, pkg-config

```sh
cmake -B build
cmake --build build -j$(nproc)
sudo cmake --install build
```

Windows cross-compile:

```sh
cmake -B build-win -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_ASM_NASM_COMPILER=nasm
cmake --build build-win -j$(nproc)
```

optional deps controlled via flags:

| Flag | Default | What |
|------|---------|------|
| `LDG_WITH_AUDIO` | `ON` | pw/ALSA (Linux), WASAPI (Windows) |
| `LDG_WITH_NET` | `ON` | `libcurl` |
| `LDG_WITH_FMT` | `ON` | embedded uncrustify cfg |
| `LDG_WITH_GPU` | `ON` | Vulkan compute + graphics |

strip everything optional:

```sh
cmake -B build -DLDG_WITH_AUDIO=OFF -DLDG_WITH_NET=OFF -DLDG_WITH_FMT=OFF -DLDG_WITH_GPU=OFF
```

after install: `pkg-config --cflags --libs dangling`

## API

API lvl `DANGLING_2.0`. `symbols.txt` is the authoritative surface: 252 exported subroutines, 1 data sym, 46 inline subroutines, 53 types, ~290 macros. `libdangling.map` enforces sym vis at lnk time (`-fvisibility=hidden` + GNU ld version script). only `LDG_EXPORT`-marked syms are exported from the `.so`

ABI ck (requires `abi-dumper` and `abi-compliance-checker`):

```sh
cmake -B build -DSTD_DEBUG=ON && cmake --build build
make -C build abi-dump
make -C build abi-check
```

## modules

### core

`core/types.h`: `ldg_byte_t`, `ldg_word_t`, `ldg_dword_t`, `ldg_qword_t` (short aliases: `byte_t`, `word_t`, `dword_t`, `qword_t`)

`core/err.h`: err codes 0-690 (libdangling reserved); 700-990 (projects). every subroutine rets `uint32_t`. `LDG_ERR_AOK` = 0. logging: `LDG_ERRLOG_ERR/WARN/INFO()`

`core/macros.h`: `LDG_LIKELY/UNLIKELY`, `LDG_KIB/MIB/GIB`, `LDG_MS_PER_SEC`, `LDG_NS_PER_SEC`, `LDG_STRUCT_ZERO_INIT`, `LDG_AMD64_CACHE_LINE_WIDTH`, `LDG_ALIGNED_UP/DOWN()`

`core/bits.h`: `LDG_BYTE_BITS/MASK`, `LDG_NIBBLE_BITS/MASK`, `LDG_IS_POW2()`

`core/arith.h`: overflow-checked arithmetic; `ldg_arith_32/64_add/sub/mul/div()`; inline, uses `__builtin_*_overflow`

### mem

`mem/alloc.h`: tracked allocator; sentinel-guarded, leak detection, pool alloc (fixed-size and variable-size). `exit()`s if subsystem not init

```c
ldg_mem_init();
void *p = 0x0;
ldg_mem_alloc(1024, &p);
ldg_mem_dealloc(p);
ldg_mem_leaks_dump();
ldg_mem_shutdown();

ldg_mem_pool_t *pool = 0x0;
thing_t *t = 0x0;
ldg_mem_pool_create(sizeof(thing_t), 256, &pool);
ldg_mem_pool_alloc(pool, sizeof(thing_t), (void **)&t);
ldg_mem_pool_dealloc(pool, t);
ldg_mem_pool_destroy(&pool);

ldg_mem_pool_t *vpool = 0x0;
void *a = 0x0;
void *b = 0x0;
ldg_mem_pool_create(0, 4096, &vpool);
ldg_mem_pool_alloc(vpool, 128, &a);
ldg_mem_pool_alloc(vpool, 64, &b);
ldg_mem_pool_rst(vpool);
ldg_mem_pool_destroy(&vpool);
```

`mem/secure.h`: constant-time ops; `ldg_mem_secure_zero/copy/cmp/cmov/neq_is()` (all ret `uint32_t`); NASM on amd64

### str

`str/str.h`: bounded copy (`ldg_strrbrcpy`); hex/dec conversion (`ldg_str_to_dec`, `ldg_hex_to_dword`, `ldg_hex_to_bytes`, `ldg_byte_to_hex`, `ldg_dword_to_hex`); char predicates (`ldg_char_digit_is`, `ldg_char_alpha_is`, `ldg_char_hex_is`)

### time

`time/time.h`: `ldg_time_epoch_ms_get()`, `ldg_time_epoch_ns_get()`, `ldg_time_monotonic_get(double *out)`

`time/perf.h`: frame timing via `ldg_time_ctx_t`; call `ldg_time_tick()` per frame, read `ldg_time_dt_get(ctx, &out)`, `ldg_time_fps_get(ctx, &out)`, `ldg_time_frame_cunt_get()`. TSC calibration via `ldg_tsc_ctx_t`

### thread

`thread/sync.h`: `ldg_mut_t`, `ldg_cond_t`, `ldg_sem_t`; all support process-shared (Linux); CRITICAL_SECTION/CONDITION_VARIABLE/Win32 semaphores (Windows). `ldg_cond_bcast()`, `ldg_cond_sig()`, `ldg_cond_timedwait()` all ret `uint32_t`

`thread/spsc.h`: lock-free SPSC queue; fixed capacity, arbitrary item size; bounds-checked buffer access

`thread/mpmc.h`: lock-free MPMC queue; sequence-based coordination, blocking wait with timeout, bounded CAS spin (1024 iters), bounded wait loop (4096 iters)

```c
ldg_mpmc_queue_t q;
ldg_mpmc_init(&q, sizeof(job_t), 256);
ldg_mpmc_push(&q, &job);
ldg_mpmc_wait(&q, &out, 5000);
ldg_mpmc_shutdown(&q);
```

`thread/pool.h`: thread pool; two modes: long-running workers (`ldg_thread_pool_start`) or job submission (`ldg_thread_pool_submit`, backed by MPMC). `start()` and `submit()` are mutually exclusive

`thread/yield.h`: `ldg_thread_yield(uint64_t ns)`; ns-granularity sleep. Linux: `nanosleep`, rets `LDG_ERR_INTERRUPTED` on signal. Windows: `Sleep`, ms granularity, sub-ms rounds up to 1ms. `ns == 0` is a no-op

### io

`io/file.h`: `ldg_io_file_open/close/rd/wr/seek/sync/truncate/lock/unlock/dup()`; `ldg_io_pipe_create()`, `ldg_io_file_stat/fstat()`

`io/dir.h`: `ldg_io_dir_create/destroy/open/rd/close()`; `ldg_io_dirent_name_get()`, `ldg_io_dirent_dir_is()`

`io/path.h`: `ldg_io_path_rename/unlink/exists_is/join/resolve/expand/normalize()`; basename, dirname, ext extraction; symlink create/read; home/tmp dir getters

### sys

`sys/info.h`: `ldg_sys_hostname_get()`, `ldg_sys_cpu_cunt_get()`, `ldg_sys_page_size_get()`, `ldg_sys_env_get()`, `ldg_sys_pid_get()`

`sys/tty.h`: `ldg_sys_tty_stdout_is()`, `ldg_sys_tty_width_get()`

`sys/uuid.h`: `ldg_sys_uuid_gen()`, `ldg_sys_uuid_to_str()`

### net

`net/curl.h`: cURL multi (concurrent + progress via `ldg_curl_multi_progress_get(ctx, &out)`) and easy (single + streaming cb) interfaces; hdr list helpers

`net/gql.h`: GraphQL client; `ldg_gql_ctx_create/destroy()`, `ldg_gql_exec()`. wraps cURL easy for JSON POST to a GQL endpoint

### audio

`audio/audio.h`: PipeWire preferred, ALSA fallback (Linux); WASAPI (Windows). master volume, per-stream volume/mute, sink/source enumeration, self-stream registration, stacked ducking

### gpu

`gpu/gpu.h`: Vulkan 1.2 compute + graphics; opaque API, no `vulkan.h` in public hdr. ctx is caller-owned (alloc'd by `ldg_gpu_init`, freed by `ldg_gpu_shutdown`); no globals. auto device selection (prefers discrete, falls back integrated; or explicit idx). slab-based GPU mem suballocator with VRAM-to-host spillover (`LDG_GPU_FLAG_SPILL_ENABLE`). staging transfers for device-local buffs, direct map for host-visible. GPU-side fill via `ldg_gpu_buff_fill` (wraps `vkCmdFillBuffer`; no staging, no CPU upload). generic 8-binding descriptor layout (partially bound, Vk 1.2 core, `VK_SHADER_STAGE_ALL`). sync + async dispatch with fence lifecycle. validation layers in debug builds. SPIR-V from files or embedded

compute:

```c
ldg_gpu_init_desc_t desc = { .dev_idx = UINT32_MAX, .flags = LDG_GPU_FLAG_SPILL_ENABLE };
void *gpu = 0x0;
ldg_gpu_init(&desc, &gpu);

ldg_gpu_buff_desc_t bd = { .size = 4096 };
ldg_gpu_buff_t buff = { 0 };
ldg_gpu_buff_create(gpu, &bd, &buff);
ldg_gpu_buff_wr(gpu, buff.id, data, 4096, 0);

uint32_t *spv = 0x0;
uint64_t spv_size = 0;
ldg_gpu_spirv_file_load("shader.spv", &spv, &spv_size);
ldg_gpu_spirv_desc_t spirv = { .code = spv, .code_size = spv_size, .entry_name = "main" };
uint32_t pipeline = 0;
ldg_gpu_pipeline_create(gpu, &spirv, &pipeline);
ldg_gpu_spirv_file_free(spv);

ldg_gpu_dispatch_desc_t dd = { .pipeline_id = pipeline, .group_cunt_x = 1, .group_cunt_y = 1, .group_cunt_z = 1, .buff_ids = { buff.id }, .buff_cunt = 1 };
ldg_gpu_dispatch(gpu, &dd);

ldg_gpu_buff_rd(gpu, buff.id, out, 4096, 0);
ldg_gpu_buff_destroy(gpu, buff.id);
ldg_gpu_pipeline_destroy(gpu, pipeline);
ldg_gpu_shutdown(gpu);
```

graphics (surface/swapchain/renderpass/pipeline/frame): caller creates `VkSurfaceKHR` via GLFW (or equivalent), hands it to `ldg_gpu_surface_create`. swapchain owns framebuffers + optional depth image. 3 frames-in-flight with per-frame fences + semaphores. push constants (128B, vert+frag). shared pipeline registry (compute + graphics, 64 slots, tagged by kind). `instance_extensions` in `ldg_gpu_init_desc_t` for WSI extensions

```c
uint32_t ext_cunt = 0;
const char **exts = glfwGetRequiredInstanceExtensions(&ext_cunt);
ldg_gpu_init_desc_t desc = { .dev_idx = UINT32_MAX, .instance_extensions = exts, .instance_extension_cunt = ext_cunt };
void *gpu = 0x0;
ldg_gpu_init(&desc, &gpu);

void *instance = 0x0;
ldg_gpu_instance_get(gpu, &instance);
VkSurfaceKHR vk_surface = 0x0;
glfwCreateWindowSurface(instance, window, 0x0, &vk_surface);
ldg_gpu_surface_t surface = { 0 };
ldg_gpu_surface_create(gpu, (void *)vk_surface, &surface);

ldg_gpu_swapchain_desc_t sc_desc = { .surface_id = surface.id, .w = 1280, .h = 720, .preferred_image_cunt = 3, .present_mode = LDG_GPU_PRESENT_MAILBOX };
ldg_gpu_swapchain_t swapchain = { 0 };
ldg_gpu_swapchain_create(gpu, &sc_desc, &swapchain);

ldg_gpu_renderpass_desc_t rp_desc = { .color_fmt = LDG_GPU_FMT_B8G8R8A8_SRGB, .load_clear = 1 };
uint32_t renderpass = 0;
ldg_gpu_renderpass_create(gpu, &rp_desc, &renderpass);

ldg_gpu_gfx_pipeline_desc_t pipeline_desc = { .vert = vert_spirv, .frag = frag_spirv, .renderpass_id = renderpass, .vertex_stride = 20, .vertex_attr_cunt = 2, .vertex_attrs = { { 0, 0, LDG_GPU_FMT_R32G32_SFLOAT }, { 1, 8, LDG_GPU_FMT_R32G32B32_SFLOAT } }, .topology = LDG_GPU_TOPOLOGY_TRI_LIST };
uint32_t gfx_pipeline = 0;
ldg_gpu_gfx_pipeline_create(gpu, &pipeline_desc, &gfx_pipeline);

uint32_t image_idx = 0;
ldg_gpu_swapchain_image_acquire(gpu, swapchain.id, &image_idx);
ldg_gpu_frame_t frame = { 0 };
ldg_gpu_frame_begin(gpu, swapchain.id, &frame);
double clear[4] = { 0.02, 0.02, 0.03, 1.0 };
ldg_gpu_frame_renderpass_begin(gpu, &frame, renderpass, clear, 1.0);
ldg_gpu_frame_pipeline_bind(gpu, &frame, gfx_pipeline);
ldg_gpu_frame_vertex_buff_bind(gpu, &frame, vbo.id);
ldg_gpu_frame_push_const(gpu, &frame, 0, 64, &mvp);
ldg_gpu_frame_draw(gpu, &frame, 3, 1);
ldg_gpu_frame_renderpass_end(gpu, &frame);
ldg_gpu_frame_end(gpu, &frame);
ldg_gpu_swapchain_present(gpu, swapchain.id, image_idx);
```

### math

`math/linalg.h`: header-only; `ldg_vec3_*` (add, sub, scale, dot, cross, length, `scaled_add`); `ldg_mat3_*` (add, sub, mul, `vec_mul`, inv, det, trace, transpose, polar decomposition). `static inline`, depends only on `<math.h>` and `<stdint.h>`

### parse

`parse/parse.h`: whitespace tokenizer into `ldg_tok_arr_t`; equality: `ldg_parse_streq_is()`

### proto

`proto/emiru.h`: EMIRU binary fmt; 32B hdr for user-space executables; encode, decode, validate. fields: magic, rev, ring, flags, entry, text/data/bss sizes

`proto/emiemi.h`: EMIEMI transfer protocol; `<<EMIEMI>XXXXXX>..payload..<<EMIEMI>>` framing; streaming recv via cb, buff encode/decode, FNV-1a integrity. max payload 16M

### arch/amd64

`atomic.h`: `LDG_READ/WRITE_ONCE`, `LDG_LOAD_ACQUIRE/STORE_RELEASE`, `LDG_CAS`, `LDG_FETCH_ADD/SUB`

`fence.h`: `LDG_MFENCE/SFENCE/LFENCE`, `LDG_SMP_MB/WMB/RMB`

`prefetch.h`: `LDG_PREFETCH_R/W/NTA`

`tsc.h`: TSC sampling, serialized reads, calibration

`cpuid.h`: `ldg_cpuid()`; `ldg_cpuid_feat_get()`, vendor/brand, core ID

`syscall.h`: `ldg_syscall0` through `ldg_syscall4`

### arch/misc

`misc/misc.h`: 32-bit fixed-width ISA; 16 opcodes: cpy; ldd; std; cph; add; sub; and; or; xor; shl; shr; jmp; jnz; call; ret; and cpl; 8 regs: dr0; dr1; dr2; dr3; dr4; dr5; dr6 (LOC); and dr7 (SP); memory map (RAM/CPU ctrl/IO); encode, decode, validate, disassemble

### fmt

`fmt/fmt.h`: embedded uncrustify cfg; `ldg_fmt_cfg_get()` rets the string; `ldg_fmt_cfg_path_get()` rets the install path

## tests

QEMU/KVM only, not host; see [OC Std 2.5 SS5.10](https://github.com/danglingptr0x0/dangstd)

```sh
make tests-run
```
