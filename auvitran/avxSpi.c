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

#include "avxSpi.h"
#include "avxDefs.h"

DLT_IMPORT_CONTEXT(dlt_ctxt_avx);

/* Internal functions */
static int avx_test_and_toggle(avx_device *dev, uint8_t command, int page, int offset, uint8_t mask, uint8_t *data);
static int avx_test_and_toggle_mailbox(avx_device *dev, uint8_t command, int slot, int page, int offset, uint8_t mask, uint8_t *data);
static int avx_write_mailbox_byte(avx_device *dev, int slot, int page, int offset, uint8_t data);
static int avx_read_mailbox_byte(avx_device *dev, int slot, int page, int offset, uint8_t *data);
static int avx_wait_mailbox_completion(avx_device *dev);
static int avx_wait_flash_completion(avx_device *dev, int slot, int page_fppr);

//==============================================================================
//! \brief Function to initialize the SPI bus
//!
//! \param  dev: pointer to the avx_device structure to be initialized
//! \param  dev_path: path to the spidev device in /dev
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_init(avx_device *dev, char *dev_path)
{
   uint8_t sr1 = 0;
   int ret;

   ret = spi_init(&dev->spi_dev, dev_path, AVX_SPI_HIGH_SPEED, SPI_MODE_3);
   if (ret < 0)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed to initialize SPI device"));
      return ret;
   }

   /* Read SR1 register */
   ret = avx_read_byte(dev, PAGE_0, REG_SR1, &sr1);
   if (ret != 0)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed to read from Auvitran"), DLT_UINT32(errno));
      return ret;
   }

   dev->burst_support = (sr1 & SR1_BURST_MASK) > 0;
   dev->fast_support = (sr1 & SR1_FAST_MASK) > 0;
   dev->mbox_support = (sr1 & SR1_MBOX_MASK) > 0;

  // TRACE_PRINT(trace_info, trace_rackAuvitran, "Auvitran Rack - burst:%d - fast:%d - mbox:%d", dev->burst_support, dev->fast_support, dev->mbox_support);
   return 0;
}

//==============================================================================
//! \brief Close the SPI bus
//!
//! \param  dev: pointer to the avx_device structure to be closed
//! \return none
//==============================================================================
void avx_terminate(avx_device *dev)
{
   spi_close(&dev->spi_dev);
}

//==============================================================================
//! \brief Write a single byte ("unitary write") to the SPI slave
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  page: page to be written
//! \param  offset: offset in page to access byte
//! \param  data: byte to be written
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_write_byte(avx_device *dev, int page, int offset, uint8_t data)
{
   uint8_t tx_buff[2];
   int ret;

   tx_buff[0] = UNITARY_WRITE | (offset & OFFSET_MASK);

   if (page == 0)
   {
      tx_buff[0] &= ~PAGE0_BIT;
   }
   else
   {
      // 1st, write target page to the proper register
      ret = avx_write_byte(dev, PAGE_0, REG_PAGE, page);
      if (ret < 0)
      {
         DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed changing (page/err"), DLT_HEX32(page), DLT_UINT32(errno));
         return ret;
      }

      tx_buff[0] |= PAGE0_BIT;
   }

   tx_buff[1] = data;

   ret = spi_transfer(&dev->spi_dev, tx_buff, NULL, sizeof(tx_buff));
   if (ret == -1)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed SPI transfer (err)"), DLT_UINT32(errno));
      return errno;
   }

   return 0;
}

