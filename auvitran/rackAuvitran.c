//==============================================================================
//! \addtogroup configuring audioApp
//! \ingroup core Core
//! \{
//!
//! \file 	rackAuvitran.c
//! \brief  to configure audioApp
//!
//! Historique :
//! 	- 09/04/2021 : Creation par Smile okhan
//!
//! \}
//=============================================================================
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "rackAuvitran.h"

#include "avxSpi.h"
#include "avxDefs.h"

DLT_DECLARE_CONTEXT(dlt_ctxt_rack);

typedef struct
{
   uint8_t product_id;
   uint8_t pages_addresses[PAGE_TYPE_MAX];
} slot_info_t;

typedef struct
{
   avx_device avx_device;
   slot_info_t slots[MAX_SLOT_NUMBERS];

} rackinfo_t;

// Task state and variables
static rackinfo_t RackAuvitran;

//==============================================================================
//! \brief Find page address for a given slot and page type
//!
//! \param  slot: Slot number
//! \param page_type: Type of page (IPR, OPR, GPIOPR, ...)
//! \return page address or error code if <0
//==============================================================================
int find_page_for(int slot, page_type_t page_type)
{
   if ((slot != AVBX7_SLOT) && (slot != MATRIX_SLOT) && ((slot < 0) || (slot > MAX_SLOT_NUMBERS)))
   {
      return -EINVAL;
   }

   if ((page_type < 0) || (page_type >= PAGE_TYPE_MAX))
   {
      return -EINVAL;
   }

   return RackAuvitran.slots[slot].pages_addresses[page_type];
}

//==============================================================================
//! \brief Remember what's in a slot, especially product ID
//!
//! \param  slot: Slot number
//! \param  product_id: content of the PIR register on the given slot
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int add_slot(int slot, uint8_t product_id)
{
   if ((slot != AVBX7_SLOT) && (slot != MATRIX_SLOT) && ((slot < 0) || (slot > MAX_SLOT_NUMBERS)))
   {
      return -EINVAL;
   }

   RackAuvitran.slots[slot].product_id = product_id;

   // Initialize all page addresses to -ENODATA
   memset(RackAuvitran.slots[slot].pages_addresses, -ENODATA, PAGE_TYPE_MAX);

   return 0;
}

//==============================================================================
//! \brief Set page address for a given slot and page type
//!
//! \param  slot: Slot number
//! \param  page_type: Type of page (enum: IPR, OPR, GPIOPR, ...)
//! \return 0 in case of success, or negative errno error code
//==============================================================================
static int set_slot_page_address(int slot, page_type_t page_type, uint8_t page_address)
{
   if ((slot != AVBX7_SLOT) && (slot != MATRIX_SLOT) && ((slot < 0) || (slot > MAX_SLOT_NUMBERS)))
   {
      return -EINVAL;
   }

   if ((page_type < 0) || (page_type >= PAGE_TYPE_MAX))
   {
      return -EINVAL;
   }

   RackAuvitran.slots[slot].pages_addresses[page_type] = page_address;

   return 0;
}

//==============================================================================
//! \brief Find in which slot a card sits, given its product ID
//!
//! \param  product_id: content of the PIR register on the given slot
//! \return slot or error code if <0
//==============================================================================
static int find_slot(uint8_t product_id)
{
   int slot = -ENOENT;

   for (slot = 0; slot < MAX_SLOT_NUMBERS; slot++)
   {
      if (RackAuvitran.slots[slot].product_id == product_id)
      {
         break;
      }
   }

   if (-ENOENT == slot)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed finding slot containing card with PIR "), DLT_HEX8(product_id));
   }

   return slot;
}

