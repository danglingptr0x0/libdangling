# libdangling

libdangling is my personal util lib. I grew tired of writing the same things over and over or copying chunks of code, so I wrote this lib to solve that problem. it largely follows C99 and POSIX.1-2008, and it runs on both the AMD64 and the i386. there's a GNU/Linux-native build and a windows (sadly, some of my work requires windows) cross-compile via mingw-w64 (i386 ships a freestanding 32-bit subset). it serves as the canonical source of types, err codes, mem, threading, strings, timing, timers, scheduling, an entity-component-system, net, audio, gpu compute and graphics, math (dense and sparse linear algebra, iterative solvers, graph algorithms), parsing, binary protocols, compression, I/O, and system utilities for all my projects.

conforms to [dangstd 2.17](https://github.com/danglingptr0x0/dangstd).

## build

the build requires CMake 3.16+, NASM, and pkg-config.

```sh
cmake -B build
cmake --build build -j
doas cmake --install build
```

the Windows cross-compile uses mingw-w64:

```sh
cmake -B build-win -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_ASM_NASM_COMPILER=nasm
cmake --build build-win -j
```

the i386 cross-compile produces a freestanding, statically linked 32-bit subset (the four 32-bit `ldg_arith_*` subroutines, mem, and thread sync):

```sh
cmake -B build-i386 -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=i386 -DCMAKE_C_COMPILER=gcc -DCMAKE_C_FLAGS="-m32 -static"
cmake --build build-i386 -j
```

build flags control the optional deps, and each one defaults to `ON`:

`LDG_WITH_AUDIO` enables pw/alsa on Linux and WASAPI on Windows.
`LDG_WITH_NET` enables libcurl on Linux and WinHTTP on Windows.
`LDG_WITH_FMT` embeds the uncrustify cfg.
`LDG_WITH_GPU` enables Vulkan compute and graphics.

this strips everything optional:

```sh
cmake -B build -DLDG_WITH_AUDIO=OFF -DLDG_WITH_NET=OFF -DLDG_WITH_FMT=OFF -DLDG_WITH_GPU=OFF
```

after install, query the lib with `pkg-config --cflags --libs dangling`.

## API

the API sits at level `DANGLING_4.0`. `symbols.txt` records the authoritative surface: it lists 453 exported subroutines, 1 data sym, 54 inline subroutines, 111 types, and ~331 macros. `libdangling.map` enforces sym visibility at link time through `-fvisibility=hidden` and a GNU ld version script, so the `.so` exports only `LDG_EXPORT`-marked syms.

the ABI ck requires `abi-dumper` and `abi-compliance-checker`:

```sh
cmake -B build -DSTD_DEBUG=ON && cmake --build build
make -C build abi-dump
make -C build abi-check
```

## modules

### core

`core/types.h` defines `ldg_byte_t`, `ldg_word_t`, `ldg_dword_t`, and `ldg_qword_t`, plus the short aliases `byte_t`, `word_t`, `dword_t`, and `qword_t`.

`core/err.h` reserves err codes 0-690 for libdangling and leaves 700-990 for projects; every subroutine returns `uint32_t`, and `LDG_ERR_AOK` equals 0. the macros `LDG_ERRLOG_ERR/WARN/INFO()` log diagnostics.

`core/macros.h` supplies `LDG_LIKELY/UNLIKELY`, `LDG_KIB/MIB/GIB`, `LDG_MS_PER_SEC`, `LDG_NS_PER_SEC`, `LDG_STRUCT_ZERO_INIT`, `LDG_AMD64_CACHE_LINE_WIDTH`, and `LDG_ALIGNED_UP/DOWN()`.

`core/bits.h` supplies `LDG_BYTE_BITS/MASK`, `LDG_NIBBLE_BITS/MASK`, and `LDG_IS_POW2()`.

`core/arith.h` implements overflow-checked arithmetic in hand-written assembly; `ldg_arith_32/64_add/sub/mul/div()` write their result through an out-param and return `uint32_t`. overflow detection rides the hardware carry flag, and each divide guards against a zero divisor before it runs. amd64 provides all eight subroutines (System V and Microsoft x64), and i386 provides the four 32-bit subroutines only.

### mem

`mem/alloc.h` provides a tracked allocator with sentinel guards, leak detection, and pool allocation in fixed-size and variable-size modes. it calls `exit()` if the caller uses it before init.

```c
ldg_mem_init();
void *buff = 0x0;
ldg_mem_alloc(1024, &buff);
ldg_mem_dealloc(buff);
ldg_mem_leaks_dump();
ldg_mem_shutdown();

ldg_mem_pool_t *pool = 0x0;
job_t *job = 0x0;
ldg_mem_pool_create(sizeof(job_t), 256, &pool);
ldg_mem_pool_alloc(pool, sizeof(job_t), (void **)&job);
ldg_mem_pool_dealloc(pool, job);
ldg_mem_pool_destroy(&pool);

ldg_mem_pool_t *var_pool = 0x0;
void *hdr = 0x0;
void *payload = 0x0;
ldg_mem_pool_create(0, 4096, &var_pool);
ldg_mem_pool_alloc(var_pool, 128, &hdr);
ldg_mem_pool_alloc(var_pool, 64, &payload);
ldg_mem_pool_rst(var_pool);
ldg_mem_pool_destroy(&var_pool);
```

`mem/secure.h` provides constant-time ops; `ldg_mem_secure_zero/copy/cmp/cmov/neq_is()` all return `uint32_t`, and amd64 implements them in NASM.

### str

`str/str.h` provides bounded copy (`ldg_strrbrcpy`), hex and decimal conversion (`ldg_str_to_dec`, `ldg_hex_to_dword`, `ldg_hex_to_bytes`, `ldg_byte_to_hex`, `ldg_dword_to_hex`), char predicates (`ldg_char_digit_is`, `ldg_char_alpha_is`, `ldg_char_hex_is`), and reusable FNV-1a hashing (`ldg_hash_fnv1a_32_compute`, `ldg_hash_fnv1a_64_compute`).

### time

`time/time.h` exposes `ldg_time_epoch_ms_get()`, `ldg_time_epoch_ns_get()`, and `ldg_time_monotonic_get(double *out)`.

`time/perf.h` measures frame timing through `ldg_time_ctx_t`; the caller invokes `ldg_time_tick()` per frame and then reads `ldg_time_dt_get(ctx, &out)`, `ldg_time_fps_get(ctx, &out)`, and `ldg_time_frame_cunt_get()`. `ldg_tsc_ctx_t` calibrates and samples the TSC.

`time/timer.h` runs a hashed timer wheel (1024 buckets, up to 4096 timers); `ldg_time_timer_create()` arms one-shot or repeating timers, `ldg_time_timer_wheel_tick()` advances the wheel and fires due callbacks, and `ldg_time_timer_destroy/rst/interval_set()` manage them.

### thread

`thread/sync.h` wraps `ldg_mut_t`, `ldg_cond_t`, and `ldg_sem_t`; all support process-shared mode on Linux and map to CRITICAL_SECTION, CONDITION_VARIABLE, and Win32 semaphores on Windows. `ldg_cond_bcast()`, `ldg_cond_sig()`, and `ldg_cond_timedwait()` return `uint32_t`.

`thread/spsc.h` implements a lock-free SPSC queue with fixed capacity, arbitrary item size, and bounds-checked buffer access.

`thread/mpmc.h` implements a lock-free MPMC queue; it coordinates by sequence number, blocks with a timeout, and bounds both the CAS spin (1024 iters) and the wait loop (4096 iters).

```c
ldg_mpmc_queue_t queue;
ldg_mpmc_init(&queue, sizeof(job_t), 256);
ldg_mpmc_push(&queue, &job);
ldg_mpmc_wait(&queue, &out, 5000);
ldg_mpmc_shutdown(&queue);
```

`thread/pool.h` runs a thread pool in two modes: long-running workers via `ldg_thread_pool_start`, or job submission via `ldg_thread_pool_submit` (backed by MPMC). the two modes exclude each other.

`thread/yield.h` offers `ldg_thread_yield(uint64_t ns)` for ns-granularity sleep. Linux calls `nanosleep` and returns `LDG_ERR_INTERRUPTED` on signal; Windows calls `Sleep`, rounds a sub-ms request up to 1ms, and treats `ns == 0` as a no-op.

### io

`io/file.h` provides `ldg_io_file_open/close/rd/wr/seek/sync/truncate/lock/unlock/dup()`, `ldg_io_pipe_create()`, and `ldg_io_file_stat/fstat()`.

`io/dir.h` provides `ldg_io_dir_create/destroy/open/rd/close()`, `ldg_io_dirent_name_get()`, and `ldg_io_dirent_dir_is()`.

`io/path.h` provides `ldg_io_path_rename/unlink/exists_is/join/resolve/expand/normalize()`, basename, dirname, and ext extraction, symlink create and read, and home and tmp dir getters.

### sys

`sys/info.h` exposes `ldg_sys_hostname_get()`, `ldg_sys_cpu_cunt_get()`, `ldg_sys_page_size_get()`, `ldg_sys_env_get()`, and `ldg_sys_pid_get()`.

`sys/tty.h` exposes `ldg_sys_tty_stdout_is()` and `ldg_sys_tty_width_get()`.

`sys/uuid.h` exposes `ldg_sys_uuid_gen()` and `ldg_sys_uuid_to_str()`.

### net

`net/curl.h` houses cURL `multi` (concurrent transfers with progress via `ldg_curl_multi_progress_get(ctx, &out)`) and `easy` (single transfer with a streaming cb) interfaces, and it offers hdr-list helpers. on Windows the backend uses WinHTTP and drops the libcurl and OpenSSL dependency while it keeps the same API.

`net/gql.h` provides a GraphQL client; `ldg_gql_ctx_create/destroy()` and `ldg_gql_exec()` wrap cURL easy for a JSON POST to a GQL endpoint.

### audio

`audio/audio.h` prefers pipewire and falls back to alsa on Linux, and it uses WASAPI on Windows. it controls master volume and per-stream volume and mute, enumerates sinks and sources, registers a self-stream, and stacks ducking requests.

### gpu

`gpu/gpu.h` exposes Vulkan 1.2 compute and graphics through an opaque API that keeps `vulkan.h` out of the public hdr. the caller owns the ctx, which `ldg_gpu_init` allocates and `ldg_gpu_shutdown` frees, so the module holds no globals. it selects a device automatically (it prefers a discrete GPU and falls back to integrated, or it honors an explicit idx). a slab-based GPU mem suballocator spills from VRAM to host memory when the caller sets `LDG_GPU_FLAG_SPILL_ENABLE`. staging transfers feed device-local buffs, and host-visible buffs map directly. `ldg_gpu_buff_fill` fills on the GPU side (it wraps `vkCmdFillBuffer` with no staging and no CPU upload). a generic 8-binding descriptor layout binds partially (Vk 1.2 core, `VK_SHADER_STAGE_ALL`). the dispatch path runs sync or async with a full fence lifecycle. debug builds enable the validation layers, and the loader reads SPIR-V from files or from embedded data.

a compute dispatch runs a pipeline over a buffer:

```c
ldg_gpu_init_desc_t init_desc = { .dev_idx = UINT32_MAX, .flags = LDG_GPU_FLAG_SPILL_ENABLE };
void *gpu = 0x0;
ldg_gpu_init(&init_desc, &gpu);

ldg_gpu_buff_desc_t buff_desc = { .size = 4096 };
ldg_gpu_buff_t buff = { 0 };
ldg_gpu_buff_create(gpu, &buff_desc, &buff);
ldg_gpu_buff_wr(gpu, buff.id, src_data, 4096, 0);

uint32_t *spirv_code = 0x0;
uint64_t spirv_size = 0;
ldg_gpu_spirv_file_load("shader.spv", &spirv_code, &spirv_size);
ldg_gpu_spirv_desc_t spirv_desc = { .code = spirv_code, .code_size = spirv_size, .entry_name = "main" };
uint32_t pipeline = 0;
ldg_gpu_pipeline_create(gpu, &spirv_desc, &pipeline);
ldg_gpu_spirv_file_free(spirv_code);

ldg_gpu_dispatch_desc_t dispatch_desc = { .pipeline_id = pipeline, .group_cunt_x = 1, .group_cunt_y = 1, .group_cunt_z = 1, .buff_ids = { buff.id }, .buff_cunt = 1 };
ldg_gpu_dispatch(gpu, &dispatch_desc);

ldg_gpu_buff_rd(gpu, buff.id, readback, 4096, 0);
ldg_gpu_buff_destroy(gpu, buff.id);
ldg_gpu_pipeline_destroy(gpu, pipeline);
ldg_gpu_shutdown(gpu);
```

the graphics path covers surface, swapchain, renderpass, pipeline, and frame. the caller creates a `VkSurfaceKHR` (via GLFW or an equivalent) and hands it to `ldg_gpu_surface_create`. the swapchain owns the framebuffers and an optional depth image. the renderer keeps 3 frames in flight with per-frame fences and semaphores, exposes 128B push constants across the vert and frag stages, and shares one pipeline registry (compute and graphics, 64 slots, tagged by kind). `instance_extensions` in `ldg_gpu_init_desc_t` carries the WSI extensions.

```c
uint32_t extension_cunt = 0;
const char **extensions = glfwGetRequiredInstanceExtensions(&extension_cunt);
ldg_gpu_init_desc_t init_desc = { .dev_idx = UINT32_MAX, .instance_extensions = extensions, .instance_extension_cunt = extension_cunt };
void *gpu = 0x0;
ldg_gpu_init(&init_desc, &gpu);

void *instance = 0x0;
ldg_gpu_instance_get(gpu, &instance);
VkSurfaceKHR vk_surface = 0x0;
glfwCreateWindowSurface(instance, window, 0x0, &vk_surface);
ldg_gpu_surface_t surface = { 0 };
ldg_gpu_surface_create(gpu, (void *)vk_surface, &surface);

ldg_gpu_swapchain_desc_t swapchain_desc = { .surface_id = surface.id, .w = 1280, .h = 720, .preferred_img_cunt = 3, .present_mode = LDG_GPU_PRESENT_MAILBOX };
ldg_gpu_swapchain_t swapchain = { 0 };
ldg_gpu_swapchain_create(gpu, &swapchain_desc, &swapchain);

ldg_gpu_renderpass_desc_t renderpass_desc = { .color_fmt = LDG_GPU_FMT_B8G8R8A8_SRGB, .load_clear = 1 };
uint32_t renderpass = 0;
ldg_gpu_renderpass_create(gpu, &renderpass_desc, &renderpass);

ldg_gpu_gfx_pipeline_desc_t pipeline_desc = { .vert = vert_spirv, .frag = frag_spirv, .renderpass_id = renderpass, .vert_stride = 20, .vert_attr_cunt = 2, .vert_attrs = { { 0, 0, LDG_GPU_FMT_R32G32_SFLOAT }, { 1, 8, LDG_GPU_FMT_R32G32B32_SFLOAT } }, .topology = LDG_GPU_TOPOLOGY_TRI_LIST };
uint32_t gfx_pipeline = 0;
ldg_gpu_gfx_pipeline_create(gpu, &pipeline_desc, &gfx_pipeline);

uint32_t img_idx = 0;
ldg_gpu_swapchain_img_acquire(gpu, swapchain.id, &img_idx);
ldg_gpu_frame_t frame = { 0 };
ldg_gpu_frame_begin(gpu, swapchain.id, &frame);
double clear[4] = { 0.02, 0.02, 0.03, 1.0 };
ldg_gpu_frame_renderpass_begin(gpu, &frame, renderpass, clear, 1.0);
ldg_gpu_frame_pipeline_bind(gpu, &frame, gfx_pipeline);
ldg_gpu_frame_vert_buff_bind(gpu, &frame, vert_buff.id);
ldg_gpu_frame_push_const(gpu, &frame, 0, 64, &mvp);
ldg_gpu_frame_draw(gpu, &frame, 3, 1);
ldg_gpu_frame_renderpass_end(gpu, &frame);
ldg_gpu_frame_end(gpu, &frame);
ldg_gpu_swapchain_present(gpu, swapchain.id, img_idx);
```

### math

`math/math.h` defines the math subsystem's own err codes (`LDG_MATH_OK` through `LDG_MATH_DIM_MISMATCH`) and constants (`LDG_MATH_PI`, `LDG_MATH_TAU`, `LDG_MATH_DEG_TO_RAD`, `LDG_MATH_RAD_TO_DEG`).

`math/vec.h` provides header-only dense vector ops (`ldg_math_vec_zero/set/cpy/scale/axpy/sub/mul_elem`, dot, and the L1, L2, and inf norms); it stays `static inline` and depends only on the math header.

`math/sparse.h` stores a CSR sparse matrix; `ldg_math_sparse_create/entry_add/finalize` assemble it from triplets, `ldg_math_sparse_mv` multiplies it by a vector, and `ldg_math_sparse_diag_get/symmetric_is/nnz_get` query it.

`math/krylov.h` solves sparse symmetric systems iteratively; `ldg_math_krylov_cg_solve` runs conjugate gradient and `ldg_math_krylov_pcg_solve` runs the preconditioned variant (Jacobi or IC0, built by `ldg_math_krylov_precond_build`).

`math/laplace.h` solves the weighted graph Laplacian Dirichlet problem; `ldg_math_laplace_solve` and `ldg_math_laplace_anchor_solve` compute node potentials, while `ldg_math_laplace_resistance_get`, `ldg_math_laplace_tree_cunt_get`, and `ldg_math_laplace_kron_reduce` derive effective resistance, spanning-tree count, and Kron reduction.

`math/astar.h` runs A* over a caller-described graph; `ldg_math_astar_solve` returns a shortest path and `ldg_math_astar_traverse` returns the visit order, both driven by successor and heuristic callbacks.

`math/linalg.h` stays header-only; `ldg_math_vec3_*` covers add, sub, scale, dot, cross, len, and scaled add, while `ldg_math_mat3_*` covers add, sub, mul, vec mul, inv, det, trace, transpose, and polar decomposition. it stays `static inline` and depends only on `<math.h>` and `<stdint.h>`.

### sched

`sched/sched.h` runs a frame-oriented job scheduler. it ticks through phases (start, spin begin, fixed step, step, late step, spin end, stop), runs jobs on a work-stealing thread pool, and orders them with an A*-based planner under a frame deadline. it estimates each job's duration with one of three strategies (manual, a linear SVM in `sched/svm.h`, or a Bayesian model in `sched/bayes.h`) and the caller selects one through `ldg_sched_ctx_est_strat_set`. `sched/event.h` provides a filtered pub-sub event pool, `sched/job.h` provides the job pool, dependency graph, and per-worker deques, and `sched/trace.h` records a TSC-stamped trace ring.

### ecs

`ecs/ecs.h` runs an entity-component-system world. `ecs/entity.h` stores entities in a sparse set with generational handles, `ecs/comp.h` stores each component type in its own sparse set over a pool arena, and `ecs/taxon.h` classifies entities by required, optional, and forbidden component masks. `ecs/query.h` iterates the entities that hold a set of components, and `ecs/cmd.h` records structural changes (entity create and destroy, component add and remove) in a mutex-guarded command buffer that flushes at a safe point. systems declare read and write masks and run either sequentially (`ldg_ecs_ctx_sys_all_seq_run`) or across the scheduler (`ldg_ecs_ctx_sys_all_sched_run`, bridged by `ldg_ecs_sched_init/shutdown`). global rules and forbidden-mask checks validate entities on commit.

### compress

`compress/inflate.h` inflates a zlib stream (DEFLATE under a zlib hdr, with an Adler-32 trailer) without any heap allocation; `ldg_compress_inflate(src, src_len, dst, dst_cap, out_len)` writes the inflated bytes into a caller-supplied buffer and reports the produced length.

### parse

`parse/parse.h` tokenizes on whitespace into `ldg_tok_arr_t` and compares tokens with `ldg_parse_streq_is()`.

### proto

`proto/emiru.h` encodes, decodes, and validates the EMIRU binary fmt, a 32B hdr for user-space executables; its fields cover magic, rev, ring, flags, entry, and the text, data, and bss sizes.

`proto/emiemi.h` frames the EMIEMI transfer protocol as `<<EMIEMI>XXXXXX>..payload..<<EMIEMI>>`; it streams recv through a cb, encodes and decodes whole buffs, checks integrity with FNV-1a, and caps the payload at 16M.

### arch/amd64

`atomic.h` provides `LDG_RD/WR_ONCE`, `LDG_LOAD_ACQUIRE/STORE_RELEASE`, `LDG_CAS`, and `LDG_FETCH_ADD/SUB`.

`fence.h` provides `LDG_MFENCE/SFENCE/LFENCE` and `LDG_SMP_MB/WMB/RMB`.

`prefetch.h` provides `LDG_PREFETCH_R/W/NTA`.

`tsc.h` samples the TSC, serializes reads, and calibrates.

`cpuid.h` runs `ldg_cpuid()` and exposes `ldg_cpuid_feat_get()`, the vendor and brand strings, and the core ID.

`syscall.h` provides `ldg_syscall0` through `ldg_syscall4`.

### arch/misc

`misc/misc.h` encodes, decodes, validates, and disassembles a 32-bit fixed-width ISA: 16 opcodes (cpy, ldd, std, cph, add, sub, and, or, xor, shl, shr, jmp, jnz, call, ret, cpl), 8 registers (dr0 through dr5, dr6 as LOC, dr7 as SP), and a memory map over RAM, CPU control, and IO.

### fmt

`fmt/fmt.h` embeds an uncrustify cfg; `ldg_fmt_cfg_get()` returns the string and `ldg_fmt_cfg_path_get()` returns the install path.

## tests

```sh
make tests-run
```