//==============================================================================
//! \brief Read a single byte ("unitary read") from the SPI slave
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  page: page to be read
//! \param  offset: offset in page to access byte
//! \param  data: byte to be read
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_read_byte(avx_device *dev, int page, int offset, uint8_t *data)
{
   uint8_t buff[2];
   int ret;

   buff[0] = UNITARY_READ | (offset & OFFSET_MASK);
   buff[1] = page & 0xFF;
   ret = spi_transfer(&dev->spi_dev, buff, NULL, sizeof(buff));
   if (ret == -1)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed SPI transfer (err)"), DLT_UINT32(errno));
      return errno;
   }

   ret = spi_transfer(&dev->spi_dev, NULL, buff, sizeof(buff));
   if (ret == -1)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed SPI transfer (err)"), DLT_UINT32(errno));
      return errno;
   }

   // Getting all 1s shows the rack is not connected
   if ((buff[0] == 0xFF) && (buff[1] == 0xFF))
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed SPI transfer, rack is not connected"));
      return -ENODEV;
   }

   if (buff[0] == 0)
   {
      // Data not valid: read once again through a 16b NOP word
      ret = spi_transfer(&dev->spi_dev, NULL, buff, sizeof(buff));
      if (ret == -1)
      {
         DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed SPI transfer (err)"), DLT_UINT32(errno));
         return errno;
      }

      if (buff[0] == 0)
      {
         // Still invalid: give up
         DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Invalid read from SPI"));
         return -EINVAL;
      }
   }

   *data = buff[1];

   return 0;
}

//==============================================================================
//! \brief Set a single bit ("test and set") to the SPI slave
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  page: page to be read
//! \param  offset: offset in page to access byte/bit
//! \param  mask: mask to indicate which bit has to be set
//! \param  data: byte to be read
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_test_and_set(avx_device *dev, int page, int offset, uint8_t mask, uint8_t *data)
{
   return avx_test_and_toggle(dev, TEST_AND_SET, page, offset, mask, data);
}

//==============================================================================
//! \brief Clear a single bit ("test and reset") to the SPI slave
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  page: page to be read
//! \param  offset: offset in page to access byte/bit
//! \param  mask: mask to indicate which bit has to be cleared
//! \param  data: byte to be read
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_test_and_reset(avx_device *dev, int page, int offset, uint8_t mask, uint8_t *data)
{
   return avx_test_and_toggle(dev, TEST_AND_RESET, page, offset, mask, data);
}

//==============================================================================
//! \brief Set or clear a single bit ("test and set/reset") to the SPI slave
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  command: SPI command to determine if bit should be set or cleared
//! \param  page: page to be read
//! \param  offset: offset in page to access byte/bit
//! \param  mask: mask to indicate which bit has to be set or cleared
//! \param  data: byte to be read
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_test_and_toggle(avx_device *dev, uint8_t command, int page, int offset, uint8_t mask, uint8_t *data)
{
   uint8_t buff[2];
   int ret;

   buff[0] = TEST_AND_SET | (offset & OFFSET_MASK);
   buff[1] = mask;

   if (page == 0)
   {
      buff[0] &= ~PAGE0_BIT;
   }
   else
   {
      // 1st, write target page to the proper register
      ret = avx_write_byte(dev, PAGE_0, REG_PAGE, page);
      if (ret < 0)
      {
         DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed changing (page/err)"), DLT_HEX32(page), DLT_UINT32(ret));
         return ret;
      }

      buff[0] |= PAGE0_BIT;
   }

   ret = spi_transfer(&dev->spi_dev, buff, NULL, sizeof(buff));
   if (ret == -1)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed SPI transfer (err)"), DLT_UINT32(errno));
      return errno;
   }

   ret = spi_transfer(&dev->spi_dev, NULL, buff, sizeof(buff));
   if (ret == -1)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed SPI transfer (err)"), DLT_UINT32(errno));
      return errno;
   }

   // Getting all 1s shows the rack is not connected
   if ((buff[0] == 0xFF) && (buff[1] == 0xFF))
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed SPI transfer, rack is not connected"));
      return -ENODEV;
   }

   if (buff[0] == 0)
   {
      // Data not valid: read once again through a 16b NOP word
      ret = spi_transfer(&dev->spi_dev, NULL, buff, sizeof(buff));
      if (ret == -1)
      {
         DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed SPI transfer (err)"), DLT_UINT32(errno));
         return errno;
      }

      if (buff[0] == 0)
      {
         // Still invalid: give up
         DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Invalid read from SPI"));
         return -EINVAL;
      }
   }

   *data = buff[1];

   return 0;
}

