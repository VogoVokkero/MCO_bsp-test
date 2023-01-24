/*
 ============================================================================
 Name        : gpio.c
 Author      : tmu
 Version     :
 Copyright   : Closed
 Description : gpio use tools
 ============================================================================
 */
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>         //4 close/open
#include <string.h> //4 strlen & memcpy
#include <sys/time.h>  //4 time

#include "elite-slave-ready-gpio.h"
#include "dlt-client.h"

DLT_DECLARE_CONTEXT(dlt_ctxt_gpioe);

#define IMX_GPIO_NR(port, index)    ((((port)-1)*32)+((index)&31))
#define MAX_BUF 150         // max path is in linux/limits.h but 4096 seems way to much 4 me...

elite_gpio_t slave_ready_gpio = {0};

int elite_gpio_open(uint8_t gpio_port, uint8_t gpio_index, elite_gpio_dir_t dir, elite_gpio_t *gpio)
{
    int fd;
    char buf[MAX_BUF]; 
    int open_flag = O_RDONLY;
    gpio->id = IMX_GPIO_NR(gpio_port, gpio_index); 

    // Initialize GPIO
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if(fd <= 0)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not open : /sys/class/gpio/export"));
        return -1;
    }
    sprintf(buf, "%d", gpio->id);
    if(write(fd, buf, strlen(buf)) < strlen(buf))
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could write in: /sys/class/gpio/export"));
        close(fd);
        return -1;
    }
    close(fd);

    //Set Direction
    sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio->id);
    uint8_t timeout = 100;
    while(timeout)
    {
        fd = open(buf, O_WRONLY);
        if(fd <= 0)
        {
            usleep(10000);
            timeout--;
            if(!timeout)
                break;
        }
        else
        {
            break;
        }
    }
    if(fd <= 0)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not open : /sys/class/gpio/gpio/direction"));
        return -1;
    }
    if(dir == gpio_in) {
        /* Set in direction */
        if(write(fd, "in", 2) != 2)
        {
            DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not write : /sys/class/gpio/gpioX/direction"));
            return -1;
        } 
        open_flag = O_RDONLY;
    } else{
        /* Set out direction */
        if(write(fd, "out", 3) != 3)
        {
            DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not write : /sys/class/gpio/gpioX/direction"));
            return -1;
        } 
        open_flag = O_WRONLY;
    }
    close(fd);

    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio->id);
    gpio->fd = open(buf, open_flag);
    if(gpio->fd <= 0)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not open : /sys/class/gpio/gpioX/value"));
        return -1;
    }

    return gpio->fd;
}

int elite_gpio_close(elite_gpio_t *gpio)
{
    if(gpio->fd)
    {
        int fd;
        char buf[MAX_BUF]; 
        close(gpio->fd);
        fd = open("/sys/class/gpio/unexport", O_WRONLY);
        if(fd <= 0)
        {
            DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not open : /sys/class/gpio/unexport"));
            return -1;
        }
        sprintf(buf, "%d", gpio->id);
        if(write(fd, buf, strlen(buf)) != strlen(buf))
        {
            DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not write : /sys/class/gpio/unexport"));
            close(fd);
            return -1;
        }
        close(fd);
        gpio->fd = 0;
    }
    return 0;
}


int8_t elite_gpio_get(elite_gpio_t *gpio)
{
    char value;
    if(read(gpio->fd, &value, 1) < 1)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not read gpio"));
        return -1;
    }

    lseek(gpio->fd, 0, SEEK_SET);     //set the file ptr back at the start of the file

    return (int8_t)value - '0';
}

// 1: high
// 0: low
int elite_gpio_set(elite_gpio_t *gpio, int8_t value)
{
    char buff[] = {"0"};
    buff[0] += value;
    if(write(gpio->fd, buff, 1) < 1)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not write gpio"));
        return -1;
    }
    lseek(gpio->fd, 0, SEEK_SET);     //set the file ptr back at the start of the file
    return 0;
} 

int elite_gpio_configure_event(elite_gpio_t *gpio, elite_gpio_event_t event)
{
    int fd;
    char buf[MAX_BUF]; 

    // Configure GPIO event
    sprintf(buf, "/sys/class/gpio/gpio%d/edge", gpio->id);
    fd = open(buf, O_WRONLY);
    if(fd <= 0)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not open /sys/class/gpio/gpioX/edge"));
        return -1;
    }
    switch (event) {
        case rising:
            strcpy(buf, "rising");
            break;
        case falling:
            strcpy(buf, "falling");
            break;
        case both:
            strcpy(buf, "both");
            break;
        //case none:
        default:
            strcpy(buf, "");
            break;
    }
    if(write(fd, buf, strlen(buf)) < strlen(buf))
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could write in: /sys/class/gpio/gpio/edge"));
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}


int elite_slave_ready_gpio(void)
{
    int ret = -1;

   	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_gpioe,"GPIO","ESG BSP ELITE TDMA Context", DLT_LOG_INFO, DLT_TRACE_STATUS_DEFAULT);

    /* ret is an fd here, must be > 0*/
    ret = elite_gpio_open(
        GPIO_PORT_SPI_STM_READY,
        GPIO_ID_SPI_STM_READY,
        gpio_in,
        &slave_ready_gpio);


    if (0 < ret)
    {
        ret = elite_gpio_configure_event(&slave_ready_gpio, falling);
        if (0 != ret)
        {
            DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("elite_gpio_configure_event failed"));
        }
    }
    else
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("GPIO 'spi slave ready' init failed"));
    }
    
}
