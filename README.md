ESG BSP Test Suite
==================

## Overview
#### Principle and Usage

This application provides low-level stubbing for the BSP interfaces, to assess robustness.
The idea is that you can enable
* only one subsystem, like audio, spi transfert, uart parsing etc... and insure the elementary bricks are robust.
* select any combo of interface, and check is when combined, the system is robust.
* each interface is implemented using a init/loop scheme, with a so-called 'runner' that is executed as a pthread. The main process just waits (joins) all active runners.

Some interfaces, like uart, can be easily debugged PC-side, using the tty0tty uart emulator, and running with gdb. In this case, make sure to enable debug flags in the CMakeLists, for instance:
```
-g -O0 -fno-omit-frame-pointer
```

For performances related test on the target however, make sure to set -O2 (default for the yocto toolchain) and remove the no-omit-fp flag.

!!! See possible options with `--help`, you need to activate at least one runner.

#### Tracing
Most of the debugging on the target (when built with recipe _esg-bsp-test_ in yocto) is best done using DLT traces, the trace projerct for this app is located [here](https://github.com/VogoVokkero/dlt-client/blob/develop/resources/A8375/btst.dlp).
Traces are crucial for real-time uses, e.g. for the audio, the default scenario expects you to feed continuous audio on ESG-IN1, and traces will report lack of levels on this input.

To enable trace on PC/Native side, you may have to hack the owner or _/tmp/dlt_ with this a chown command, just add this pipe to your group.  

## SUBSYSTEM : Audio Loop

The Audio runner is based on a sample app from http://equalarea.com/paul/alsa-audio.html

#### ALSA transfert methods and ALSA example

ESG _mco-audio-app_ is implementin the 'poll/select' scheme, which is not the simplest, but back then did best match the 'multicore-tools/task-manager' scheme.
In order to go back to the basics and test the vanilla ALSA example, the _alsa-poll-example_ app can be built.
This alsa example shall be kept unchanged, as a reference, it features all implemented schemes and must be launched with proper options to specifically test for poll/select.

In our case, we will be looking into:
- handling clean Start/Pause/Resume/Stop
- handling clean X-run/Recover scenarios (which is more or less the same problame than above).

!!! use option `--audio` to activate the audio loop.

The following commande will loopback audio durig 10s:
```
time  /mnt/diag/esg-bsp-test --audio -l 1000
```

#### reference alsa application

a ref app from the ALSA projet is also built as 'alsa-poll-example'

https://github.com/TomDataworks/whisper_client/blob/master/celt-0.7.0-src/tools/alsa_device.c


## SUBSYSTEM : Elite : SPI/TDMA Protocol Stub

Using Elite/SPI protocol, provide an audio frames streaming stub: 
* the VO8346 must be in audio mode
* linux is spi-master, this application must listen to the "slave-ready" GPIO, and initiate an spi transfer using spidev when the GPIO is asserted.

The slave-ready GPIO is gpiochip1, gpio15.
The best is to use the libgpiod library, much more reliable than the older gpiolib interface.

manual dump of the slave-reay config:
```
/sys/class/gpio/gpio47# cat active_low consumers direction edge value
```

!!! use option `--tdma` to activate the audio loop.

## SUBSYSTEM : Elite : UART Protocol

#### test and debug on PC

this app will open ttymxc* when built for arm, or tnt0 when build for native PC.
/dev/tnt0 is created by the tty0tty module, it's usage is documented here: https://github.com/freemed/tty0tty

the idea is that:
* you can inject a message in __/dev/tnt1__ via a script
* it will be received by the test app on __/dev/tnt0__ and parsed.
* dlt traces will let you know whatg the test app does of it

!!! use option `--uart` to activate the audio loop.

## SUBSYSTEM : Auvitran Interface

This test module allows to exercise the spidev device, used to configure de Auvitran modules.
At this moment, no stress option is available (TODO), is is used mainly to try for a clean mute/demute when we reset the audio devices (ESG-190 workaround)

#### getting, building and running tty0tty

```
   git clone git@github.com:freemed/tty0tty.git
   cd tty0tty/module/
   make
   sudo make modules_install
   sudo insmod /lib/modules/5.15.0-58-generic/extra/tty0tty.ko
```

Note that you must be part of the _dialout_ group, to use the devices.

```
crw-rw---- 1 root dialout 236, 0 févr.  6 11:35 /dev/tnt0
crw-rw---- 1 root dialout 236, 1 févr.  6 11:35 /dev/tnt1
```

#### Elite uart protocol parser

At this moment, the parser just deals with SOF, RAW-size fields, the reset being just payload.
As an example, emulatinf an STM32 message: 


| Source   |  Dest  | Type  | Emulated message  | DLT trace from parser (may change)  |
|---|---|---|---|---|
| STM32   |  BSP-test-app  | ASCII  | `$>echo "DSP<TST::VERSION" > /dev/tnt1` | BTST UDSP udsp_runner got frame DSP TST DSP<TST VERSION |
| STM32  | BSP-test-app  | RAW  | `$>echo "DSP<TST:8:ABCD1234" > /dev/tnt1`  | BTST UDSP udsp_runner got RAW frame 0x41 0x42 0x43 0x44 0x31 0x32 0x33 0x34  |

## Improving and Building

* new arguments must be added using the gengetopt pattern, see 'options' folfer
* build in yocto is done using 'bitbake -c cleansstate esg-bsp-test-audio; bitbake esg-bsp-test-audio', the recipe is in meta-vogo-mco/recipes-bsp
* on native PC, make sure to install 'sudo apt install gpiod libgpiod-dev libasound2-dev dlt-daemon libdlt-dev', the recipe will need to dep too of course.


## Troubleshooting

* App fails to initialize the audio devices : maybe mco-audio-app is still running, try stopping it with "systemctl stop mco-audio-app"
* trace show audio read and write OK, but only zeros are recorded (and hence played) : maybe channels are muted, or not matrix'ed (auvitran routing), try relaunching the product with audio app and backend enabled.