//==============================================================================
//! \brief Write a buffer ("burst write") to the SPI slave
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  page: page to be written
//! \param  offset: offset in page to access data
//! \param  data: buffer to be written
//! \param  length: size of buffer
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_write_burst(avx_device *dev, int page, int offset, const uint8_t *data, size_t length)
{
   // +1 for 'command'
   uint8_t tx_buff[length + 1];
   int ret;

   if (!dev->burst_support)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Burst write not supported"));
      return -ENOSYS;
   }

   tx_buff[0] = UNITARY_WRITE | (offset & OFFSET_MASK);

   if (page == 0)
   {
      tx_buff[0] &= ~PAGE0_BIT;
   }
   else
   {
      // 1st, write target page to the proper register
      ret = avx_write_byte(dev, PAGE_0, REG_PAGE, page);
      if (ret < 0)
      {
         DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed changing (page/err)"), DLT_HEX32(page), DLT_UINT32(ret));
         return ret;
      }

      tx_buff[0] |= PAGE0_BIT;
   }

   memcpy(tx_buff + 1, data, length);

   ret = spi_transfer(&dev->spi_dev, tx_buff, NULL, sizeof(tx_buff));
   if (ret == -1)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed SPI transfer (err)"), DLT_UINT32(errno));
      return errno;
   }

   return 0;
}

//==============================================================================
//! \brief Read to a buffer ("burst read") from the SPI slave
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  page: page to be read
//! \param  offset: offset in page to access data
//! \param  data: buffer to be filled with data
//! \param  length: size of buffer
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_read_burst(avx_device *dev, int page, int offset, uint8_t *data, size_t length)
{
   // +2 for 'command' and +1 for STATUS byte
   uint8_t tx_buff[length + 3];
   uint8_t rx_buff[length + 3];
   int ret;

   if (!dev->burst_support)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Burst read not supported"));
      return -ENOSYS;
   }

   // 1st, send command and get status
   tx_buff[0] = UNITARY_READ | (offset & OFFSET_MASK);
   tx_buff[1] = page & 0xFF;
   bzero(tx_buff + 2, length + 1);
   bzero(rx_buff, sizeof(rx_buff));
   ret = spi_transfer(&dev->spi_dev, tx_buff, rx_buff, length + 2 + 1);
   if (ret == -1)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed SPI transfer (err)"), DLT_UINT32(errno));
      return errno;
   }

   if (rx_buff[2] == 0)
   {
      // Status byte is not ok
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Burst read failed"));
      return -EINVAL;
   }

   memcpy(data, rx_buff + 3, length);

   return 0;
}

//==============================================================================
//! \brief Write a single byte ("unitary write") to the SPI slave, in a given slot
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  slot: slot number of the card to be written
//! \param  page: page to be written
//! \param  offset: offset in page to access byte
//! \param  data: byte to be written
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_write_mailbox_byte(avx_device *dev, int slot, int page, int offset, uint8_t data)
{
   uint8_t ecs[2];
   uint8_t cmd[2];
   uint8_t answer[2];
   int ret;

   if (page == 0)
   {
      // We'll never write to page 0 on a slot
      return -ENOSYS;
   }

   // ECS : page , slot and flags (WP0=1, MOV=0)
   ecs[0] = slot & SLOT_MASK;
   ecs[0] &= ~MBX_MOV;
   ecs[0] |= MBX_WP0;
   ecs[1] = page;
   ret = avx_write_burst(dev, PAGE_0, REG_MBX_ECS_0, ecs, sizeof(ecs));
   if (ret != 0)
   {
      return ret;
   }

   // CMD
   cmd[0] = data;
   cmd[1] = (MBX_WRITE_PAGED << OFFSET_LENGTH) | (offset & OFFSET_MASK);
   ret = avx_write_burst(dev, PAGE_0, REG_MBX_CMD_0, cmd, sizeof(cmd));
   if (ret != 0)
   {
      return ret;
   }

   /* Now wait for mailbox operation to complete */
   ret = avx_wait_mailbox_completion(dev);
   if (ret != 0)
   {
      return ret;
   }

   /* Read data back */
   ret = avx_read_burst(dev, PAGE_0, REG_MBX_ANS_0, answer, sizeof(answer));
   if (ret != 0)
   {
      return ret;
   }

   if (answer[1] == 0)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed mailbox read operation (status)"));
      return -EIO;
   }
   if (answer[0] != data)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed mailbox read operation (data confirmation)"));
      return -EIO;
   }

   return 0;
}

