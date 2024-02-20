/*
 ============================================================================
 Name        : avxSpi.c
 Author      : okhan
 Version     :
 Copyright   : Closed
 Description : avxSpi use tools
 ============================================================================
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>

#include "esg-spidev.h"
#include "stm32.h"

#define STM_SPIDEV  "/dev/spidev3.0"


spi_dev_t spi_dev = {0};

//==============================================================================
//! \brief Function to initialize the SPI bus
//!
//! \param  dev: pointer to the spi_dev_t sspitructure to be initialized
//! \param  dev_path: path to the spidev device in /dev
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int stm32_init(void)
{
   int ret = EXIT_SUCCESS;

   ret = spi_init(&spi_dev, STM_SPIDEV, SPI_STM_SPEED, SPI_MODE_3);
   if (0 > ret)
   {
      DLT_LOG(dlt_ctxt_stm32, DLT_LOG_ERROR, DLT_STRING("Failed to initialize SPI device"), DLT_STRING(STM_SPIDEV));
   }

   /* Read SR1 register */
   if (EXIT_SUCCESS == ret)
   {

   }

   return ret;
}

//==============================================================================
//! \brief Close the SPI bus
//!
//! \param  dev: pointer to the spi_dev_t sspitructure to be closed
//! \return none
//==============================================================================
void stm32_fini(void)
{
   spi_close(&spi_dev);
}