//==============================================================================
//! \brief Ask the rack for clock settings capabilities, and check it suits our needs
//!
//! \return 0 if capabilities are as expected, or negative errno error code
//==============================================================================
static int check_clock_capabilities(void)
{
   int ret;
   int page;
   uint8_t sr_capability;

   // Which is page index ?
   page = find_page_for(AVBX7_SLOT, PAGE_CLKPR);
   if (page < 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed finding CLKPR page address for core"));
      return page;
   }

   ret = avx_read_mailbox(&RackAuvitran.avx_device, AVBX7_SLOT, page, REG_CLKPR_SRCAP, &sr_capability, 1);
   if (ret != 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed reading clock capabilities from core, err: "), DLT_INT(ret));
      return ret;
   }

   /* We want 48kHz and x2 */
   if ((sr_capability & CLK_SRCAP_BASE) != CLK_SRCAP_44_1_48)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Rack clocking: missing 48kHz capability"));
      return -ENOSYS;
   }
   if ((sr_capability & CLK_SRCAP_x2) == 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Rack clocking: missing x2 capability"));
      return -ENOSYS;
   }

   return 0;
}

//==============================================================================
//! \brief Get product ID and pages addresses for a given slot. Do nothing if
//!        the slot is empty
//!
//! \param  slot: index of the slot to enquire
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int discover_slot(int slot)
{
   uint8_t PIR;
   uint8_t page_address;
   page_type_t page_type;
   uint8_t page_1_contents[8];
   int ret;

   ret = avx_get_slot_PIR(&RackAuvitran.avx_device, slot, &PIR);
   if (ret == -EIO)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_DEBUG, DLT_STRING("Empty slot: "), DLT_INT(slot));
      return 0;
   }
   else if (ret != 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed reading PIR for slot "), DLT_INT(slot), DLT_STRING(", err:"), DLT_INT(ret));
      return ret;
   }

   DLT_LOG(dlt_ctxt_rack, DLT_LOG_DEBUG, DLT_STRING("slot "), DLT_INT(slot), DLT_STRING(" contains PIR="), DLT_HEX8(PIR));

   ret = add_slot(slot, PIR);
   if (ret != 0)
   {
      return ret;
   }

   // Ask for page address for all interesting types of pages
   for (page_type = 0; page_type < PAGE_TYPE_MAX; page_type++)
   {
      ret = avx_get_slot_page_address(&RackAuvitran.avx_device, slot, page_type, &page_address);
      if (ret == 0)
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_DEBUG, DLT_STRING("slot "), DLT_INT(slot), DLT_STRING(" - page type "), DLT_INT(page_type), DLT_STRING(" @ "), DLT_UINT8(page_address));

         ret = set_slot_page_address(slot, page_type, page_address);
         if (ret != 0)
         {
            return ret;
         }
      }
      else
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed reading page address for slot "), DLT_INT(slot), DLT_STRING(", err: "), DLT_INT(ret));
         return ret;
      }
   }

   /* Compute the CRC-32 to unlock FPPR (flash) page. It is based on the 8 first bytes of page 1 */
   ret = avx_read_mailbox(&RackAuvitran.avx_device, slot, PAGE_1, 0, page_1_contents, sizeof(page_1_contents));

   return ret;
}

//==============================================================================
//! \brief Set sampling rate for the rack
//! \param sampling_rate: 44.1, 48, 88 or 96 kHz
//! \return 0 in case of success, or negative errno error code

/*  sampling_rate = FREQ_44_1k;

     sampling_rate = FREQ_48k;

     sampling_rate = FREQ_88_2k;

     sampling_rate = FREQ_96k;*/

//==============================================================================
int32_t rack_set_sampling_rate(sampling_rate_t sampling_rate)
{
   int32_t ret;
   int page;
   uint8_t sampling_encoded;

   // Which is page index ?
   page = find_page_for(AVBX7_SLOT, PAGE_CLKPR);
   if (page < 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed finding CLKPR page address for core"));
      return page;
   }

   switch (sampling_rate)
   {
   case FREQ_44_1k:
      sampling_encoded = CLK_SRCFG_44_1k;
      break;

   case FREQ_48k:
      sampling_encoded = CLK_SRCFG_48k;
      break;

   case FREQ_88_2k:
      sampling_encoded = CLK_SRCFG_88_2k;
      break;

   case FREQ_96k:
      sampling_encoded = CLK_SRCFG_96k;
      break;

   default:
      return -EINVAL;
   }

   ret = avx_write_mailbox(&RackAuvitran.avx_device, AVBX7_SLOT, page, REG_CLKPR_SRCFG, &sampling_encoded, 1);
   if (ret != 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed writing clock source to core, err: "), DLT_INT(ret));
   }

   return ret;
}

