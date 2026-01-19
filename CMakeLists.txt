cmake_minimum_required(VERSION 3.16)
project(dangling VERSION 0.1.0 LANGUAGES C ASM_NASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(YDER REQUIRED libyder)
pkg_check_modules(CURL REQUIRED libcurl)

set(LDG_SOURCES
    src/mem/alloc.c
    src/str/str.c
    src/time/time.c
    src/time/perf.c
    src/net/curl.c
    src/fmt/fmt.c
    src/thread/spsc.c
    src/thread/sync.c
    src/thread/pool.c
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(LDG_ARCH_SOURCES
        src/arch/x86_64/mem_secure.asm
        src/arch/x86_64/time_tsc.asm
        src/arch/x86_64/syscall.asm
    )
    set_source_files_properties(${LDG_ARCH_SOURCES} PROPERTIES LANGUAGE ASM_NASM)
    set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -f elf64")
else()
    set(LDG_ARCH_SOURCES src/mem/secure.c)
endif()

set(LDG_FMT_CFG_INSTALL_PATH "${CMAKE_INSTALL_FULL_DATADIR}/dangling/uncrustify.cfg")

add_library(dangling_shared SHARED ${LDG_SOURCES} ${LDG_ARCH_SOURCES})
set_target_properties(dangling_shared PROPERTIES OUTPUT_NAME dangling)
target_include_directories(dangling_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${YDER_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)
target_link_libraries(dangling_shared PUBLIC ${YDER_LIBRARIES} ${CURL_LIBRARIES} Threads::Threads)
target_compile_definitions(dangling_shared PRIVATE LDG_FMT_CFG_PATH="${LDG_FMT_CFG_INSTALL_PATH}")

add_library(dangling_static STATIC ${LDG_SOURCES} ${LDG_ARCH_SOURCES})
set_target_properties(dangling_static PROPERTIES OUTPUT_NAME dangling)
target_include_directories(dangling_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${YDER_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)
target_link_libraries(dangling_static PUBLIC ${YDER_LIBRARIES} ${CURL_LIBRARIES} Threads::Threads)
target_compile_definitions(dangling_static PRIVATE LDG_FMT_CFG_PATH="${LDG_FMT_CFG_INSTALL_PATH}")

install(TARGETS dangling_shared dangling_static
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/dangling DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES share/dangling/uncrustify.cfg DESTINATION ${CMAKE_INSTALL_DATADIR}/dangling)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/dangling.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/dangling.pc"
    @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/dangling.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