//==============================================================================
//! \brief Write a buffer (up to 32 bytes) to the SPI slave, in a given slot
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  slot: slot number of the card to be written
//! \param  page: page to be written
//! \param  offset: offset in page to write buffer
//! \param  data: pointer to the buffer to be written
//! \param  length: size of buffer, must be < 32
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_write_mailbox(avx_device *dev, int slot, int page, int offset, const uint8_t *data, size_t length)
{
   uint8_t config[3];
   uint8_t cmd[2];
   uint8_t answer[2];
   int ret;

   if (length == 1)
   {
      return avx_write_mailbox_byte(dev, slot, page, offset, data[0]);
   }

   if (page == 0)
   {
      // We'll never write to page 0 on a slot
      return -ENOSYS;
   }

   if (length > AXC_PAGE_SIZE)
   {
      return -EINVAL;
   }

   // Write data to page 2
   ret = avx_write_burst(dev, PAGE_2, 0, data, length);
   if (ret != 0)
   {
      return ret;
   }

   // ECS[0], ECS[1] and MOV[0] : page , slot and flags (WP0=1, MOV=1)
   config[0] = slot & SLOT_MASK;
   config[0] |= MBX_MOV;
   config[0] |= MBX_WP0;
   config[1] = page;
   config[2] = length - 1;
   ret = avx_write_burst(dev, PAGE_0, REG_MBX_ECS_0, config, sizeof(config));
   if (ret != 0)
   {
      return ret;
   }

   // CMD
   cmd[0] = 0;
   cmd[1] = (MBX_WRITE_PAGED << OFFSET_LENGTH) | (offset & OFFSET_MASK);
   ret = avx_write_burst(dev, PAGE_0, REG_MBX_CMD_0, cmd, sizeof(cmd));
   if (ret != 0)
   {
      return ret;
   }

   /* Now wait for mailbox operation to complete */
   ret = avx_wait_mailbox_completion(dev);
   if (ret != 0)
   {
      return ret;
   }

   /* Read data back */
   ret = avx_read_burst(dev, PAGE_0, REG_MBX_ANS_0, answer, sizeof(answer));
   if (ret != 0)
   {
      return ret;
   }

   if (answer[1] == 0)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed mailbox read operation (status)"));
      return -EIO;
   }

   return 0;
}

//==============================================================================
//! \brief Read a single byte ("unitary read") from the SPI slave, in a given slot
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  slot: slot number of the card to be read
//! \param  page: page to be read
//! \param  offset: offset in page to access byte
//! \param  data: pointer to the byte to be updated
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_read_mailbox_byte(avx_device *dev, int slot, int page, int offset, uint8_t *data)
{
   uint8_t cmd[2];
   uint8_t ecs;
   uint8_t answer[2];
   int ret;

   // ECS[0] : slot and flags (WP0=0, MOV=0)
   ecs = slot & SLOT_MASK;
   ecs &= ~(MBX_WP0 & MBX_MOV);
   ret = avx_write_byte(dev, PAGE_0, REG_MBX_ECS_0, ecs);
   if (ret != 0)
   {
      return ret;
   }

   // CMD[0] and CMD[1]
   cmd[0] = page;
   cmd[1] = (MBX_READ_PAGED << OFFSET_LENGTH) | (offset & OFFSET_MASK);
   ret = avx_write_burst(dev, PAGE_0, REG_MBX_CMD_0, cmd, sizeof(cmd));
   if (ret != 0)
   {
      return ret;
   }

   /* Now wait for mailbox operation to complete */
   ret = avx_wait_mailbox_completion(dev);
   if (ret != 0)
   {
      return ret;
   }

   /* Read data back */
   ret = avx_read_burst(dev, PAGE_0, REG_MBX_ANS_0, answer, sizeof(answer));
   if (ret != 0)
   {
      return ret;
   }

   if (answer[1] == 0)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed mailbox read operation"));
      return -EIO;
   }

   *data = answer[0];

   return 0;
}

