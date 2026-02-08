cmake_minimum_required(VERSION 3.16)
project(dangling VERSION 0.1.0 LANGUAGES C ASM_NASM)

list(APPEND CMAKE_MODULE_PATH "$ENV{HOME}/.config/cmake")
include(std)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)

option(LDG_WITH_AUDIO "Build audio subsystem (auto-detects PipeWire or ALSA)" ON)
option(LDG_WITH_NET "Build network subsystem (requires libcurl)" ON)
option(LDG_WITH_FMT "Build format config subsystem" ON)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(YDER REQUIRED libyder)

set(LDG_OPT_SOURCES "")
set(LDG_OPT_INCLUDE_DIRS "")
set(LDG_OPT_LIBRARIES "")
set(LDG_OPT_DEFINES "")

if(LDG_WITH_NET)
    pkg_check_modules(CURL REQUIRED libcurl)
    list(APPEND LDG_OPT_SOURCES src/net/curl.c)
    list(APPEND LDG_OPT_INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
    list(APPEND LDG_OPT_LIBRARIES ${CURL_LIBRARIES})
    list(APPEND LDG_OPT_DEFINES LDG_NET)
    message(STATUS "net: libcurl")
else()
    message(STATUS "net: off")
endif()

if(LDG_WITH_FMT)
    list(APPEND LDG_OPT_SOURCES src/fmt/fmt.c)
    list(APPEND LDG_OPT_DEFINES LDG_FMT)
    message(STATUS "fmt: on")
else()
    message(STATUS "fmt: off")
endif()

if(LDG_WITH_AUDIO)
    pkg_check_modules(PIPEWIRE libpipewire-0.3)
    pkg_check_modules(ALSA alsa)

    if(PIPEWIRE_FOUND)
        set(LDG_AUDIO_BACKEND "pipewire")
        list(APPEND LDG_OPT_SOURCES src/audio/audio_pw.c)
        list(APPEND LDG_OPT_INCLUDE_DIRS ${PIPEWIRE_INCLUDE_DIRS})
        list(APPEND LDG_OPT_LIBRARIES ${PIPEWIRE_LIBRARIES})
        list(APPEND LDG_OPT_DEFINES LDG_AUDIO_PIPEWIRE)
        message(STATUS "audio: pipewire")
    elseif(ALSA_FOUND)
        set(LDG_AUDIO_BACKEND "alsa")
        list(APPEND LDG_OPT_SOURCES src/audio/audio_alsa.c)
        list(APPEND LDG_OPT_INCLUDE_DIRS ${ALSA_INCLUDE_DIRS})
        list(APPEND LDG_OPT_LIBRARIES ${ALSA_LIBRARIES})
        list(APPEND LDG_OPT_DEFINES LDG_AUDIO_ALSA)
        message(STATUS "audio: alsa")
    else()
        set(LDG_AUDIO_BACKEND "none")
        message(STATUS "audio: none; libpipewire-0.3 or alsa not found")
    endif()
else()
    set(LDG_AUDIO_BACKEND "none")
    message(STATUS "audio: off")
endif()

set(LDG_SOURCES
    src/mem/alloc.c
    src/str/str.c
    src/time/time.c
    src/time/perf.c
    src/thread/spsc.c
    src/thread/sync.c
    src/thread/pool.c
    src/thread/mpmc.c
    src/parse/parse.c
    src/proto/emiru.c
    src/proto/emiemi.c
    ${LDG_OPT_SOURCES}
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(LDG_ARCH_SOURCES
        src/arch/x86_64/mem_secure.asm
        src/arch/x86_64/time_tsc.asm
        src/arch/x86_64/syscall.asm
        src/arch/x86_64/cpuid.asm
    )
    set_source_files_properties(${LDG_ARCH_SOURCES} PROPERTIES LANGUAGE ASM_NASM)
    set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -f elf64")
else()
    set(LDG_ARCH_SOURCES src/mem/secure.c)
endif()

set(LDG_FMT_CFG_INSTALL_PATH "${CMAKE_INSTALL_FULL_DATADIR}/dangling/uncrustify.cfg")

foreach(LDG_TARGET dangling_shared dangling_static)
    if(LDG_TARGET STREQUAL "dangling_shared")
        add_library(${LDG_TARGET} SHARED ${LDG_SOURCES} ${LDG_ARCH_SOURCES})
    else()
        add_library(${LDG_TARGET} STATIC ${LDG_SOURCES} ${LDG_ARCH_SOURCES})
    endif()

    set_target_properties(${LDG_TARGET} PROPERTIES OUTPUT_NAME dangling)

    target_include_directories(${LDG_TARGET} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_include_directories(${LDG_TARGET} SYSTEM PUBLIC
        ${YDER_INCLUDE_DIRS}
        ${LDG_OPT_INCLUDE_DIRS}
    )
    target_link_libraries(${LDG_TARGET} PUBLIC ${YDER_LIBRARIES} ${LDG_OPT_LIBRARIES} Threads::Threads)
    target_compile_definitions(${LDG_TARGET} PRIVATE ${LDG_OPT_DEFINES})

    if(LDG_WITH_FMT)
        target_compile_definitions(${LDG_TARGET} PRIVATE LDG_FMT_CFG_PATH="${LDG_FMT_CFG_INSTALL_PATH}")
    endif()
endforeach()

install(TARGETS dangling_shared dangling_static
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/dangling DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(LDG_WITH_FMT)
    install(FILES share/dangling/uncrustify.cfg DESTINATION ${CMAKE_INSTALL_DATADIR}/dangling)
endif()

set(LDG_PC_REQUIRES "libyder")
if(LDG_WITH_NET)
    string(APPEND LDG_PC_REQUIRES " libcurl")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/dangling.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/dangling.pc"
    @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/dangling.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

std_invalidate_test_state(dangling_shared)
qemu_add_run_target()