//==============================================================================
//! \brief Apply default parameters (such as gain, etc...) to the slots we
//!        discovered in the rack.
//!
//! \return 0 in case of success, or negative errno error code
//==============================================================================
static int32_t initialize_default_parameters(void)
{
   int32_t ret = EXIT_SUCCESS;

   /* Create default matrix */


   // If we have an AX4M card, then we set default gains
   if (find_slot(PID_AxC_AX4M) >= 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_INFO, DLT_STRING("Setting default parameters to AX4M card"));
      ret = rack_set_gain(IN, 1, DEFAULT_GAIN_AX4M_CHAN1);
      if (ret != 0)
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed to set default gain for AX4M (channel=1)"));
         return ret;
      }

      ret = rack_set_gain(IN, 4, DEFAULT_GAIN_AX4M_CHAN4);
      if (ret != 0)
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed to set default gain for AX4M (channel=4)"));
         return ret;
      }
   }

   return ret;
}

//==============================================================================
//! \brief Set a mapping in the audio matrix
//! \param slot_in: Slot index for the input
//! \param channel_in: Channel for the input, within range of input card
//! \param slot_out: Slot index for the output
//! \param channel_out: Channel for the output, within range of output card
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int32_t rack_matrix_set(int product_id_in, int channel_in, int product_id_out, int channel_out, bool enable_nMute)
{
   int32_t ret = EXIT_SUCCESS;
   int page_mxpr, page_out;
   int offset;
   uint8_t matrix_entry[2];
   uint8_t allowed;

   int32_t slot_in = find_slot(product_id_in);
   int32_t slot_out = find_slot(product_id_out);

   if (0 < slot_in)
   {
      slot_in |= enable_nMute;
   }
   else
   {
      ret = slot_in;
   }

   if (0 > slot_out)
   {
      ret = slot_out;
   }

   /*/

      enable_flag = MATRIX_SLOT_ENABLE;
      product_id_in = PID_AxC_AX4I;
      product_id_in = PID_AxC_AX4M;
      product_id_in = PID_AxC_VOGO;
      product_id_in = PID_AxC_DANTE;
      product_id_out = PID_AxC_AX40;
      product_id_out = PID_AxC_VOGO;
      product_id_out = PID_AxC_DANTE;*/
   if (EXIT_SUCCESS == ret)
   {
      page_mxpr = find_page_for(MATRIX_SLOT, PAGE_MXPR);
      if (page_mxpr < 0)
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed finding MXPR page address for core slot"));
         ret = page_mxpr;
      }
   }

   if (EXIT_SUCCESS == ret)
   {
      /* Are we allowed to modify matrix ? */
      ret = avx_read_mailbox(&RackAuvitran.avx_device, MATRIX_SLOT, page_mxpr, REG_MXPR_ALLOW, &allowed, 1);
      if (ret != 0)
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed reading matrix modification rights"));
      }
   }

   if ((EXIT_SUCCESS == ret) && ((allowed & MXPR_CHG_BIT) == 0))
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Modifying audio matrix is forbidden"));
      ret = -EPERM;
   }

   if (EXIT_SUCCESS == ret)
   {
      page_out = page_mxpr + 1 + (4 * slot_out) + ((channel_out - 1) / 16);
      offset = 2 * ((channel_out - 1) % 16);

      matrix_entry[0] = channel_in - 1;
      matrix_entry[1] = slot_in;
      ret = avx_write_mailbox(&RackAuvitran.avx_device, MATRIX_SLOT, page_out, offset, matrix_entry, sizeof(matrix_entry));
      if (ret != 0)
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed writing audio matrix: slot in "), DLT_INT(slot_in), DLT_STRING(", slot out "), DLT_INT(slot_out));
      }
   }

   return ret;
}

