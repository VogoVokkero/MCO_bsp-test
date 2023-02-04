ESG BSP Test Suite : Audio and SPI
==================================

This application provides low-level stubbing for the BSP interfaces, to assess robustness.

## Audio Loop

Based on a sample app from http://equalarea.com/paul/alsa-audio.html

### ALSA transfert methods and ALSA example

ESG "mco-audio-app" is implementin the 'poll/select' scheme, which is not the simplest, but back then did best match the 'multicore-tools/task-manager' scheme.
In order to go back to the basics and test the vanilla ALSA example, the 'alsa-pôll-example' app can be built.
This alsa example shall be kept unchanged, as a reference, it features all implemented schemes and must be launched with proper options to specifically test for poll/select.

In our case, we will be looking into:
- handling clean Start/Pause/Resume/Stop
- handling clean X(-run/Recover scenarios (which is more or less the same problame than above).

**use option '--audio' to activate the audio loop.**

### reference alsa application

a ref app from the ALSA projet is also built as 'alsa-poll-example'

## Elite : SPI/TDMA Protocol Stub

Using Elite/SPI protocol, provide an audio frames streaming stub: 
* the VO8346 must be in audio mode
* linux is spi-master, this application must listen to the "slave-ready" GPIO, and initiate an spi transfer using spidev when the GPIO is asserted.

The slave-ready GPIO is gpiochip1, gpio15.
The best is to use the libgpiod library, much more reliable than the older gpiolib interface.

manual dump of the slave-reay config:
```
/sys/class/gpio/gpio47# cat active_low consumers direction edge value
```

**use option '--tdma' to activate the spi/tdma runner.**

## Elite UART Protocol

this app will open ttymxc* when built for arm, or tnt0 when build for native PC.
/dev/tnt0 is created by the tty0tty module, it's usage is documented here: https://github.com/freemed/tty0tty

the idea is tha you can inject a message in /dev/tnt1 via a script, and it will be received on /dev/tnt0 by the test app, and parsed.

At this moment, the parser just deals with SOF, RAW-size fields, the reset being just payload.

**use option '--uart' to activate the uart runner.**

## Improving and Building

* new arguments must be added using the gengetopt pattern, see 'options' folfer
* build in yocto is done using 'bitbake -c cleansstate esg-bsp-test-audio; bitbake esg-bsp-test-audio', the recipe is in meta-vogo-mco/recipes-bsp
* on native PC, make sure to install 'sudo apt install gpiod libgpiod-dev libasound2-dev dlt-daemon libdlt-dev', the recipe will need to dep too of course.


## Troubleshooting

* App fails to initialize the audio devices : maybe mco-audio-app is still running, try stopping it with "systemctl stop mco-audio-app"
* trace show audio read and write OK, but only zeros are recorded (and hence played) : maybe channels are muted, or not matrix'ed (auvitran routing), try relaunching the product with audio app and backend enabled.

