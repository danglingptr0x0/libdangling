cmake_minimum_required(VERSION 3.16)
project(dangling VERSION 5.0.0 LANGUAGES C ASM_NASM)

list(APPEND CMAKE_MODULE_PATH "$ENV{HOME}/.config/cmake")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(STD_NO_INSTR ON CACHE BOOL "" FORCE)
endif()
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "i386" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "misc32")
    set(STD_NO_INSTR ON CACHE BOOL "" FORCE)
    set(STD_DEBUG ON CACHE BOOL "" FORCE)
endif()
include(std OPTIONAL RESULT_VARIABLE LDG_STD_FILE)
set(STD_PROJ_PREFIX "ldg")

if(NOT LDG_STD_FILE)
    message(STATUS "std: not found; building with CMake defaults")
endif()

if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "i386" AND NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "misc32")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
set(CMAKE_C_VISIBILITY_PRESET hidden)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(_WIN32_WINNT=0x0601)
endif()

include(GNUInstallDirs)

option(LDG_WITH_AUDIO "Build audio subsystem (auto-detects pw or ALSA)" ON)
option(LDG_WITH_NET "Build net subsystem (requires libcurl)" ON)
option(LDG_WITH_FMT "Build fmt config subsys" ON)
option(LDG_WITH_GPU "Build GPU compute subsystem (requires Vulkan)" ON)
option(LDG_WITH_STEAMWORKS "Build Steamworks subsystem (requires vendored steamworks_sdk)" ON)
option(LDG_WITH_STEAMWORKS_APP_TKT "Link sdkencryptedappticket for server-side ticket decrypt" ON)

if(NOT CMAKE_SYSTEM_PROCESSOR)
    if(CMAKE_C_COMPILER MATCHES "x86_64")
        set(CMAKE_SYSTEM_PROCESSOR "x86_64")
    endif()
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(LDG_ARCH "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "i386")
    set(LDG_ARCH "i386")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "misc32")
    set(LDG_ARCH "MISC")
