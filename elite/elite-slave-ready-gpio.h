/*
 ============================================================================
 Name        : gpio.h
 Author      : tmu
 Version     :
 Copyright   : Closed
 Description : gpio use tools
 ============================================================================
 */
#ifndef ELITE_SLAVE_READY_GPIO
#define ELITE_SLAVE_READY_GPIO

#include <stdint.h>
#include "errno.h"
#include <gpiod.h>

#define SPIDEV  "/dev/spidev3.0"
#define GPIO_PORT_SPI_STM_READY 2
#define GPIO_ID_SPI_STM_READY   15

#define EVENT_BUF_SIZE 32

typedef struct{
    uint16_t id;
    int fd;
} elite_gpio_t;


int elite_gpio_close(elite_gpio_t *sready_gpio);
int8_t elite_gpio_get(elite_gpio_t *sready_gpio);
int elite_slave_ready_gpio(elite_gpio_t *sready_gpio);
int elite_slave_ready_wait(elite_gpio_t *sready_gpio);

#endif //ELITE_SLAVE_READY_GPIO