//==============================================================================
//! \brief task start function
//!
//! \param  startData: unused
//! \return none
//==============================================================================
uint32_t rack_initialize(void)
{
   uint8_t PIR = 0;
   int slot;
   uint32_t ret = avx_init(&RackAuvitran.avx_device, RACK_SPIDEV);

   if (0 > ret)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_FATAL, DLT_STRING("Cannot initialize SPI device"));
   }

   /* Read rack product ID, just to check communication is consistent */
   if (0 == ret)
   {
      ret = avx_get_rack_PIR(&RackAuvitran.avx_device, &PIR);
      if (ret != 0)
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed reading rack PIR, err="), DLT_INT(ret));
      }
   }

   if (PIR != PID_AxC_VOGO)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_WARN, DLT_STRING("rack PIR is "), DLT_HEX8(PIR), DLT_STRING(", expected "), DLT_HEX8(PID_AxC_VOGO));
      ret = -EINVAL;
   }

   /* Get page addresses for slot 14 which is the rack's matrix "virtual" slot */
   if (0 == ret)
   {
      ret = discover_slot(MATRIX_SLOT);
   }

   /* Get page addresses for slot 15 which is the rack's core */
   if (0 == ret)
   {
      ret = discover_slot(AVBX7_SLOT);
   }

   /* Check rack clocking capabilities are suitable to our needs */
   if (0 == ret)
   {
      ret = check_clock_capabilities();
   }

   if (0 == ret)
   {
      /* Now ask product ID for each slot and get page addresses */
      for (slot = FIRST_SLOT; slot <= LAST_SLOT; slot++)
      {
         ret = discover_slot(slot);
      }
   }

   if (0 == ret)
   {
      // Apply default parameters for the slots we discovered
      ret = initialize_default_parameters();
      if (ret != 0)
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed to apply default parameters, errno="), DLT_INT(ret));
      }
   }

   return ret;
}


