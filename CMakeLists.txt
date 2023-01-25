cmake_minimum_required(VERSION 3.9)

project(esg-bsp-test-audio VERSION 1.0.1 DESCRIPTION "ESG test-stub for audio and SPI")

# build with DLT, but by default, traces will be muted, so we can decouple MII stress from audio + SPI.
add_definitions(-DENABLE_DLT=${ENABLE_DLT})

find_package(PkgConfig REQUIRED)
pkg_check_modules(CDLT REQUIRED dlt-client)
pkg_check_modules(ALSA REQUIRED alsa)

include(GNUInstallDirs)

add_executable(esg-bsp-test-audio
    main.c
    options/cmdline.c
    audio/audio-runner.c
    elite/elite-tdma-runner.c
    elite/elite-uart-dsp-runner.c
    elite/elite-spidev.c
    elite/elite-slave-ready-gpio.c
    )

add_definitions(-flto -g -O2 -fstack-protector-strong)

target_link_libraries(esg-bsp-test-audio -pthread ${CDLT_LIBRARIES} ${ALSA_LIBRARIES})
target_include_directories(esg-bsp-test-audio PUBLIC ./inc ${CDLT_INCLUDE_DIRS} ${ALSA_INCLUDE_DIRS})
target_compile_options(esg-bsp-test-audio PUBLIC ${CDLT_CFLAGS_OTHER} ${ALSA_CFLAGS_OTHER})

install(TARGETS esg-bsp-test-audio DESTINATION bin)