//==============================================================================
//! \brief Read a single byte ("unitary read") from the SPI slave, in a given slot
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  slot: slot number of the card to be read
//! \param  page: page to be read
//! \param  offset: offset in page to access byte
//! \param  data: pointer to the byte to be updated
//! \param  length: must be 1
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_read_mailbox(avx_device *dev, int slot, int page, int offset, uint8_t *data, size_t length)
{
   uint8_t cmd[2];
   uint8_t config[3];
   uint8_t answer;
   int ret;

   if (length == 1)
   {
      return avx_read_mailbox_byte(dev, slot, page, offset, data);
   }

   if (length > AXC_PAGE_SIZE)
   {
      return -EINVAL;
   }

   // ECS[0], ECS[1] and MOV[0] : page , slot and flags (WP0=0, MOV=1)
   config[0] = slot & SLOT_MASK;
   config[0] |= MBX_MOV;
   config[0] |= MBX_WP0;
   config[1] = page;
   config[2] = length - 1;
   ret = avx_write_burst(dev, PAGE_0, REG_MBX_ECS_0, config, sizeof(config));
   if (ret != 0)
   {
      return ret;
   }

   // CMD[0] and CMD[1]
   cmd[0] = 0;
   cmd[1] = (MBX_READ_PAGED << OFFSET_LENGTH) | (offset & OFFSET_MASK);
   ret = avx_write_burst(dev, PAGE_0, REG_MBX_CMD_0, cmd, sizeof(cmd));
   if (ret != 0)
   {
      return ret;
   }

   /* Now wait for mailbox operation to complete */
   ret = avx_wait_mailbox_completion(dev);
   if (ret != 0)
   {
      return ret;
   }

   /* Check status */
   ret = avx_read_byte(dev, PAGE_0, REG_MBX_ANS_1, &answer);
   if (ret != 0)
   {
      return ret;
   }

   if (answer == 0)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed mailbox read operation"));
      return -EIO;
   }

   /* Read data back (from page 2) */
   ret = avx_read_burst(dev, PAGE_2, 0, data, length);
   if (ret != 0)
   {
      return ret;
   }

   return 0;
}

//==============================================================================
//! \brief Set or clear a single bit ("test and set/reset") to the SPI slave in a given slot
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  command: SPI command to determine if bit should be set or cleared
//! \param  slot: slot number of the card to be written
//! \param  page: page to be written
//! \param  offset: offset in page to access byte/bit
//! \param  mask: mask to indicate which bit has to be set or cleared
//! \param  data: byte to be read
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_test_and_toggle_mailbox(avx_device *dev, uint8_t command, int slot, int page, int offset, uint8_t mask, uint8_t *data)
{
   uint8_t ecs[2];
   uint8_t cmd[2];
   uint8_t answer[2];
   int ret;

   if (page == 0)
   {
      // We'll never write to page 0 on a slot
      return -ENOSYS;
   }

   // ECS : page , slot and flags (WP0=1, MOV=0)
   ecs[0] = slot & SLOT_MASK;
   ecs[0] &= ~MBX_MOV;
   ecs[0] |= MBX_WP0;
   ecs[1] = page;
   ret = avx_write_burst(dev, PAGE_0, REG_MBX_ECS_0, ecs, sizeof(ecs));
   if (ret != 0)
   {
      return ret;
   }

   // CMD
   cmd[0] = mask;
   cmd[1] = (command << OFFSET_LENGTH) | (offset & OFFSET_MASK);
   ret = avx_write_burst(dev, PAGE_0, REG_MBX_CMD_0, cmd, sizeof(cmd));
   if (ret != 0)
   {
      return ret;
   }

   /* Now wait for mailbox operation to complete */
   ret = avx_wait_mailbox_completion(dev);
   if (ret != 0)
   {
      return ret;
   }

   /* Read data back */
   ret = avx_read_burst(dev, PAGE_0, REG_MBX_ANS_0, answer, sizeof(answer));
   if (ret != 0)
   {
      return ret;
   }

   if (answer[1] == 0)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed mailbox read operation (status)"));
      return -EIO;
   }

   *data = answer[0];

   return 0;
}