//==============================================================================
//! \brief Set audio gain
//!
//! \param  direction: IN or OUT
//! \param  channel: channel number
//! \param  gain: gain in dB (typically negative)
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int32_t rack_set_gain(direction_t direction, int channel, float gain)
{
   int slot;
   int page;
   int gain_register;
   uint8_t product_id;
   uint8_t gain_encoded;
   int32_t ret = EXIT_SUCCESS;

   if ((channel < 1) || (channel > MAX_CHANNELS_NB))
   {
      return -EINVAL;
   }

   // Consider direction
   if (direction == IN)
   {
      /* See if we have an AX4I or AX4M card */
      if (find_slot(PID_AxC_AX4I) >= 0)
      {
         product_id = PID_AxC_AX4I;
         gain_register = REG_AX4I_GAIN_BASE;
      }
      else if (find_slot(PID_AxC_AX4M) >= 0)
      {
         product_id = PID_AxC_AX4M;
         gain_register = REG_AX4M_GAIN_BASE;
      }
      else
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("We have neither AX4I nor AX4M card to write gain"));
         ret = -EINVAL;
      }
   }
   else if (direction == OUT)
   {
      product_id = PID_AxC_AX40;
      gain_register = REF_AX4O_GAIN_BASE;
   }
   else
   {
      ret = -EINVAL;
   }

   if (EXIT_SUCCESS == ret)
   {
      // We have different min/max ranges depending on card (Ax4I/Ax4O or Ax4M)
      if ((product_id == PID_AxC_AX40) || (product_id == PID_AxC_AX4I))
      {
         if ((gain < GAIN_MIN_AX4I_O) || (gain > GAIN_MAX_AX4I_O))
         {
            DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Gain is out of range: "), DLT_FLOAT32(gain));
            ret = -EINVAL;
         }
      }
      else if (product_id == PID_AxC_AX4M)
      {
         if ((gain < GAIN_MIN_AX4M) || (gain > GAIN_MAX_AX4M))
         {
            DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Gain is out of range: "), DLT_FLOAT32(gain));
            ret = -EINVAL;
         }
      }
      else
      {
         ret = -EINVAL;
      }
   }

   if (EXIT_SUCCESS == ret)
   {
      // Where's the card ?
      slot = find_slot(product_id);
      if (slot < 0)
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed finding slot containing card with PIR "), DLT_HEX8(product_id));
         ret = slot;
      }
   }

   if (EXIT_SUCCESS == ret)
   {
      // Which is page index ?
      page = find_page_for(slot, PAGE_CSPR);
      if (page < 0)
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed finding CSPR page address for slot "), DLT_INT(slot));
         ret = page;
      }
   }

   if (EXIT_SUCCESS == ret)
   {
      // Encode gain and set value
      if ((product_id == PID_AxC_AX40) || (product_id == PID_AxC_AX4I))
      {
         gain_encoded = DB_FLOAT_TO_INT_AX4I_O(gain);
      }
      else if (product_id == PID_AxC_AX4M)
      {
         gain_encoded = DB_FLOAT_TO_INT_AX4M(gain);
      }
      else
      {
         ret = -EINVAL;
      }
   }

   if (EXIT_SUCCESS == ret)
   {
      ret = avx_write_mailbox(&RackAuvitran.avx_device, slot, page, gain_register + channel - 1, &gain_encoded, 1);
   }
   else
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Set gain failed"));
   }

   DLT_LOG(dlt_ctxt_rack, DLT_LOG_INFO, DLT_STRING("Set gain (dir/ch/val/encoded)"),
           DLT_INT(direction),
           DLT_INT(channel),
           DLT_FLOAT32(gain),
           DLT_HEX32(gain_encoded));
   return ret;
}

//==============================================================================
//! \brief Get audio gain
//!
//! \param  direction: IN or OUT
//! \param  channel: channel number
//! \param  gain: Updated with current gain in dB (typically negative)
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int32_t rack_get_gain(direction_t direction, int channel, float *gain)
{
   int slot;
   int page;
   int gain_register;
   uint8_t product_id;
   uint8_t gain_encoded;
   int32_t ret = EXIT_SUCCESS;

   if ((gain == NULL) || (channel < 1) || (channel > MAX_CHANNELS_NB))
   {
      return -EINVAL;
   }

   // Consider direction
   if (direction == IN)
   {
      /* See if we have an AX4I or AX4M card */
      if (find_slot(PID_AxC_AX4I) >= 0)
      {
         product_id = PID_AxC_AX4I;
         gain_register = REG_AX4I_GAIN_BASE;
      }
      else if (find_slot(PID_AxC_AX4M) >= 0)
      {
         product_id = PID_AxC_AX4M;
         gain_register = REG_AX4M_GAIN_BASE;
      }
      else
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("We have neither AX4I nor AX4M card to read gain"));
         return -EINVAL;
      }
   }
   else if (direction == OUT)
   {
      product_id = PID_AxC_AX40;
      gain_register = REF_AX4O_GAIN_BASE;
   }
   else
   {
      return -EINVAL;
   }

   // Where's the card ?
   slot = find_slot(product_id);
   if (slot < 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed finding slot containing card with PIR "), DLT_HEX8(product_id));
      return slot;
   }

   // Which is page index ?
   page = find_page_for(slot, PAGE_CSPR);
   if (page < 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed finding CSPR page address for slot "), DLT_INT(slot));
      return page;
   }

   // Get value and decode gain
   ret = avx_read_mailbox(&RackAuvitran.avx_device, slot, page, gain_register + channel - 1, &gain_encoded, 1);
   if (ret != 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed reading gain from slot "), DLT_INT(slot), DLT_STRING(", err: "), DLT_INT(ret));
      return ret;
   }
   if (gain_encoded == GAIN_MUTE)
   {
      // special case: mute
      *gain = -115.0;
   }
   else
   {
      if ((product_id == PID_AxC_AX40) || (product_id == PID_AxC_AX4I))
      {
         // -115 to +10 dB
         *gain = DB_INT_TO_FLOAT_AX4I_O(gain_encoded);
      }
      else if (product_id == PID_AxC_AX4M)
      {
         // -24 to +55 dB
         *gain = DB_INT_TO_FLOAT_AX4M(gain_encoded);
      }
      else
      {
         return -EINVAL;
      }
   }

   DLT_LOG(dlt_ctxt_rack, DLT_LOG_VERBOSE, DLT_STRING("get_gain dir/ch/reg/float"),
           DLT_INT(direction),
           DLT_INT(channel),
           DLT_HEX32(gain_encoded),
           DLT_FLOAT32(*gain));

   return 0;
}

