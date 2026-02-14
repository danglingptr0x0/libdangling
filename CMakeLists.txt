cmake_minimum_required(VERSION 3.16)
project(dangling VERSION 1.0.0 LANGUAGES C ASM_NASM)

list(APPEND CMAKE_MODULE_PATH "$ENV{HOME}/.config/cmake")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(STD_NO_INSTR ON CACHE BOOL "" FORCE)
endif()
include(std)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(_WIN32_WINNT=0x0600)
endif()

include(GNUInstallDirs)

option(LDG_WITH_AUDIO "Build audio subsystem (auto-detects pw or ALSA)" ON)
option(LDG_WITH_NET "Build net subsystem (requires libcurl)" ON)
option(LDG_WITH_FMT "Build fmt config subsys" ON)

if(NOT CMAKE_SYSTEM_PROCESSOR)
    if(CMAKE_C_COMPILER MATCHES "x86_64")
        set(CMAKE_SYSTEM_PROCESSOR "x86_64")
    endif()
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(LDG_ARCH "amd64")
else()
    message(FATAL_ERROR "unsupported arch: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LDG_PLATFORM "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(LDG_PLATFORM "windows")
else()
    message(FATAL_ERROR "unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

set(LDG_PLATFORM_DIR "src/${LDG_ARCH}/${LDG_PLATFORM}")

find_package(Threads REQUIRED)

if(LDG_PLATFORM STREQUAL "linux")
    find_package(PkgConfig REQUIRED)
endif()

set(LDG_OPT_SOURCES "")
set(LDG_OPT_INCLUDE_DIRS "")
set(LDG_OPT_LIBRARIES "")
set(LDG_OPT_DEFINES "")

if(LDG_WITH_NET)
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
            if(LDG_PLATFORM STREQUAL "windows")
                find_library(SSL_LIBRARY NAMES ssl)
                find_library(CRYPTO_LIBRARY NAMES crypto)
                list(APPEND LDG_OPT_LIBRARIES ${SSL_LIBRARY} ${CRYPTO_LIBRARY} ws2_32 crypt32 bcrypt advapi32 gdi32 iphlpapi)
            endif()
        endif()
        message(STATUS "net: libcurl")
    else()
        message(STATUS "net: off; libcurl not found")
    endif()
else()
    message(STATUS "net: off")
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

set(LDG_SOURCES
    src/none/none/str/str.c
    src/none/none/parse/parse.c
    src/none/none/proto/emiru.c
    src/none/none/proto/emiemi.c
    src/none/none/misc/misc.c
    ${LDG_PLATFORM_DIR}/thread/sync.c
    ${LDG_PLATFORM_DIR}/thread/pool.c
    ${LDG_PLATFORM_DIR}/thread/spsc.c
    ${LDG_PLATFORM_DIR}/thread/mpmc.c
    ${LDG_PLATFORM_DIR}/mem/alloc.c
    ${LDG_PLATFORM_DIR}/time/time.c
    ${LDG_PLATFORM_DIR}/time/perf.c
    ${LDG_PLATFORM_DIR}/io/file.c
    ${LDG_PLATFORM_DIR}/io/dir.c
    ${LDG_PLATFORM_DIR}/io/path.c
    ${LDG_PLATFORM_DIR}/sys/uuid.c
    ${LDG_PLATFORM_DIR}/sys/tty.c
    ${LDG_PLATFORM_DIR}/sys/info.c
    ${LDG_OPT_SOURCES}
)

if(LDG_ARCH STREQUAL "amd64")
    set(LDG_ARCH_SOURCES
        ${LDG_PLATFORM_DIR}/arch/mem_secure.asm
        ${LDG_PLATFORM_DIR}/arch/time_tsc.asm
        ${LDG_PLATFORM_DIR}/arch/syscall.asm
        ${LDG_PLATFORM_DIR}/arch/cpuid.asm
    )
    set_source_files_properties(${LDG_ARCH_SOURCES} PROPERTIES LANGUAGE ASM_NASM)
    if(LDG_PLATFORM STREQUAL "windows")
        set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -f win64")
    else()
        set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -f elf64")
    endif()

    set(LDG_ERR_INC "${CMAKE_CURRENT_BINARY_DIR}/include/dangling/core/err.inc")
    add_custom_command(
        OUTPUT ${LDG_ERR_INC}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/include/none/none/dangling/core/err.h
            -DOUTPUT=${LDG_ERR_INC}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/err_nasm_gen.cmake
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/none/none/dangling/core/err.h
        COMMENT "gen NASM err codes from err.h"
    )
    add_custom_target(err_nasm_gen DEPENDS ${LDG_ERR_INC})
    set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -I${CMAKE_CURRENT_BINARY_DIR}/include/")
else()
    set(LDG_ARCH_SOURCES src/none/none/mem/secure.c)
endif()

set(LDG_FMT_CFG_INSTALL_PATH "${CMAKE_INSTALL_FULL_DATADIR}/dangling/uncrustify.cfg")

foreach(LDG_TARGET dangling_shared dangling_static)
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
        )
    endif()

    target_include_directories(${LDG_TARGET} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/none/none>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/${LDG_ARCH}/none>
        $<INSTALL_INTERFACE:include/none/none>
        $<INSTALL_INTERFACE:include/${LDG_ARCH}/none>
    )
    if(LDG_OPT_INCLUDE_DIRS)
        target_include_directories(${LDG_TARGET} SYSTEM PUBLIC ${LDG_OPT_INCLUDE_DIRS})
    endif()
    target_link_libraries(${LDG_TARGET} PUBLIC ${LDG_OPT_LIBRARIES} Threads::Threads)
    if(LDG_PLATFORM STREQUAL "windows")
        target_link_libraries(${LDG_TARGET} PUBLIC kernel32 bcrypt ws2_32)
    endif()
    target_compile_definitions(${LDG_TARGET} PRIVATE ${LDG_OPT_DEFINES})

    if(LDG_WITH_FMT)
        target_compile_definitions(${LDG_TARGET} PRIVATE LDG_FMT_CFG_PATH="${LDG_FMT_CFG_INSTALL_PATH}")
    endif()

    if(TARGET err_nasm_gen)
        add_dependencies(${LDG_TARGET} err_nasm_gen)
    endif()
endforeach()

install(TARGETS dangling_shared dangling_static
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/none/none/dangling DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/none/none)
install(DIRECTORY include/${LDG_ARCH}/none/dangling DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LDG_ARCH}/none)

if(EXISTS ${LDG_ERR_INC})
    install(FILES ${LDG_ERR_INC} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dangling/core)
endif()

if(LDG_WITH_FMT)
    install(FILES share/dangling/uncrustify.cfg DESTINATION ${CMAKE_INSTALL_DATADIR}/dangling)
endif()

set(LDG_PC_REQUIRES "")
if(LDG_WITH_NET)
    set(LDG_PC_REQUIRES "libcurl")
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

if(ABI_DUMPER)
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

if(ABI_DUMPER AND ABI_CHECKER)
    add_custom_target(abi-check
        COMMAND ${ABI_CHECKER}
            -l dangling
            -old ${CMAKE_CURRENT_SOURCE_DIR}/abi/dangling-baseline.abi.tar.gz
            -new ${LDG_ABI_DUMP}
        DEPENDS abi-dump
        COMMENT "abi-compliance-checker: baseline vs ${PROJECT_VERSION}"
    )
endif()

std_invalidate_test_state(dangling_shared)
qemu_add_run_target()