//==============================================================================
//! \brief Set a single bit ("test and set") to the SPI slave, in a given slot
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  slot: slot number of the card to be written
//! \param  page: page to be write
//! \param  offset: offset in page to access byte/bit
//! \param  mask: mask to indicate which bit has to be set
//! \param  data: byte to be read
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_test_and_set_mailbox(avx_device *dev, int slot, int page, int offset, uint8_t mask, uint8_t *data)
{
   return avx_test_and_toggle_mailbox(dev, MBX_TAS_PAGED, slot, page, offset, mask, data);
}

//==============================================================================
//! \brief Clear a single bit ("test and reset") to the SPI slave, in a given slot
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  slot: slot number of the card to be written
//! \param  page: page to be write
//! \param  offset: offset in page to access byte/bit
//! \param  mask: mask to indicate which bit has to be cleared
//! \param  data: byte to be read
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_test_and_reset_mailbox(avx_device *dev, int slot, int page, int offset, uint8_t mask, uint8_t *data)
{
   return avx_test_and_toggle_mailbox(dev, MBX_TAR_PAGED, slot, page, offset, mask, data);
}

//==============================================================================
//! \brief Wait for completion of a mailbox operation
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_wait_mailbox_completion(avx_device *dev)
{
   uint8_t cmd_1;
   int ret;

   // TODO: Timeout ! en effet !! 
   cmd_1 = 1;
   while (cmd_1)
   {
      ret = avx_read_byte(dev, PAGE_0, REG_MBX_CMD_1, &cmd_1);
      if (ret != 0)
      {
         DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed polling mailbox completion"));
         return ret;
      }
   }

   return 0;
}

//==============================================================================
//! \brief Read Product ID for the whole rack
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  pir: pointer to the PIR to be updated
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_get_rack_PIR(avx_device *dev, uint8_t *pir)
{
   int ret = -EINVAL;

   if ((NULL != pir) && (NULL != dev))
   {
      ret = avx_read_byte(dev, PAGE_1, REG_PIR, pir);
   }

   return ret;
}

//==============================================================================
//! \brief Read Product ID for a given slot
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  slot: slot number of the card to be read
//! \param  pir: pointer to the PIR to be updated
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_get_slot_PIR(avx_device *dev, uint8_t slot, uint8_t *pir)
{
   int ret = -EINVAL;

   if ((NULL != pir) && (NULL != dev))
   {
      ret = avx_read_mailbox(dev, slot, PAGE_1, REG_PIR, pir, 1);
   }

   return ret;
}