//==============================================================================
//! \brief Set audio pad level
//!
//! \param  direction: IN or OUT
//! \param  channel: channel number
//! \param  pad_level: Either PAD_10_DBU or PAD_24_DBU
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int32_t rack_set_pad_level(direction_t direction, int channel, uint8_t pad_level)
{
   int slot;
   int page;
   int pad_register;
   uint8_t pad_bitfield;
   int32_t ret = EXIT_SUCCESS;

   /*
      direction = IN;

      pad_level = PAD_BITFIELD_10DBU;

      pad_level = PAD_BITFIELD_24DBU;
   */

   /* set a pad level */

   if ((channel < 1) || (channel > MAX_CHANNELS_NB) || (DIRECTION_INVALID <= direction))
   {
      ret = -EINVAL;
   }

   // Consider direction
   if (EXIT_SUCCESS == ret)
   {
      if (IN == direction)
      {
         slot = find_slot(PID_AxC_AX4I);
         /* See if we have an AX4I or AX4M card */
         if (slot >= 0)
         {
            pad_register = REG_AX4I_PAD;
         }
         else
         {
            DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Pad settings failed: found no AX4I card"));
            ret = -EINVAL;
         }
      }
      else /* OUT == direction */
      {
         slot = find_slot(PID_AxC_AX40);
         if (slot >= 0)
         {
            pad_register = REG_AX4O_PAD;
         }
         else
         {
            DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Pad settings failed: found no AX4O card"));
            ret = -EINVAL;
         }
      }
   }

   // Which is page index ?
   if (EXIT_SUCCESS == ret)
   {
      page = find_page_for(slot, PAGE_CSPR);
      if (page < 0)
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed finding CSPR page address for slot "), DLT_INT(slot));
         ret = -EADDRNOTAVAIL;
      }
   }

   // Get value and decode pad levels
   if (EXIT_SUCCESS == ret)
   {
      ret = avx_read_mailbox(&RackAuvitran.avx_device, slot, page, pad_register, &pad_bitfield, 1);
      if (0 != ret)
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed reading pad levels from slot "), DLT_INT(slot));
      }
   }

   if (EXIT_SUCCESS == ret)
   {
      // Zero-out the pad level for considered channel, then apply new value
      pad_bitfield &= ~(PAD_BITFIELD_MASK << ((channel - 1) * 2));
      pad_bitfield |= pad_level << ((channel - 1) * 2);
      ret = avx_write_mailbox(&RackAuvitran.avx_device, slot, page, pad_register, &pad_bitfield, 1);
      if (0 != ret)
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed writing pad levels to slot "), DLT_INT(slot));
      }
   }

   return ret;
}

