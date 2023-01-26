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
#include <unistd.h>   //4 close/open
#include <string.h>   //4 strlen & memcpy
#include <sys/time.h> //4 time

#include "elite-slave-ready-gpio.h"
#include "dlt-client.h"

DLT_DECLARE_CONTEXT(dlt_ctxt_gpioe);

#define IMX_GPIO_NR(port, index) (((port)-1) * 32) + ((index)&31)
#define MAX_BUF 150 // max path is in linux/limits.h but 4096 seems way to much 4 me...

static int elite_gpio_open(elite_gpio_t *sready_gpio)
{
    int fd;
    char buf[MAX_BUF];
    int open_flag = O_RDONLY;
    int ret = 0;

    // Initialize GPIO
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd <= 0)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not open : /sys/class/gpio/export"));
        return -1;
    }
    sprintf(buf, "%d", sready_gpio->id);
    ret = write(fd, buf, strlen(buf)) < strlen(buf);
    if (0 != ret)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could write in: /sys/class/gpio/export"));
    }
    close(fd);

    DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_INFO, DLT_STRING("GPIO export"), DLT_UINT32(sready_gpio->id), DLT_UINT32(ret));

    // Set Direction
    sprintf(buf, "/sys/class/gpio/gpio%d/direction", sready_gpio->id);
    uint8_t timeout = 100;
    while (timeout)
    {
        fd = open(buf, O_WRONLY);
        if (fd <= 0)
        {
            usleep(10000);
            timeout--;
            if (!timeout)
                break;
        }
        else
        {
            break;
        }
    }

    if (fd <= 0)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not open : /sys/class/gpio/gpio47/direction"));
        return -1;
    }

    DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_INFO, DLT_STRING("GPIO direction openned"));

    /* Set in direction */
    if (write(fd, "in", 2) != 2)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not write : /sys/class/gpio/gpioX/direction"));
        return -1;
    }
    open_flag = O_RDONLY;

    close(fd);

    DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_INFO, DLT_STRING("GPIO direction written"));

    sprintf(buf, "/sys/class/gpio/gpio%d/value", sready_gpio->id);
    sready_gpio->fd = open(buf, open_flag);
    if (sready_gpio->fd <= 0)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not open : /sys/class/gpio/gpioX/value"));
        return -1;
    }

    DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_INFO, DLT_STRING("GPIO value openned fd="), DLT_UINT32(sready_gpio->fd));

    return sready_gpio->fd;
}

int elite_gpio_close(elite_gpio_t *sready_gpio)
{
    if (sready_gpio->fd)
    {
        int fd;
        char buf[MAX_BUF];
        close(sready_gpio->fd);
        fd = open("/sys/class/gpio/unexport", O_WRONLY);
        if (fd <= 0)
        {
            DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not open : /sys/class/gpio/unexport"));
            return -1;
        }
        sprintf(buf, "%d", sready_gpio->id);
        if (write(fd, buf, strlen(buf)) != strlen(buf))
        {
            DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not write : /sys/class/gpio/unexport"));
            close(fd);
            return -1;
        }
        close(fd);
        sready_gpio->fd = 0;
    }
    return 0;
}

int8_t elite_gpio_get(elite_gpio_t *sready_gpio)
{
    char value;
    if (read(sready_gpio->fd, &value, 1) < 1)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not read gpio"));
        return -1;
    }

    lseek(sready_gpio->fd, 0, SEEK_SET); // set the file ptr back at the start of the file
    return (int8_t)value - '0';
}

static int elite_gpio_configure_event(elite_gpio_t *sready_gpio)
{
    int fd;
    char buf[MAX_BUF];
    const char *edge = "falling";

    // Configure GPIO event
    sprintf(buf, "/sys/class/gpio/gpio%d/edge", sready_gpio->id);
    fd = open(buf, O_WRONLY);
    if (fd <= 0)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could not open /sys/class/gpio/gpioX/edge"));
        return -1;
    }

    DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_INFO, DLT_STRING("GPIO open edge"));

    //    strcpy(edge, "rising");

    if (write(fd, edge, strlen(buf)) < strlen(buf))
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("Could write in: /sys/class/gpio/gpio/edge"));
        close(fd);
        return -1;
    }

    DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_INFO, DLT_STRING("GPIO edge written"));

    close(fd);
    return 0;
}

int elite_slave_ready_gpio(elite_gpio_t *sready_gpio)
{
    int ret = -EINVAL;

    DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_gpioe, "GPIO", "ESG BSP ELITE TDMA Context", DLT_LOG_INFO, DLT_TRACE_STATUS_DEFAULT);

    if (NULL != sready_gpio)
    {
        sready_gpio->id = (32 + 15);

        ret = elite_gpio_open(sready_gpio);

        /* ret is an fd here, must be > 0*/
        if (0 < ret)
        {
            ret = elite_gpio_configure_event(sready_gpio);
        }
        else
        {
            DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("GPIO elite_slave_ready_gpio fd not > 0"));
            ret = -EBUSY;
        }
    }

    if (0 > ret)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("GPIO 'spi slave ready' init failed"));
    }
}

int elite_slave_ready_wait(elite_gpio_t *sready_gpio)
{
    fd_set read_fds;
    int ret_select;

    FD_ZERO(&read_fds);
    FD_SET(sready_gpio->fd, &read_fds);

    // Attente passive
    ret_select = select(sready_gpio->fd+1, NULL, NULL, &read_fds, NULL);

  //  DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_INFO, DLT_STRING("GPIO select ret_select="), DLT_UINT32(ret_select));

    if (ret_select < 0)
    {
        DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_ERROR, DLT_STRING("GPIO select failed"));
        // pthread_exit(0);
    }

    int val = elite_gpio_get(sready_gpio);

    DLT_LOG(dlt_ctxt_gpioe, DLT_LOG_VERBOSE, DLT_STRING("GPIO val ="), DLT_UINT32(val));
}
