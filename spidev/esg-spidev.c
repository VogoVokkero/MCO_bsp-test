/*
 ============================================================================
 Name        : spi.c
 Author      : tmu
 Version     :
 Copyright   : Closed
 Description : spi use tools
 ============================================================================
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "esg-bsp-test.h"

#include "esg-spidev.h"

DLT_IMPORT_CONTEXT(dlt_ctxt_rack);

#ifdef debug_verbose
static void hex_dump(const void *src, size_t length, size_t line_size,
                     char *prefix)
{
    int i = 0;
    const unsigned char *address = src;
    const unsigned char *line = address;
    unsigned char c;

    printf("%s | ", prefix);
    while (length-- > 0)
    {
        printf("%02X ", *address++);
        if (!(++i % line_size) || (length == 0 && i % line_size))
        {
            if (length == 0)
            {
                while (i++ % line_size)
                    printf("__ ");
            }
            printf(" |");
            while (line < address)
            {
                c = *line++;
                printf("%c", (c < 32 || c > 126) ? '.' : c);
            }
            printf("|\n");
            if (length > 0)
                printf("%s | ", prefix);
        }
    }
}
#endif // spi_init debug_verbose

//==============================================================================
//! \brief Start an SPI IOCTL transfert
//!
//! \param  fd: SPI file descriptor
//! \param  tx: Data to transmit
//! \param  rx: Data received
//! \param  len: data len to transmit
//! \return Data size receive on success; 0 if no data is received; < 0 if error
//==============================================================================
int spi_transfer(spi_dev_t *spi_struct, uint8_t const *tx, uint8_t const *rx, size_t len)
{
    int ret;
#ifdef debug_verbose
    struct timespec tx_time, rx_time;
#endif // debug_verbose
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = len,
        .delay_usecs = spi_struct->delay,
        .speed_hz = spi_struct->speed,
        .bits_per_word = spi_struct->bits,
    };

    if (spi_struct->mode & SPI_TX_QUAD)
        tr.tx_nbits = 4;
    else if (spi_struct->mode & SPI_TX_DUAL)
        tr.tx_nbits = 2;
    if (spi_struct->mode & SPI_RX_QUAD)
        tr.rx_nbits = 4;
    else if (spi_struct->mode & SPI_RX_DUAL)
        tr.rx_nbits = 2;
    if (!(spi_struct->mode & SPI_LOOP))
    {
        if (spi_struct->mode & (SPI_TX_QUAD | SPI_TX_DUAL))
            tr.rx_buf = 0;
        else if (spi_struct->mode & (SPI_RX_QUAD | SPI_RX_DUAL))
            tr.tx_buf = 0;
    }

#ifdef debug_verbose
    clock_gettime(CLOCK_MONOTONIC, &tx_time);
#endif // debug_verbose
    ret = ioctl(spi_struct->fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1)
    {
        DLT_LOG(dlt_ctxt_rack, DLT_LOG_VERBOSE, DLT_STRING("can't send spi message"));
    }

#ifdef debug_verbose
    clock_gettime(CLOCK_MONOTONIC, &rx_time);
    printf("SPI %d:\nTemps entre tx et rx pour transmettre %d octets: %lds %ldµs", fd, len, rx_time.tv_sec - tx_time.tv_sec, (rx_time.tv_nsec - tx_time.tv_nsec) / 1000);
#endif // debug_verbose

#ifdef debug_verbose
    hex_dump(tx, len, 32, "TX");
#endif // debug_verbose

#ifdef debug_verbose
    hex_dump(rx, len, 32, "RX");
#endif // debug_verbose
    return ret;
}

int spi_init(spi_dev_t *spi_struct, char *spi_device, uint32_t spi_speed, uint32_t spi_mode)
{
    int ret = EXIT_SUCCESS;

    // parse_opts(argc, argv);
    spi_struct->device = spi_device;
    spi_struct->fd = open(spi_struct->device, O_RDWR);
    if (spi_struct->fd < 0)
    {
        DLT_LOG(dlt_ctxt_rack, DLT_LOG_VERBOSE, DLT_STRING("can't open device"));
        ret = errno;
    }

    /*
     * spi mode:
     * SPI_LOOP | SPI_CPHA | SPI_CPOL | SPI_LSB_FIRST | SPI_CS_HIGH | SPI_3WIRE | SPI_NO_CS |
     * SPI_READY | SPI_TX_DUAL | SPI_TX_QUAD | SPI_LOOP | SPI_RX_DUAL | SPI_RX_QUAD
     */
    spi_struct->mode = spi_mode;

    if (EXIT_SUCCESS == ret)
    {
        int32_t val = ioctl(spi_struct->fd, SPI_IOC_WR_MODE32, &spi_struct->mode);
        if (0 > val)
        {
            DLT_LOG(dlt_ctxt_rack, DLT_LOG_VERBOSE, DLT_STRING("can't set spi mode"));
            ret = val;
        }
    }

    if (EXIT_SUCCESS == ret)
    {
        int32_t val = ioctl(spi_struct->fd, SPI_IOC_RD_MODE32, &spi_struct->mode);
        if (0 > val)
        {
            DLT_LOG(dlt_ctxt_rack, DLT_LOG_VERBOSE, DLT_STRING("can't get spi mode"));
            ret = val;
        }
    }

    if (EXIT_SUCCESS == ret)
    {
        int32_t val = ioctl(spi_struct->fd, SPI_IOC_WR_BITS_PER_WORD, &spi_struct->bits);
        if (0 > val)
        {
            DLT_LOG(dlt_ctxt_rack, DLT_LOG_VERBOSE, DLT_STRING("can't set bits per word"));
            ret = val;
        }
    }

    if (EXIT_SUCCESS == ret)
    {
        int32_t val = ioctl(spi_struct->fd, SPI_IOC_RD_BITS_PER_WORD, &spi_struct->bits);
        if (0 > val)
        {
            DLT_LOG(dlt_ctxt_rack, DLT_LOG_VERBOSE, DLT_STRING("can't get bits per word"));
            ret = val;
        }
    }

    /*
     * max speed hz (defaut 500kHz)
     */

    spi_struct->speed = spi_speed;
    if (EXIT_SUCCESS == ret)
    {
        int32_t val = ioctl(spi_struct->fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_struct->speed);
        if (0 > val)
        {
            DLT_LOG(dlt_ctxt_rack, DLT_LOG_VERBOSE, DLT_STRING("can't set max speed hz"));
            ret = val;
        }
    }

    if (EXIT_SUCCESS == ret)
    {
        int32_t val = ioctl(spi_struct->fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_struct->speed);
        if (0 > val)
        {
            DLT_LOG(dlt_ctxt_rack, DLT_LOG_VERBOSE, DLT_STRING("can't get max speed hz"));
            ret = val;
        }
    }

    DLT_LOG(dlt_ctxt_rack, DLT_LOG_INFO, DLT_STRING("spi mode"), DLT_UINT32(spi_struct->mode));
    DLT_LOG(dlt_ctxt_rack, DLT_LOG_INFO, DLT_STRING("bits per word"), DLT_UINT32(spi_struct->bits));
    DLT_LOG(dlt_ctxt_rack, DLT_LOG_INFO, DLT_STRING("max speed"), DLT_UINT32(spi_struct->speed));

    return ret;
}

void spi_close(spi_dev_t *spi_struct)
{
    if (spi_struct->fd)
    {
        close(spi_struct->fd);
        spi_struct->fd = 0;
    }
}
