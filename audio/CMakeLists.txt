cmake_minimum_required(VERSION 3.9)

project(alsa-poll-example VERSION 1.0.0 DESCRIPTION "ALSA simple test app, matching ESG scheme")

# build with DLT, but by default, traces will be muted, so we can decouple MII stress from audio + SPI.
add_definitions(-DENABLE_DLT=${ENABLE_DLT})

find_package(PkgConfig REQUIRED)
pkg_check_modules(CDLT REQUIRED dlt-client)
pkg_check_modules(ALSA REQUIRED alsa)

include(GNUInstallDirs)

add_executable(alsa-poll-example   alsa-poll-example.c
    )

add_definitions(-flto -g -O2 -fstack-protector-strong)

target_link_libraries(alsa-poll-example -pthread -lm
    ${CDLT_LIBRARIES}
    ${GPIOD_LIBRARIES}
    ${ALSA_LIBRARIES})

target_include_directories(alsa-poll-example PUBLIC ../inc . 
    ${CDLT_INCLUDE_DIRS}
    ${GPIOD_INCLUDE_DIRS}
    ${ALSA_INCLUDE_DIRS})

target_compile_options(alsa-poll-example PUBLIC
    ${CDLT_CFLAGS_OTHER}
    ${GPIOD_CFLAGS_OTHER}
    ${ALSA_CFLAGS_OTHER})

install(TARGETS alsa-poll-example DESTINATION bin)
