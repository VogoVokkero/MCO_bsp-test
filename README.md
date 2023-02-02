ESG BSP Test Suite : Audio and SPI
==================================

This application provides low-level stubbing for the BSP interfaces, to assess robustness.

## Audio Loop

Based on a sample app from http://equalarea.com/paul/alsa-audio.html

## ALSA transfert methods and ALSA example

ESG "mco-audio-app" is implementin the 'poll/select' scheme, which is not the simplest, but back then did best match the 'multicore-tools/task-manager' scheme.
In order to go back to the basics and test the vanilla ALSA example, the 'alsa-pôll-example' app can be built.
This alsa example shall be kept unchanged, as a reference, it features all implemented schemes and must be launched with proper options to specifically test for poll/select.

In our case, we will be looking into:
- handling clean Start/Pause/Resume/Stop
- handling clean X(-run/Recover scenarios (which is more or less the same problame than above).

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

## Improving and Building

* new arguments must be added using the gengetopt pattern, see 'options' folfer
* build in yocto is done using 'bitbake -c cleansstate esg-bsp-test-audio; bitbake esg-bsp-test-audio', the recipe is in meta-vogo-mco/recipes-bsp
* on native PC, make sure to install 'sudo apt install gpiod libgpiod-dev', the recipe will need to dep too of course.
