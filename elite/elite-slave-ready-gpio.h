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

#define SPIDEV  "/dev/spidev3.0"
#define GPIO_PORT_SPI_STM_READY 2
#define GPIO_ID_SPI_STM_READY   15

typedef struct{
    uint16_t id;
    int fd;
    //bool last_value;
} elite_gpio_t;

typedef enum{
    gpio_in,
    gpio_out
}elite_gpio_dir_t;

typedef enum{
    rising,
    falling,
    both,
    none
}elite_gpio_event_t;

int elite_gpio_open(uint8_t gpio_port, uint8_t gpio_index, elite_gpio_dir_t dir, elite_gpio_t *gpio);
int elite_gpio_close(elite_gpio_t *gpio);
int8_t elite_gpio_get(elite_gpio_t *gpio);
int elite_gpio_set(elite_gpio_t *gpio, int8_t value);
int elite_gpio_configure_event(elite_gpio_t *gpio, elite_gpio_event_t event);

int elite_slave_ready_gpio(void);

#endif //ELITE_SLAVE_READY_GPIO