else()
    message(FATAL_ERROR "unsupported arch: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

if(LDG_ARCH STREQUAL "i386")
    set(LDG_PLATFORM "DANGLING")
    set(LDG_WITH_AUDIO OFF)
    set(LDG_WITH_NET OFF)
    set(LDG_WITH_FMT OFF)
    set(LDG_WITH_GPU OFF)
    set(LDG_WITH_STEAMWORKS OFF)
    set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -masm=intel -march=i386 -mno-mmx -mno-sse -mno-sse2 -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-stack-clash-protection -fcf-protection=none -fno-analyzer -fno-builtin -fshort-enums -fpack-struct=1 -fno-omit-frame-pointer -fno-strict-aliasing -fno-delete-null-pointer-checks -fno-common -U_POSIX_C_SOURCE -U_FORTIFY_SOURCE")
elseif(LDG_ARCH STREQUAL "MISC")
    set(LDG_PLATFORM "DANGLING")
    set(LDG_WITH_AUDIO OFF)
    set(LDG_WITH_NET OFF)
    set(LDG_WITH_FMT OFF)
    set(LDG_WITH_GPU OFF)
    set(LDG_WITH_STEAMWORKS OFF)
    string(REPLACE " -fstack-clash-protection" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REPLACE " -fcf-protection=full" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REPLACE " -fstack-protector-strong" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -nostdlibinc -fPIC -fno-stack-protector -fno-builtin -fshort-enums -fno-omit-frame-pointer -fno-strict-aliasing -fno-delete-null-pointer-checks -fno-common -U_POSIX_C_SOURCE -U_FORTIFY_SOURCE")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LDG_PLATFORM "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(LDG_PLATFORM "windows")
else()
    message(FATAL_ERROR "unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

set(LDG_PLATFORM_DIR "src/${LDG_ARCH}/${LDG_PLATFORM}")

if(LDG_PLATFORM STREQUAL "linux" OR LDG_PLATFORM STREQUAL "windows")
    find_package(Threads REQUIRED)
endif()

if(LDG_PLATFORM STREQUAL "linux")
    find_package(PkgConfig REQUIRED)
endif()

set(LDG_OPT_SOURCES "")
set(LDG_OPT_INCLUDE_DIRS "")
set(LDG_OPT_LIBRARIES "")
set(LDG_OPT_DEFINES "")

if(LDG_WITH_NET)
    if(LDG_PLATFORM STREQUAL "windows")
        list(APPEND LDG_OPT_SOURCES ${LDG_PLATFORM_DIR}/net/curl.c)
        list(APPEND LDG_OPT_SOURCES src/none/none/net/gql.c)
        list(APPEND LDG_OPT_LIBRARIES winhttp)
        list(APPEND LDG_OPT_DEFINES LDG_NET)
        message(STATUS "net: winhttp")
    else()
        if(PkgConfig_FOUND)
            pkg_check_modules(CURL REQUIRED libcurl)
        else()
            find_library(CURL_LIBRARIES NAMES curl libcurl)
            find_path(CURL_INCLUDE_DIRS curl/curl.h)
            if(CURL_LIBRARIES AND CURL_INCLUDE_DIRS)
                set(CURL_FOUND TRUE)
            endif()
        endif()
        if(CURL_FOUND OR CURL_LIBRARIES)
            list(APPEND LDG_OPT_SOURCES ${LDG_PLATFORM_DIR}/net/curl.c)
            list(APPEND LDG_OPT_SOURCES src/none/none/net/gql.c)
            list(APPEND LDG_OPT_INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
            list(APPEND LDG_OPT_LIBRARIES ${CURL_LIBRARIES})
            list(APPEND LDG_OPT_DEFINES LDG_NET)
            if(NOT PkgConfig_FOUND)
                list(APPEND LDG_OPT_DEFINES CURL_STATICLIB)
            endif()
            message(STATUS "net: libcurl")
        else()
            message(STATUS "net: off; libcurl not found")
        endif()
    endif()
else()
    message(STATUS "net: off")
endif()

if(LDG_WITH_STEAMWORKS AND (LDG_PLATFORM STREQUAL "linux" OR LDG_PLATFORM STREQUAL "windows"))
    set(LDG_STEAMWORKS_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/steamworks_sdk/sdk")
    if(LDG_PLATFORM STREQUAL "windows")
        set(LDG_STEAMWORKS_API_LIB "${LDG_STEAMWORKS_SDK_DIR}/redistributable_bin/win64/steam_api64.lib")
        set(LDG_STEAMWORKS_TKT_LIB "${LDG_STEAMWORKS_SDK_DIR}/public/steam/lib/win64/sdkencryptedappticket64.lib")
    else()
        set(LDG_STEAMWORKS_API_LIB "${LDG_STEAMWORKS_SDK_DIR}/redistributable_bin/linux64/libsteam_api.so")
        set(LDG_STEAMWORKS_TKT_LIB "${LDG_STEAMWORKS_SDK_DIR}/public/steam/lib/linux64/libsdkencryptedappticket.so")
    endif()
    if(EXISTS ${LDG_STEAMWORKS_API_LIB})
        list(APPEND LDG_OPT_SOURCES
            src/none/none/net/steamworks.c
            src/none/none/net/steamworks_lobby.c
            src/none/none/net/steamworks_lboard.c
            src/none/none/net/steamworks_econ.c
            src/none/none/net/steamworks_inventory.c
            src/none/none/net/steamworks_cloud.c
            src/none/none/net/steamworks_wshop.c
            src/none/none/net/steamworks_srv_browser.c
            src/none/none/net/steamworks_tkt.c
            src/none/none/net/steamworks_srv.c
        )
        list(APPEND LDG_OPT_LIBRARIES ${LDG_STEAMWORKS_API_LIB})
        list(APPEND LDG_OPT_DEFINES LDG_NET_STEAMWORKS)
        message(STATUS "steamworks: on; sdk vendored; steam_api linked")
        if(LDG_WITH_STEAMWORKS_APP_TKT AND EXISTS ${LDG_STEAMWORKS_TKT_LIB})
            list(APPEND LDG_OPT_SOURCES src/none/none/net/steamworks_srv_tkt.c)
            list(APPEND LDG_OPT_LIBRARIES ${LDG_STEAMWORKS_TKT_LIB})
            list(APPEND LDG_OPT_DEFINES LDG_NET_STEAMWORKS_APP_TKT)
            message(STATUS "steamworks app tkt: on; sdkencryptedappticket linked")
        else()
            message(STATUS "steamworks app tkt: off")
        endif()
    else()
        message(STATUS "steamworks: off; steamworks_sdk not present")
    endif()
else()
    message(STATUS "steamworks: off")
endif()

if(LDG_WITH_FMT)
    list(APPEND LDG_OPT_SOURCES src/none/none/fmt/fmt.c)
    list(APPEND LDG_OPT_DEFINES LDG_FMT)
    message(STATUS "fmt: on")
else()
    message(STATUS "fmt: off")
endif()

if(LDG_WITH_AUDIO)
    if(LDG_PLATFORM STREQUAL "windows")
        set(LDG_AUDIO_BACKEND "wasapi")
        list(APPEND LDG_OPT_SOURCES ${LDG_PLATFORM_DIR}/audio/wasapi.c)
        list(APPEND LDG_OPT_LIBRARIES ole32 oleaut32)
        list(APPEND LDG_OPT_DEFINES LDG_AUDIO_WASAPI)
        message(STATUS "audio: wasapi")
    else()
        pkg_check_modules(PIPEWIRE libpipewire-0.3)
        pkg_check_modules(ALSA alsa)

        if(PIPEWIRE_FOUND)
            set(LDG_AUDIO_BACKEND "pipewire")
            list(APPEND LDG_OPT_SOURCES ${LDG_PLATFORM_DIR}/audio/pw.c)
            list(APPEND LDG_OPT_INCLUDE_DIRS ${PIPEWIRE_INCLUDE_DIRS})
            list(APPEND LDG_OPT_LIBRARIES ${PIPEWIRE_LIBRARIES})
            list(APPEND LDG_OPT_DEFINES LDG_AUDIO_PIPEWIRE)
            message(STATUS "audio: pipewire")
        elseif(ALSA_FOUND)
            set(LDG_AUDIO_BACKEND "alsa")
            list(APPEND LDG_OPT_SOURCES ${LDG_PLATFORM_DIR}/audio/alsa.c)
            list(APPEND LDG_OPT_INCLUDE_DIRS ${ALSA_INCLUDE_DIRS})
            list(APPEND LDG_OPT_LIBRARIES ${ALSA_LIBRARIES})
            list(APPEND LDG_OPT_DEFINES LDG_AUDIO_ALSA)
            message(STATUS "audio: alsa")
        else()
            set(LDG_AUDIO_BACKEND "none")
            message(STATUS "audio: none; libpipewire-0.3 or alsa not found")
        endif()
    endif()
else()
    set(LDG_AUDIO_BACKEND "none")
    message(STATUS "audio: off")
endif()

if(NOT LDG_AUDIO_BACKEND STREQUAL "none")
    list(APPEND LDG_OPT_SOURCES src/none/none/audio/play.c)
endif()

set(LDG_WINRT_INCLUDE_DIR "" CACHE PATH "Windows SDK winrt include directory holding windows.media.control.h")
if(LDG_PLATFORM STREQUAL "windows")
    if(NOT LDG_WINRT_INCLUDE_DIR)
        file(GLOB LDG_WINRT_CANDIDATE_LIST "/mnt/c/Program Files (x86)/Windows Kits/10/Include/*/winrt" "C:/Program Files (x86)/Windows Kits/10/Include/*/winrt")
        list(SORT LDG_WINRT_CANDIDATE_LIST)
        list(REVERSE LDG_WINRT_CANDIDATE_LIST)
        foreach(LDG_WINRT_CANDIDATE ${LDG_WINRT_CANDIDATE_LIST})
            if(EXISTS "${LDG_WINRT_CANDIDATE}/windows.media.control.h")
                set(LDG_WINRT_INCLUDE_DIR "${LDG_WINRT_CANDIDATE}")
                break()
            endif()
        endforeach()
    endif()
    if(NOT LDG_WINRT_INCLUDE_DIR OR NOT EXISTS "${LDG_WINRT_INCLUDE_DIR}/windows.media.control.h")
        message(FATAL_ERROR "windows target needs the Windows SDK winrt headers for system media control; set LDG_WINRT_INCLUDE_DIR to the directory holding windows.media.control.h")
    endif()
    list(APPEND LDG_OPT_INCLUDE_DIRS ${LDG_WINRT_INCLUDE_DIR})
    list(APPEND LDG_OPT_LIBRARIES runtimeobject ole32)
    message(STATUS "sys: media control via winrt; sdk: ${LDG_WINRT_INCLUDE_DIR}")
endif()

if(LDG_WITH_GPU)
    if(PkgConfig_FOUND)
        pkg_check_modules(VULKAN vulkan)
    else()
        find_library(VULKAN_LIBRARIES NAMES vulkan vulkan-1)
        find_path(VULKAN_INCLUDE_DIRS vulkan/vulkan.h)
        if(VULKAN_LIBRARIES AND VULKAN_INCLUDE_DIRS)
            set(VULKAN_FOUND TRUE)
        endif()
    endif()
    if(VULKAN_FOUND OR VULKAN_LIBRARIES)
        list(APPEND LDG_OPT_SOURCES
            src/none/none/gpu/gpu.c
            src/none/none/gpu/surface.c
            src/none/none/gpu/swapchain.c
            src/none/none/gpu/renderpass.c
            src/none/none/gpu/pipeline.c
            src/none/none/gpu/frame.c
        )
        list(APPEND LDG_OPT_INCLUDE_DIRS ${VULKAN_INCLUDE_DIRS})
        list(APPEND LDG_OPT_LIBRARIES ${VULKAN_LIBRARIES})
        list(APPEND LDG_OPT_DEFINES LDG_GPU_VULKAN)
        find_program(LDG_GLSLANG glslangValidator)
        if(NOT LDG_GLSLANG)
            message(FATAL_ERROR "LDG_WITH_GPU=ON with vulkan found needs glslangValidator on PATH for the embedded compute kernels")
        endif()
        set(LDG_SPIRV_NAME_LIST em_electric_rigid em_magnetic_rigid em_electric_cpml em_magnetic_cpml acoustic_prime vol_pack slice_pack atmosphere_transmittance atmosphere_single atmosphere_order atmosphere_gather atmosphere_irradiance atmosphere_cum atmosphere_vol acoustic_velocity acoustic_pressure acoustic_velocity_rigid acoustic_pressure_rigid acoustic_drive_clear acoustic_src_deposit acoustic_tap acoustic_vol_pack acoustic_slice_pack em_port_tap)
        set(LDG_SPIRV_DIR ${CMAKE_BINARY_DIR}/spirv)
        file(MAKE_DIRECTORY ${LDG_SPIRV_DIR})
        set(LDG_SPIRV_EMBED_BODY "#include <stdint.h>\n\n")
        foreach(LDG_SPIRV_NAME ${LDG_SPIRV_NAME_LIST})
            execute_process(
                COMMAND ${LDG_GLSLANG} --target-env vulkan1.2 -o ${LDG_SPIRV_DIR}/${LDG_SPIRV_NAME}.spv ${CMAKE_SOURCE_DIR}/src/none/none/math/phys/shader/${LDG_SPIRV_NAME}.comp
                RESULT_VARIABLE LDG_SPIRV_RES
                OUTPUT_VARIABLE LDG_SPIRV_LOG
                ERROR_VARIABLE LDG_SPIRV_LOG)
            if(NOT LDG_SPIRV_RES EQUAL 0)
                message(FATAL_ERROR "spirv compile failed for ${LDG_SPIRV_NAME}: ${LDG_SPIRV_LOG}")
            endif()
            file(READ ${LDG_SPIRV_DIR}/${LDG_SPIRV_NAME}.spv LDG_SPIRV_HEX HEX)
            string(LENGTH "${LDG_SPIRV_HEX}" LDG_SPIRV_HEX_LEN)
            math(EXPR LDG_SPIRV_BYTE_CUNT "${LDG_SPIRV_HEX_LEN} / 2")
            string(REGEX MATCHALL "........" LDG_SPIRV_CHUNK_LIST "${LDG_SPIRV_HEX}")
            set(LDG_SPIRV_WORD_LIST "")
            foreach(LDG_SPIRV_CHUNK ${LDG_SPIRV_CHUNK_LIST})
                string(SUBSTRING "${LDG_SPIRV_CHUNK}" 0 2 LDG_SPIRV_B0)
                string(SUBSTRING "${LDG_SPIRV_CHUNK}" 2 2 LDG_SPIRV_B1)
                string(SUBSTRING "${LDG_SPIRV_CHUNK}" 4 2 LDG_SPIRV_B2)
                string(SUBSTRING "${LDG_SPIRV_CHUNK}" 6 2 LDG_SPIRV_B3)
                list(APPEND LDG_SPIRV_WORD_LIST "0x${LDG_SPIRV_B3}${LDG_SPIRV_B2}${LDG_SPIRV_B1}${LDG_SPIRV_B0}U")
            endforeach()
            string(REPLACE ";" ", " LDG_SPIRV_WORD_TEXT "${LDG_SPIRV_WORD_LIST}")
            string(APPEND LDG_SPIRV_EMBED_BODY "const uint32_t ldg_math_phys_spirv_${LDG_SPIRV_NAME}[] = { ${LDG_SPIRV_WORD_TEXT} };\n")
            string(APPEND LDG_SPIRV_EMBED_BODY "const uint64_t ldg_math_phys_spirv_${LDG_SPIRV_NAME}_size = ${LDG_SPIRV_BYTE_CUNT}ULL;\n\n")
        endforeach()
        file(WRITE ${CMAKE_BINARY_DIR}/ldg_math_phys_compute_spirv.c "${LDG_SPIRV_EMBED_BODY}")
        list(APPEND LDG_OPT_SOURCES
            src/none/none/math/phys/compute/vk.c
            ${CMAKE_BINARY_DIR}/ldg_math_phys_compute_spirv.c
        )
        message(STATUS "gpu: vulkan; vk backend built; spirv embedded")
    else()
        message(STATUS "gpu: off; vulkan not found; cpu backend only")
    endif()
else()
    message(STATUS "gpu: off; cpu backend only")
endif()

if(LDG_ARCH STREQUAL "amd64")
    set(LDG_MEM_CPY_SOURCE src/amd64/none/mem/cpy.c)
    set(LDG_MATH_SCALAR_SOURCE src/amd64/none/math/scalar.c)
else()
    set(LDG_MEM_CPY_SOURCE src/none/none/mem/cpy.c)
    set(LDG_MATH_SCALAR_SOURCE src/none/none/math/scalar.c)
endif()

set(LDG_SOURCES
        ${LDG_MEM_CPY_SOURCE}
        ${LDG_MATH_SCALAR_SOURCE}
        src/none/none/mem/frame.c
        src/none/none/compute/compute.c
        src/none/none/math/phys/compute/compute.c
        src/none/none/math/root.c
        src/none/none/math/phys/compute/cpu.c
        src/none/none/str/str.c
        src/none/none/str/fmt.c
        src/none/none/str/base64.c
        src/none/none/math/rand.c
        src/none/none/time/cal.c
        src/none/none/io/line.c
        src/none/none/sort/sort.c
        src/none/none/parse/parse.c
        src/none/none/compress/inflate.c
        src/none/none/compress/deflate.c
        src/none/none/proto/emiru.c
        src/none/none/proto/emiemi.c
        src/none/none/arch/MISC/MISC.c
        src/none/none/ecs/entity.c
        src/none/none/ecs/comp.c
        src/none/none/ecs/taxon.c
        src/none/none/ecs/cmd.c
        src/none/none/ecs/ecs.c
        src/none/none/ecs/query.c
        src/${LDG_ARCH}/none/ecs/sched.c
        ${LDG_OPT_SOURCES}
)
if(NOT LDG_ARCH STREQUAL "i386")
    list(APPEND LDG_SOURCES
        src/none/none/math/astar.c
        src/none/none/term/term.c
        src/none/none/term/vt.c
        src/none/none/math/sparse.c
        src/none/none/math/sparse_lu.c
        src/none/none/math/krylov.c
        src/none/none/math/laplace.c
        src/none/none/math/nurbs.c
        src/none/none/math/triangulate.c
        src/none/none/math/predicate.c
        src/none/none/math/tetrahedralize.c
        src/none/none/math/cdt.c
        src/none/none/math/phys.c
        src/none/none/math/phys/conserved.c
        src/none/none/math/phys/particle.c
        src/none/none/math/phys/field.c
        src/none/none/math/phys/fluid.c
        src/none/none/math/phys/thermo.c
        src/none/none/math/phys/electrostatic.c
        src/none/none/math/phys/magnetostatic.c
        src/none/none/math/phys/portmode.c
        src/none/none/math/phys/query.c
        src/none/none/math/phys/atmosphere.c
        src/none/none/math/phys/doppler.c
        src/none/none/math/phys/sig.c
        src/none/none/math/quaternion.c
        src/none/none/math/complex.c
        src/none/none/math/fft.c
        src/none/none/math/lu.c
        src/none/none/math/cholesky.c
        src/none/none/math/eigen.c
        src/none/none/math/stat/welford.c
        src/none/none/math/stat/descriptive.c
        src/none/none/math/stat/bayes.c
        src/none/none/math/stat/svm.c
        src/none/none/math/stat/rl.c
        src/none/none/math/stat/td.c
        src/none/none/math/stat/tile.c
        src/none/none/math/stat/vfa.c
        src/none/none/sys/progress.c
        src/none/none/time/timer.c
        src/none/none/sched/trace.c
        src/none/none/sched/event.c
        src/none/none/sched/job.c
        src/none/none/sched/sched.c
    )
endif()
if(LDG_PLATFORM STREQUAL "linux" OR LDG_PLATFORM STREQUAL "windows")
    list(APPEND LDG_SOURCES
        ${LDG_PLATFORM_DIR}/thread/sync.c
        ${LDG_PLATFORM_DIR}/thread/pool.c
        ${LDG_PLATFORM_DIR}/thread/spsc.c
        ${LDG_PLATFORM_DIR}/thread/mpmc.c
        ${LDG_PLATFORM_DIR}/thread/yield.c
        ${LDG_PLATFORM_DIR}/mem/alloc.c
        ${LDG_PLATFORM_DIR}/time/time.c
        ${LDG_PLATFORM_DIR}/time/perf.c
        ${LDG_PLATFORM_DIR}/io/console.c
        ${LDG_PLATFORM_DIR}/io/file.c
        ${LDG_PLATFORM_DIR}/io/dir.c
        ${LDG_PLATFORM_DIR}/io/path.c
        ${LDG_PLATFORM_DIR}/io/serial.c
        ${LDG_PLATFORM_DIR}/io/midi.c
        ${LDG_PLATFORM_DIR}/io/pty.c
        ${LDG_PLATFORM_DIR}/sys/uuid.c
        ${LDG_PLATFORM_DIR}/sys/tty.c
        ${LDG_PLATFORM_DIR}/sys/info.c
        ${LDG_PLATFORM_DIR}/sys/proc.c
        ${LDG_PLATFORM_DIR}/sys/window.c
        ${LDG_PLATFORM_DIR}/hw/sensor.c
        ${LDG_PLATFORM_DIR}/sys/clipboard.c
    )
    if(LDG_PLATFORM STREQUAL "windows")
        list(APPEND LDG_SOURCES ${LDG_PLATFORM_DIR}/sys/media.c)
    endif()
elseif(LDG_PLATFORM STREQUAL "DANGLING")
    list(APPEND LDG_SOURCES
        ${LDG_PLATFORM_DIR}/mem/alloc.c
        src/none/none/mem/slab.c
    )
    if(LDG_ARCH STREQUAL "i386")
        list(APPEND LDG_SOURCES ${LDG_PLATFORM_DIR}/thread/sync.c)
    elseif(LDG_ARCH STREQUAL "MISC")
        list(APPEND LDG_SOURCES
            ${LDG_PLATFORM_DIR}/arch/arith.c
            ${LDG_PLATFORM_DIR}/io/console.c
            ${LDG_PLATFORM_DIR}/time/time.c
            ${LDG_PLATFORM_DIR}/mem/rt.c
            ${LDG_PLATFORM_DIR}/sys/handover.c
            ${LDG_PLATFORM_DIR}/thread/yield.c
            ${LDG_PLATFORM_DIR}/io/file.c
            ${LDG_PLATFORM_DIR}/io/dir.c
            ${LDG_PLATFORM_DIR}/io/path.c
        )
    endif()
endif()

if(LDG_ARCH STREQUAL "amd64")
    set(LDG_ARCH_SOURCES
        ${LDG_PLATFORM_DIR}/arch/mem_secure.asm
        ${LDG_PLATFORM_DIR}/arch/time_tsc.asm
        ${LDG_PLATFORM_DIR}/arch/syscall.asm
        ${LDG_PLATFORM_DIR}/arch/cpuid.asm
        ${LDG_PLATFORM_DIR}/arch/arith.asm
    )
    set_source_files_properties(${LDG_ARCH_SOURCES} PROPERTIES LANGUAGE ASM_NASM)
    if(LDG_PLATFORM STREQUAL "windows")
        set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -f win64")
    else()
        set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -f elf64")
    endif()
    set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -I${CMAKE_CURRENT_SOURCE_DIR}/src/amd64/")
elseif(LDG_ARCH STREQUAL "i386")
    set(LDG_ARCH_SOURCES
        src/none/none/mem/secure.c
        ${LDG_PLATFORM_DIR}/arch/arith.asm
    )
    set_source_files_properties(${LDG_PLATFORM_DIR}/arch/arith.asm PROPERTIES LANGUAGE ASM_NASM)
    set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -f elf32")
else()
    set(LDG_ARCH_SOURCES src/none/none/mem/secure.c)
endif()

if(LDG_ARCH STREQUAL "amd64" OR LDG_ARCH STREQUAL "i386")
    set(LDG_ERR_INC "${CMAKE_CURRENT_BINARY_DIR}/include/dangling/core/err.inc")
    add_custom_command(
        OUTPUT ${LDG_ERR_INC}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/include/dangling/none/none/dangling/core/err.h
            -DOUTPUT=${LDG_ERR_INC}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/err_nasm_gen.cmake
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/dangling/none/none/dangling/core/err.h
        COMMENT "gen NASM err codes from err.h"
    )
    add_custom_target(err_nasm_gen DEPENDS ${LDG_ERR_INC})
    set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -I${CMAKE_CURRENT_BINARY_DIR}/include/")
endif()

set(LDG_FMT_CONF_INSTALL_PATH "${CMAKE_INSTALL_FULL_DATADIR}/dangling/uncrustify.cfg")

if(LDG_ARCH STREQUAL "i386" OR LDG_ARCH STREQUAL "MISC")
    set(LDG_TARGETS dangling_static)
else()
    set(LDG_TARGETS dangling_shared dangling_static)
endif()

foreach(LDG_TARGET ${LDG_TARGETS})
    if(LDG_TARGET STREQUAL "dangling_shared")
        add_library(${LDG_TARGET} SHARED ${LDG_SOURCES} ${LDG_ARCH_SOURCES})
    else()
        add_library(${LDG_TARGET} STATIC ${LDG_SOURCES} ${LDG_ARCH_SOURCES})
    endif()

    set_target_properties(${LDG_TARGET} PROPERTIES OUTPUT_NAME dangling)

    if(LDG_TARGET STREQUAL "dangling_shared" AND NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(LDG_VERSION_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/libdangling.map")
        set_target_properties(${LDG_TARGET} PROPERTIES
            LINK_DEPENDS ${LDG_VERSION_SCRIPT}
        )
        target_link_options(${LDG_TARGET} PRIVATE
            -Wl,--version-script=${LDG_VERSION_SCRIPT}
            -Wl,--as-needed
        )
    endif()

    target_include_directories(${LDG_TARGET} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/dangling/none/none>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/dangling/${LDG_ARCH}/none>
        $<INSTALL_INTERFACE:include>
    )
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/dangling/none/${LDG_PLATFORM})
        target_include_directories(${LDG_TARGET} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/dangling/none/${LDG_PLATFORM}>
        )
    endif()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/dangling/${LDG_ARCH}/${LDG_PLATFORM})
        target_include_directories(${LDG_TARGET} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/dangling/${LDG_ARCH}/${LDG_PLATFORM}>
        )
    endif()
    target_include_directories(${LDG_TARGET} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/none/none/math/phys>
    )
    if(LDG_OPT_INCLUDE_DIRS)
        target_include_directories(${LDG_TARGET} SYSTEM PUBLIC ${LDG_OPT_INCLUDE_DIRS})
    endif()
    target_link_libraries(${LDG_TARGET} PUBLIC ${LDG_OPT_LIBRARIES})
    if(LDG_PLATFORM STREQUAL "linux" OR LDG_PLATFORM STREQUAL "windows")
        target_link_libraries(${LDG_TARGET} PUBLIC Threads::Threads)
    endif()
    if(LDG_PLATFORM STREQUAL "linux")
        target_link_libraries(${LDG_TARGET} PUBLIC m)
    endif()
    if(LDG_PLATFORM STREQUAL "windows")
        target_link_libraries(${LDG_TARGET} PUBLIC kernel32 bcrypt ws2_32 advapi32 shell32 user32 setupapi winmm)
    endif()
    target_compile_definitions(${LDG_TARGET} PRIVATE ${LDG_OPT_DEFINES})

    if(LDG_WITH_FMT)
        target_compile_definitions(${LDG_TARGET} PRIVATE LDG_FMT_CONF_PATH="${LDG_FMT_CONF_INSTALL_PATH}")
    endif()

    if(TARGET err_nasm_gen)
        add_dependencies(${LDG_TARGET} err_nasm_gen)
    endif()
endforeach()

install(TARGETS ${LDG_TARGETS}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/dangling/none/none/dangling DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/dangling/${LDG_ARCH}/none/dangling)
    install(DIRECTORY include/dangling/${LDG_ARCH}/none/dangling DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/dangling/none/${LDG_PLATFORM}/dangling)
    install(DIRECTORY include/dangling/none/${LDG_PLATFORM}/dangling DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/dangling/${LDG_ARCH}/${LDG_PLATFORM}/dangling)
    install(DIRECTORY include/dangling/${LDG_ARCH}/${LDG_PLATFORM}/dangling DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

if(EXISTS ${LDG_ERR_INC})
    install(FILES ${LDG_ERR_INC} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dangling/core)
endif()

if(LDG_WITH_FMT)
    install(FILES share/dangling/uncrustify.cfg DESTINATION ${CMAKE_INSTALL_DATADIR}/dangling)
endif()

set(LDG_PC_REQUIRES "")
set(LDG_PC_LIBS "")
if(LDG_WITH_NET AND (CURL_FOUND OR CURL_LIBRARIES))
    if(PkgConfig_FOUND)
        string(APPEND LDG_PC_REQUIRES "libcurl")
    else()
        foreach(ldg_pc_lib IN LISTS CURL_LIBRARIES)
            string(APPEND LDG_PC_LIBS " ${ldg_pc_lib}")
        endforeach()
    endif()
endif()
if(LDG_WITH_GPU AND (VULKAN_FOUND OR VULKAN_LIBRARIES))
    if(PkgConfig_FOUND)
        if(LDG_PC_REQUIRES)
            string(APPEND LDG_PC_REQUIRES " ")
        endif()
        string(APPEND LDG_PC_REQUIRES "vulkan")
    else()
        foreach(ldg_pc_lib IN LISTS VULKAN_LIBRARIES)
            string(APPEND LDG_PC_LIBS " ${ldg_pc_lib}")
        endforeach()
    endif()
endif()

if(LDG_PLATFORM STREQUAL "windows")
    string(APPEND LDG_PC_LIBS " -lkernel32 -lbcrypt -lws2_32 -ladvapi32 -lshell32 -luser32 -lruntimeobject -lole32 -lsetupapi -lwinmm")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/dangling.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/dangling.pc"
    @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/dangling.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

set(LDG_ABI_DIR "${CMAKE_BINARY_DIR}/abi")
set(LDG_ABI_DUMP "${LDG_ABI_DIR}/dangling-${PROJECT_VERSION}.abi.tar.gz")

find_program(ABI_DUMPER abi-dumper)
find_program(ABI_CHECKER abi-compliance-checker)

if(ABI_DUMPER AND TARGET dangling_shared)
    add_custom_target(abi-dump
        COMMAND ${CMAKE_COMMAND} -E make_directory ${LDG_ABI_DIR}
        COMMAND ${ABI_DUMPER} $<TARGET_FILE:dangling_shared>
            -o ${LDG_ABI_DUMP}
            -lver ${PROJECT_VERSION}
            -public-headers ${CMAKE_CURRENT_SOURCE_DIR}/include
        DEPENDS dangling_shared
        COMMENT "abi-dumper: dangling ${PROJECT_VERSION}"
    )
endif()

if(ABI_DUMPER AND ABI_CHECKER AND TARGET dangling_shared)
    add_custom_target(abi-check
        COMMAND ${ABI_CHECKER}
            -l dangling
            -old ${CMAKE_CURRENT_SOURCE_DIR}/abi/dangling-baseline.abi.tar.gz
            -new ${LDG_ABI_DUMP}
        DEPENDS abi-dump
        COMMENT "abi-compliance-checker: baseline vs ${PROJECT_VERSION}"
    )
endif()

if(COMMAND std_invalidate_test_state)
    if(TARGET dangling_shared)
        std_invalidate_test_state(dangling_shared)
    else()
        std_invalidate_test_state(dangling_static)
    endif()
endif()

if(COMMAND qemu_add_run_target)
    qemu_add_run_target()
endif()