//==============================================================================
//! \brief Get the address of a given page type for a given slot
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  slot: slot number of the card to be read
//! \param  page_type: page type to ask for (as an enum)
//! \param  page_address: pointer to the address to be updated
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_get_slot_page_address(avx_device *dev, int slot, page_type_t page_type, uint8_t *page_address)
{
   int ret;
   uint8_t reg_index;

   if (page_address == NULL)
   {
      return -EINVAL;
   }

   switch (page_type)
   {
   case PAGE_IPR:
      reg_index = REG_IPR;
      break;

   case PAGE_OPR:
      reg_index = REG_OPR;
      break;

   case PAGE_CLKPR:
      reg_index = REG_CLKPR;
      break;

   case PAGE_CSPR:
      reg_index = REG_CSPR;
      break;

   case PAGE_MXPR:
      reg_index = REG_MXPR;
      break;

   case PAGE_FPPR:
      reg_index = REG_FPPR;
      break;

   case PAGE_CNAPR:
      reg_index = REG_CNAPR;
      break;

   default:
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Invalid page type"), DLT_UINT32(page_type));
      return -EINVAL;
   }

   ret = avx_read_mailbox(dev, slot, PAGE_1, reg_index, page_address, 1);
   return ret;
}

//==============================================================================
//! \brief Unlock the flash memory
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  slot: slot number of the card to be accessed
//! \param  page_fppr: index of the FPPR page
//! \param  key: 32-bits key required for unlocking
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_flash_unlock(avx_device *dev, int slot, int page_fppr, uint32_t key)
{
   uint32_t fcar;
   uint8_t fcr;
   uint8_t fsdr;
   int ret;

   /* Write key. Note the endianness is fine as is ! */
   ret = avx_write_mailbox(dev, slot, page_fppr, REG_FPPR_FUCKR, (uint8_t *)&key, sizeof(key));
   if (ret != 0)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed writing FUCKR key (slot)"), DLT_UINT32(slot));
      return ret;
   }

   /* Send unlock command */
   fcr = FLASH_UNLOCK;
   ret = avx_write_mailbox(dev, slot, page_fppr, REG_FPPR_FCR, &fcr, sizeof(fcr));
   if (ret != 0)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed writing flash unlock command (slot)"), DLT_UINT32(slot));
      return ret;
   }

   /* Wait for command completion */
   ret = avx_wait_flash_completion(dev, slot, page_fppr);
   if (ret != 0)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed waiting for flash command completion (slot)"), DLT_UINT32(slot));
      return ret;
   }

   /* Confirm success (or not) */
   ret = avx_read_mailbox(dev, slot, page_fppr, REG_FPPR_FCAR, (uint8_t *)&fcar, sizeof(fcar));
   if (ret != 0)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed reading flash unlock result (slot)"), DLT_UINT32(slot));
      return ret;
   }
   fcar = (fcar >> FLASH_FCR_IN_FCAR) & 0xFF;

   if (fcar != FLASH_UNLOCK)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed unlocking flash (slot)"), DLT_UINT32(slot));
      return -EIO;
   }

   /* Verify flash is unlocked */
   ret = avx_read_mailbox(dev, slot, page_fppr, REG_FPPR_FSDR, &fsdr, sizeof(fsdr));
   if (ret != 0)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed reading flash unlock result (slot)"), DLT_UINT32(slot));
      return ret;
   }

   if ((fsdr & FLASH_UNLOCKED_FLAG) == 0)
   {
      DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Flash still locked (slot)"), DLT_UINT32(slot));
      return ret;
   }

   return 0;
}

//==============================================================================
//! \brief Wait for completion of a flash memory operation
//!
//! \param  dev: pointer to the avx_device structure to be used
//! \param  slot: slot number of the card to be accessed
//! \param  page_fppr: index of the FPPR page
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int avx_wait_flash_completion(avx_device *dev, int slot, int page_fppr)
{
   uint8_t fcr;
   int ret;

   // TODO: Timeout !
   fcr = 1;
   while (fcr)
   {
      ret = avx_read_mailbox(dev, slot, page_fppr, REG_FPPR_FCR, &fcr, sizeof(fcr));
      if (ret != 0)
      {
         DLT_LOG(dlt_ctxt_avx, DLT_LOG_ERROR, DLT_STRING("Failed polling flash command completion"));
         return ret;
      }
   }

   return 0;
}