//==============================================================================
//! \brief Get audio pad level
//!
//! \param  direction: IN or OUT
//! \param  channel: channel number
//! \param  pad_level: Updated with current pad level (either PAD_10_DBU or PAD_24_DBU)
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int32_t rack_get_pad_level(direction_t direction, int channel, uint8_t *pad_level)
{
   int slot;
   int page;
   int pad_register;
   uint8_t pad_bitfield;
   int32_t status = EXIT_SUCCESS;

   DLT_LOG(dlt_ctxt_rack, DLT_LOG_VERBOSE, DLT_STRING("get_pad_level (dir/ch)"),
           DLT_INT(direction),
           DLT_INT(channel));

   if ((pad_level == NULL) || (channel < 1) || (channel > MAX_CHANNELS_NB) || (DIRECTION_INVALID <= direction))
   {
      status = -EINVAL;
   }
   else
   {
      *pad_level = 0U; /*invalid*/
   }

   // Consider direction
   if (EXIT_SUCCESS == status)
   {
      if (IN == direction)
      {
         slot = find_slot(PID_AxC_AX4I);
         /* See if we have an AX4I or AX4M card */
         if (slot >= 0)
         {
            pad_register = REG_AX4I_PAD;
         }
         else
         {
            DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Pad settings failed: found no AX4I card"));
            status = -EINVAL;
         }
      }
      else /* OUT == direction */
      {
         slot = find_slot(PID_AxC_AX40);
         if (slot >= 0)
         {
            pad_register = REG_AX4O_PAD;
         }
         else
         {
            DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Pad settings failed: found no AX4O card"));
            status = -EINVAL;
         }
      }
   }

   // Which is page index ?
   if (EXIT_SUCCESS == status)
   {
      page = find_page_for(slot, PAGE_CSPR);
      if (page < 0)
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed finding CSPR page address for slot "), DLT_INT(slot));
         status = -ENXIO;
      }
   }

   // Get value and decode pad levels
   if (EXIT_SUCCESS == status)
   {
      if (0 != avx_read_mailbox(&RackAuvitran.avx_device, slot, page, pad_register, &pad_bitfield, 1))
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed reading pad levels from slot "), DLT_INT(slot));
         status = -ENXIO;
      }
      else
      {
         *pad_level = (pad_bitfield >> ((channel - 1) * 2)) & PAD_BITFIELD_MASK;
      }
   }

   return status;
}

//==============================================================================
//! \brief Get VU-meters for pre and post fader, value 15 indicates clipping.
//!
//! \param  direction: IN or OUT
//! \param  channel: channel number
//! \param  vu_pre: Updated with current VU-meter for pre-fader
//! \param  vu_post: Updated with current VU-meter for post-fader
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int32_t rack_get_vumeter(direction_t direction, int channel, int *vu_pre, int *vu_post)
{
   int slot;
   int page;
   page_type_t page_type;
   int vu_register;
   uint8_t product_id;
   uint8_t vu_values;
   int32_t ret = EXIT_SUCCESS;

   if ((vu_pre == NULL) || (vu_post == NULL))
   {
      ret = -EINVAL;
   }

   if ((channel < 1) || (channel > MAX_CHANNELS_NB))
   {
      ret = -EINVAL;
   }

   if (DIRECTION_INVALID <= direction)
   {
      ret = -EINVAL;
   }

   // Consider direction
   if ((EXIT_SUCCESS == ret) && (IN == direction))
   {
      page_type = PAGE_IPR;
      /* See if we have an AX4I or AX4M card */
      if (find_slot(PID_AxC_AX4I) >= 0)
      {
         product_id = PID_AxC_AX4I;
         vu_register = REG_AX4I_VU_BASE;
      }
      else if (find_slot(PID_AxC_AX4M) >= 0)
      {
         product_id = PID_AxC_AX4M;
         vu_register = REG_AX4M_VU_BASE;
      }
      else
      {
         DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("We have neither AX4I nor AX4M card to read VU-meter"));
         ret = -EINVAL;
      }
   }

   if ((EXIT_SUCCESS == ret) && (OUT == direction))
   {
      product_id = PID_AxC_AX40;
      vu_register = REG_AX4O_VU_BASE;
      page_type = PAGE_OPR;
   }

   // Where's the card ?
   if (EXIT_SUCCESS == ret)
   {
      slot = find_slot(product_id);
      if (slot < 0)
      {

         return slot;
      }
   }

   // Which is page index ?
   page = find_page_for(slot, page_type);
   if (page < 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed finding IPR/OPR page address for slot "), DLT_INT(slot));
      return page;
   }

   // Get values for vu-meters
   ret = avx_read_mailbox(&RackAuvitran.avx_device, slot, page, vu_register + channel - 1, &vu_values, 1);
   if (ret != 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed reading vu meters from slot "), DLT_INT(slot), DLT_STRING(", err: "), DLT_INT(ret));
      return ret;
   }

   // Decode vu meters values
   *vu_pre = vu_values & 0xF;
   *vu_post = (vu_values >> 4) & 0xF;

   return ret;
}

