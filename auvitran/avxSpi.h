/*
 ============================================================================
 Name        : avxSpi.h
 Author      : okhan
 Version     :
 Copyright   : Closed
 Description : avxSpi use tools
 ============================================================================
 */
#ifndef AVXSPI_H
#define AVXSPI_H
#pragma once

#include <stdint.h>
#include <stddef.h>

#include "esg-spidev.h"
#include "stdbool.h"

typedef struct {
   spi_dev_t spi_dev;

   /* Low-level capabilities of the rack */
   bool burst_support;
   bool fast_support;
   bool mbox_support;

} avx_device;

// Note: we don't define all existing page types here
typedef enum
{
   PAGE_IPR = 0,
   PAGE_OPR,
   PAGE_CLKPR,
   PAGE_CSPR,
   PAGE_MXPR,
   PAGE_FPPR,
   PAGE_CNAPR,
   PAGE_TYPE_MAX
} page_type_t;

/* Base functions */
int avx_init(avx_device* dev, char *dev_path);

void avx_terminate(avx_device* dev);

int avx_write_byte(avx_device* dev, int page, int offset, uint8_t data);

int avx_read_byte(avx_device* dev, int page, int offset, uint8_t *data);

int avx_test_and_set(avx_device* dev, int page, int offset, uint8_t mask, uint8_t *data);

int avx_test_and_reset(avx_device* dev, int page, int offset, uint8_t mask, uint8_t *data);

int avx_write_burst(avx_device* dev, int page, int offset, const uint8_t *data, size_t length);

int avx_read_burst(avx_device* dev, int page, int offset, uint8_t *data, size_t length);

int32_t avx_write_mailbox(avx_device* dev, int slot, int page, int offset, const uint8_t *data, size_t length);

int avx_read_mailbox(avx_device* dev, int slot, int page, int offset, uint8_t *data, size_t length);

int avx_test_and_set_mailbox(avx_device* dev, int slot, int page, int offset, uint8_t mask, uint8_t *data);

int avx_test_and_reset_mailbox(avx_device* dev, int slot, int page, int offset, uint8_t mask, uint8_t *data);

int avx_flash_unlock(avx_device* dev, int slot, int page_fppr, uint32_t key);

/* Advanced functions */
int avx_get_rack_PIR(avx_device* dev, uint8_t *pir);

int avx_get_slot_PIR(avx_device* dev, uint8_t slot, uint8_t *pir);

int avx_get_slot_page_address(avx_device* dev, int slot, page_type_t page_type, uint8_t *page_address);

#endif //AVXSPI_H
