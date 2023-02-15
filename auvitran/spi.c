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

#include "spi.h"

DLT_IMPORT_CONTEXT(dlt_ctxt_avx);

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
	if (!(spi_struct->mode & SPI_LOOP)) {
		if (spi_struct->mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr.rx_buf = 0;
		else if (spi_struct->mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr.tx_buf = 0;
	}
  
	ret = ioctl(spi_struct->fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
	{
		DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("can't send spi message"), DLT_UINT32(spi_struct->fd));
	}

	return ret;
}

int spi_init(spi_dev_t *spi_struct, char *spi_device, uint32_t spi_speed, uint32_t spi_mode)
{
	int ret = 0;

	//parse_opts(argc, argv);
    spi_struct->device = spi_device;
	spi_struct->fd = open(spi_struct->device, O_RDWR);
	if (spi_struct->fd < 0)
	{
		DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("can't open device"), DLT_UINT32(spi_struct->fd));
	}

	/*
	 * spi mode:
     * SPI_LOOP | SPI_CPHA | SPI_CPOL | SPI_LSB_FIRST | SPI_CS_HIGH | SPI_3WIRE | SPI_NO_CS |
     * SPI_READY | SPI_TX_DUAL | SPI_TX_QUAD | SPI_LOOP | SPI_RX_DUAL | SPI_RX_QUAD
	 */
    spi_struct->mode =  spi_mode;
            
	ret = ioctl(spi_struct->fd, SPI_IOC_WR_MODE32, &spi_struct->mode);
	if (ret == -1)
	{
		DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("can't set spi mode"), DLT_UINT32(spi_struct->fd));
	}

	ret = ioctl(spi_struct->fd, SPI_IOC_RD_MODE32, &spi_struct->mode);
	if (ret == -1)
	{
		DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("can't get spi mode"), DLT_UINT32(spi_struct->fd));
	}

	/*
	 * bits per word (default 8 bits)
	 */
	ret = ioctl(spi_struct->fd, SPI_IOC_WR_BITS_PER_WORD, &spi_struct->bits);
	if (ret == -1)
	{
		DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("can't set bits per word"), DLT_UINT32(spi_struct->fd));
	}

	ret = ioctl(spi_struct->fd, SPI_IOC_RD_BITS_PER_WORD, &spi_struct->bits);
	if (ret == -1)
	{
		DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("can't get bits per word"), DLT_UINT32(spi_struct->fd));
	}

	/*
	 * max speed hz (defaut 500kHz)
	 */
    spi_struct->speed = spi_speed;
	ret = ioctl(spi_struct->fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_struct->speed);
	if (ret == -1)
	{
		DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("can't set max speed hz"), DLT_UINT32(spi_struct->fd));
	}

	ret = ioctl(spi_struct->fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_struct->speed);
	if (ret == -1)
	{
		DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("can't get max speed hz"), DLT_UINT32(spi_struct->fd));
	}

	return spi_struct->fd;
}

void spi_close(spi_dev_t *spi_struct)
{
	if(spi_struct->fd)
	{
    	close(spi_struct->fd);
		spi_struct->fd = 0;
	}
}