//==============================================================================
//! \brief Clear the whole audio matrix
//!
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int rack_matrix_clear(void)
{
   int slot;
   int page_mxpr;
   int page;
   static const uint8_t clean_page[32] = {0};
   int overall_ret;
   uint8_t allowed;
   int ret;

   page_mxpr = find_page_for(MATRIX_SLOT, PAGE_MXPR);
   if (page_mxpr < 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed finding MXPR page address for core slot"));
      return page_mxpr;
   }

   /* Are we allowed to modify matrix ? */
   ret = avx_read_mailbox(&RackAuvitran.avx_device, MATRIX_SLOT, page_mxpr, REG_MXPR_ALLOW, &allowed, 1);
   if (ret != 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed reading matrix modification rights"));
      return ret;
   }

   if ((allowed & MXPR_CHG_BIT) == 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Modifying audio matrix is forbidden"));
      return -EPERM;
   }

   page = page_mxpr + 1;
   overall_ret = 0;
   /* Note: we start from slot 0 ! */
   for (slot = 0; slot <= LAST_SLOT; slot++)
   {
      int index;

      /* Skip empty slots */
      if (0U == RackAuvitran.slots[slot].product_id)
      {
         page += 4;
         continue;
      }

      /* Clear 4 pages for one slot */
      for (index = 0; index < 4; index++)
      {
         int ret;
         ret = avx_write_mailbox(&RackAuvitran.avx_device, MATRIX_SLOT, page, 0, clean_page, sizeof(clean_page));
         if (ret != 0)
         {
            DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed clearing audio matrix, slot "), DLT_INT(slot), DLT_STRING(", page "), DLT_INT(page));
            overall_ret = ret;
         }
         page++;
      }
   }

   return overall_ret;
}

//==============================================================================
//! \brief Get the version string for a given slot (can be the core)
//! \param slot: Slot index
//! \param pFIR: Pointer to a byte representing firmware version
//! \param pEXT: Pointer to a byte representing firmware extension
//! \return 0 in case of success, or negative errno error code
//==============================================================================
int32_t rack_get_card_version(int slot, uint8_t *pFIR, uint8_t *pEXT)
{
   int32_t ret;

   if ((pFIR == NULL) || (pEXT == NULL))
   {
      return -EINVAL;
   }

   ret = avx_read_mailbox(&RackAuvitran.avx_device, slot, PAGE_1, REG_FIR, pFIR, 1);
   if (ret != 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed reading FIR from slot "), DLT_INT(slot), DLT_STRING(", err: "), DLT_INT(ret));
      return ret;
   }

   ret = avx_read_mailbox(&RackAuvitran.avx_device, slot, PAGE_1, REG_EXT, pEXT, 1);
   if (ret != 0)
   {
      DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("Failed reading EXT from slot "), DLT_INT(slot), DLT_STRING(", err: "), DLT_INT(ret));
      return ret;
   }

   return 0;
}

//==============================================================================
//! \brief Module termination, releases everything
//!
//! \param  none
//! \return none
//==============================================================================
void rack_release(void)
{
   DLT_LOG(dlt_ctxt_rack, DLT_LOG_WARN, DLT_STRING("rack:terminate"));

   avx_terminate(&RackAuvitran.avx_device);
}
